// SPDX-License-Identifier: MIT
/* Attended watchdog exercise. No register access; watchdog API only. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <linux/watchdog.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t interrupted;

static void on_signal(int signo)
{
	(void)signo;
	interrupted = 1;
}

static int read_text(const char *path, char *buf, size_t size)
{
	int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	ssize_t n;

	if (fd < 0)
		return -1;
	n = read(fd, buf, size - 1);
	close(fd);
	if (n <= 0)
		return -1;
	buf[n] = 0;
	return 0;
}

static int readonly_bootstrap(void)
{
	glob_t paths = {0};
	char line[4096], value[16];
	FILE *mounts;
	int checked = 0, ret = -1;

	if (glob("/sys/class/block/*/ro", 0, NULL, &paths))
		return -1;
	for (size_t i = 0; i < paths.gl_pathc; ++i) {
		const char *name = paths.gl_pathv[i] + strlen("/sys/class/block/");

		if (strncmp(name, "mmcblk", 6) && strncmp(name, "loop", 4) &&
		    strncmp(name, "dm-", 3))
			continue;
		if (read_text(paths.gl_pathv[i], value, sizeof(value)) ||
		    strcmp(value, "1\n"))
			goto out;
		checked++;
	}
	if (checked < 30)
		goto out;
	mounts = fopen("/proc/mounts", "r");
	if (!mounts)
		goto out;
	ret = 0;
	while (fgets(line, sizeof(line), mounts)) {
		char source[1024], target[1024], type[64];

		if (sscanf(line, "%1023s %1023s %63s", source, target, type) != 3 ||
		    !strcmp(type, "ext4") || !strcmp(type, "ext2") ||
		    !strcmp(type, "overlay")) {
			ret = -1;
			break;
		}
	}
	fclose(mounts);
out:
	globfree(&paths);
	return ret;
}

static double uptime_seconds(void)
{
	struct timespec now;

	if (clock_gettime(CLOCK_BOOTTIME, &now))
		return -1;
	return now.tv_sec + now.tv_nsec / 1e9;
}

static int disarm(int fd)
{
	int result = write(fd, "V", 1) == 1 ? 0 : -1;

	if (close(fd))
		result = -1;
	puts(result ? "WATCHDOG_DISARM_FAILED" : "WATCHDOG_MAGIC_CLOSE_SENT");
	return result;
}

int main(int argc, char **argv)
{
	struct utsname uts;
	struct watchdog_info info = {0};
	char compatible[256], driver[256];
	bool bite;
	int fd, timeout = 20, zero = 0;
	double start;
	ssize_t n;

	setvbuf(stdout, NULL, _IOLBF, 0);
	if (argc != 2 || (strcmp(argv[1], "exercise") && strcmp(argv[1], "bite"))) {
		fprintf(stderr, "usage: watchdog-test exercise|bite\n"
			"Both modes require the attended checkpoint, unmounted data and all block nodes read-only.\n");
		return 2;
	}
	bite = !strcmp(argv[1], "bite");
	if (uname(&uts) || strcmp(uts.release, "6.19.5-thinksmart1-g770e10fa15a0") ||
	    read_text("/sys/firmware/devicetree/base/compatible", compatible, sizeof(compatible)) ||
	    strcmp(compatible, "lenovo,cd-18781y") || readonly_bootstrap()) {
		fputs("STOP: kernel, board or read-only checkpoint mismatch\n", stderr);
		return 3;
	}
	n = readlink("/sys/class/watchdog/watchdog0/device/driver", driver, sizeof(driver) - 1);
	if (n <= 0)
		return 3;
	driver[n] = 0;
	if (!strrchr(driver, '/') || strcmp(strrchr(driver, '/') + 1, "qcom_wdt"))
		return 3;
	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);
	fd = open("/dev/watchdog0", O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0) {
		perror("open watchdog");
		return 4;
	}
	if (ioctl(fd, WDIOC_GETSUPPORT, &info) ||
	    strncmp((char *)info.identity, "qcom_wdt", sizeof(info.identity)) ||
	    !(info.options & WDIOF_MAGICCLOSE) ||
	    ioctl(fd, WDIOC_SETTIMEOUT, &timeout) || timeout != 20 ||
	    ((info.options & WDIOF_PRETIMEOUT) && ioctl(fd, WDIOC_SETPRETIMEOUT, &zero)) ||
	    ioctl(fd, WDIOC_GETTIMEOUT, &timeout) || timeout != 20) {
		fputs("STOP: watchdog API verification failed\n", stderr);
		disarm(fd);
		return 5;
	}
	start = uptime_seconds();
	if (start < 0) {
		disarm(fd);
		return 5;
	}
	printf("WATCHDOG_%s_ARMED timeout=%d pretimeout=0\n", bite ? "BITE" : "EXERCISE", timeout);
	if (bite) {
		/* Keep the descriptor open without feeding. No magic close on success. */
		while (!interrupted && uptime_seconds() - start < 45)
			sleep(1);
		fputs("WATCHDOG_RESET_NOT_OBSERVED_OR_TEST_INTERRUPTED\n", stderr);
		disarm(fd);
		return 6;
	}
	while (!interrupted && uptime_seconds() - start < 25) {
		if (ioctl(fd, WDIOC_KEEPALIVE, &zero)) {
			disarm(fd);
			return 7;
		}
		sleep(1);
	}
	if (disarm(fd) || interrupted)
		return 8;
	/* Survive beyond the last timeout to prove that magic close stopped it. */
	start = uptime_seconds();
	while (!interrupted && uptime_seconds() - start < 25)
		sleep(1);
	if (interrupted)
		return 8;
	puts("WATCHDOG_FEED_AND_DISARM_PASS");
	return 0;
}
