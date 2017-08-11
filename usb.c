#include <errno.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fs_mgr.h>

#include "usb.h"
#include "roots.h"

#define MAX_DISK 4
#define MAX_PARTITION 8
#define TIME_OUT 6000000

static const char *USB_ROOT = "/usb/";
static const char *USB_POINT_UBUNTU = "/media/jiangxiujie";
static const char *USB_MONTPOINT = "/mnt/myhost";

struct usb_uevent {
	char *subsystem;
	char *action;
	char **usb_path;
};

//usb2.0
static const char *sys_uevent[] = {
	"/devices/platform/sunxi-ehci.1",
	"/devices/platform/sunxi-ehci.2",
	NULL
};

struct timeval tpstart, tpend;
float timeuse = 0;

#define MOUNT_EXFAT            "/system/bin/mount.exfat"

void
startTiming()
{
	gettimeofday(&tpstart, NULL);
}

void
endTimming()
{
	gettimeofday(&tpend, NULL);
	timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) +
	    (tpend.tv_usec - tpstart.tv_usec);
	LOGD("spend Time %f\n", timeuse);
}

int
check_file_exists(const char *path)
{
	int ret = -1;
	if (path == NULL) {
		return -1;
	}
	ret = access(path, F_OK);
	return ret;
}

bool
isMountpointMounted(const char *mp)
{
	char device[256];
	char mount_path[256];
	char rest[256];
	FILE *fp;
	char line[1024];

	if (!(fp = fopen("/proc/mounts", "r"))) {
		fprintf(stderr, "Error opening /proc/mounts (%s)",
			strerror(errno));
		return false;
	}

	while (fgets(line, sizeof (line), fp)) {
		line[strlen(line) - 1] = '\0';
		sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
		if (!strcmp(mount_path, mp)) {
			fclose(fp);
			return true;
		}
	}

	fclose(fp);
	return false;
}

bool
isDeviceNodeMounted(const char *devpath, char *mount_path)
{
	char device[256];
	char rest[256];
	FILE *fp;
	char line[1024];

	if (!(fp = fopen("/proc/mounts", "r"))) {
		fprintf(stderr, "Error opening /proc/mounts (%s)",
			strerror(errno));
		return false;
	}

	while (fgets(line, sizeof (line), fp)) {
		line[strlen(line) - 1] = '\0';
		sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
		if (!strcmp(device, devpath)) {
			fclose(fp);
			return true;
		}
	}

	fclose(fp);
	return false;
}

int
ensure_dev_mounted(const char *devPath, char *mountedPoint)
{
	char mount_path[256];
	int ret;

	if (devPath == NULL || mountedPoint == NULL) {
		return -1;
	}

	if (isDeviceNodeMounted(devPath, mount_path)) {
		printf("%s is mounted %s\n", devPath, mount_path);
		/* 如果是默认的U盘挂载点比如 ubuntu下/media/xxx android: /mnt/udisk or /mnt/usbhost/xx ,
		   那么就修改mountedPoint,并且不在重新挂载，即不走下面流程了
		   */
		int len = strlen(USB_POINT_UBUNTU);
		if (!strncmp(mount_path, USB_POINT_UBUNTU, len)) {
			strncpy(mountedPoint, mount_path, strlen(mount_path) + 1);
			printf("mountedPoint: %s\n", mountedPoint);
			return 0;
		} else {
		}
	} else {
		printf("%s nono no no \n", devPath);
		mkdir(mountedPoint, 0755);	//in case it doesn't already exist
	}

	startTiming();
	ret = mount(devPath, mountedPoint, "vfat",
		    MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
	endTimming();
#if 1
	if (ret == 0) {
		LOGD("mount %s with fs 'vfat' success\n", devPath);
		return 0;
	} else {
		startTiming();
		ret = mount(devPath, mountedPoint, "ntfs",
			    MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
		endTimming();
		if (ret == 0) {
			LOGD("mount %s with fs 'ntfs' success\n", devPath);
			return 0;
		} else {
			startTiming();
			ret = mount(devPath, mountedPoint, "ext4",
				    MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
			endTimming();
			if (ret == 0) {
				LOGD("mount %s with fs 'ext4' success\n",
				     devPath);
				return 0;
			} else {
				int status;
				pid_t pid = fork();
				if (pid > 0) {
					waitpid(pid, &status, 0);
					if (WIFEXITED(status)) {
						if (WEXITSTATUS(status) != 0) {
							fprintf(stderr,
								"%s terminated by exit(%d) \n",
								MOUNT_EXFAT,
								WEXITSTATUS
								(status));
						} else {
							fprintf(stderr,
								"mount %s with fs 'exfat' success\n",
								devPath);
							return 0;
						}
					} else if (WIFSIGNALED(status))
						fprintf(stderr,
							"%s terminated by signal %d \n",
							MOUNT_EXFAT,
							WTERMSIG(status));
					else if (WIFSTOPPED(status))
						fprintf(stderr,
							"%s stopped by signal %d \n",
							MOUNT_EXFAT,
							WSTOPSIG(status));
				} else if (pid == 0) {
					fprintf(stderr, "try run %s\n",
						MOUNT_EXFAT);
					if (execl
					    (MOUNT_EXFAT, MOUNT_EXFAT, devPath,
					     mountedPoint, "-o",
					     "noatime,nodiratime",
					     (char *) NULL) < 0) {
						int err = errno;
						fprintf(stderr,
							"Cannot run %s error %s \n",
							MOUNT_EXFAT,
							strerror(err));
						exit(-1);
					}
				} else {
					fprintf(stderr,
						"Fork failed trying to run %s\n",
						"/sbin/mount.exfat");
				}
			}
		}
		LOGD("failed to mount %s (%s)\n", devPath, strerror(errno));
		return -1;
	}
#endif
}

int
search_file_in_dev(const char *file, char *absolutePath,
		   const char *devPath, const char *devName)
{
	if (!check_file_exists(devPath)) {
		LOGD("dev %s exists\n", devPath);
		char mountedPoint[32];
		sprintf(mountedPoint, "%s%s", USB_ROOT, devName);
		//if the dev exists, try to mount it
		if (!ensure_dev_mounted(devPath, mountedPoint)) {
			LOGD("dev %s mounted in %s\n", devPath, mountedPoint);
			char desFile[PATH_MAX];
			dbgprint("mountedPoint %s\n", mountedPoint);
			sprintf(desFile, "%s/%s", mountedPoint, file);
			//if mount success.search des file in it
			if (!check_file_exists(desFile)) {
				//if find the file,return its absolute path
				dbgprint("file %s exist\n", desFile);
				sprintf(absolutePath, "%s", desFile);
				return 0;
			} else {
				ensure_dev_unmounted(mountedPoint);
			}
		}
	}
	return -1;
}

int
search_file_in_usb(const char *file, char *absolutePath)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	int i = 0;
	int j = 0;
	struct timeval workTime;
	long spends;
	mkdir(USB_ROOT, 0755);	//in case dir USB_ROOT doesn't already exist
	//do main work here
	do {
		LOGD("begin....\n");
		for (i = 0; i < MAX_DISK; i++) {
			char devDisk[32];
			char devPartition[32];
			char devName[8];
			char parName[8];
			sprintf(devName, "sd%c", 'a' + i);
			sprintf(devDisk, "/dev/block/%s", devName);
			LOGD("check disk %s\n", devDisk);
			if (check_file_exists(devDisk)) {
				LOGD("dev %s does not exists (%s),waiting ...\n", devDisk, strerror(errno));
				continue;
			}
			for (j = 1; j <= MAX_PARTITION; j++) {
				sprintf(parName, "%s%d", devName, j);
				sprintf(devPartition, "%s%d", devDisk, j);
				if (!search_file_in_dev
				    (file, absolutePath, devPartition,
				     parName)) {
					return 0;
				}
			}
			if (j > MAX_PARTITION) {
				if (!search_file_in_dev
				    (file, absolutePath, devDisk, devName)) {
					return 0;
				}
			}
		}
		usleep(500000);
		gettimeofday(&workTime, NULL);
		spends =
		    (workTime.tv_sec - now.tv_sec) * 1000000 +
		    (workTime.tv_usec - now.tv_usec);
	} while (spends < TIME_OUT);
	LOGD("Time to search %s is %ld\n", file, spends);
	return -1;
}

int
ensure_dev_unmounted(const char *mountedPoint)
{
	int ret = umount(mountedPoint);
	return ret;
}

int
in_usb_device(const char *file)
{
	int len = strlen(USB_ROOT);
	if (strncmp(file, USB_ROOT, len) == 0) {
		return 0;
	}
	return -1;
}

int usb_put(const char *file_path, const char *usb_dir)
{
	char buf[512];
	sprintf(buf, "cp %s %s", file_path, usb_dir);
	buf[strlen(buf) + 1] = '\0';
	printf("buf: %s\n", buf);

	return system(buf);
}

int usb_get()
{
	return 0;
}

int mount_dev2point(char *devpt, const char *mountpt)
{
	struct fstab *fstab = NULL;

	int i = 0, j = 0, ret = 0;
	char mountpoint[64] = {0};

	if (check_file_exists(devpt)) {
		return -1;
	}

	//根据label 和 uevent事件/device/platform/xxx是usbhost，得知设备为vfat，使用vfat的fstype来挂载U盘
	fstab = get_fstab();
	dbgprint("%d", fstab->num_entries);
	for (i = 0; i < fstab->num_entries; ++i) {
		Volume *v = &fstab->recs[i];
		printf("  %d %s %s %s %lld %s\n", i, v->mount_point, v->fs_type, v->blk_device, v->length, v->label);
		for (j = 0; j < sizeof(sys_uevent)/sizeof(sys_uevent[0]); j++) {
			if (!strncmp(v->blk_device, sys_uevent[j], sizeof(sys_uevent[j]))) {
				dbgprint("%s is matched, and it is USB2.0\n", devpt);
				if (!strcmp(v->fs_type, "vfat")) {
					sprintf(mountpoint, "%s/Storage0%d", mountpt, j + 1);
					//TODO mkdir mountpoint
					mkdir(mountpoint, 0755);
					if (!check_file_exists(mountpoint)) {
						ret = mount(devpt, mountpoint, "vfat",
								MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
						if (ret == 0) {
							dbgprint("%s mount  %s success\n", devpt, mountpoint);
							return ret;
						} else {
							dbgprint("mounted failed\n");
							return -1;
						}
					}
					return -1;
				}
			}

		}
	}
	return -1;
}

/*
TODO， 两种情况，1.开机直接检测并挂载设备，2.正常启动后要动态挂载，放到uevent的callback里面处理挂载
*/
int usb_mount()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	int i = 0;
	int j = 0;
	struct timeval workTime;
	long spends;
	mkdir(USB_MONTPOINT, 0755);
	//do main work here
	do {
		LOGD("begin....\n");
		for (i = 0; i < MAX_DISK; i++) {
			char devDisk[32];
			char devPartition[32];
			char devName[8];
			char parName[8];
			sprintf(devName, "sd%c", 'a' + i);
			sprintf(devDisk, "/dev/block/%s", devName);
			LOGD("check disk %s\n", devDisk);
			if (check_file_exists(devDisk)) {
				LOGD("dev %s does not exists (%s),waiting ...\n", devDisk, strerror(errno));
				continue;
			}
			for (j = 1; j <= MAX_PARTITION; j++) {
				sprintf(parName, "%s%d", devName, j);
				sprintf(devPartition, "%s%d", devDisk, j);
				//TODO
				if (mount_dev2point(devPartition, USB_MONTPOINT)) {
					dbgprint("%s is not exsit\n", devPartition);
					continue;
				} else {
					//TODO 挂载成功后，将相关信息重新写入fstab数据结构
					return 0;
				}
			}
		}
		usleep(500000);
		gettimeofday(&workTime, NULL);
		spends =
		    (workTime.tv_sec - now.tv_sec) * 1000000 +
		    (workTime.tv_usec - now.tv_usec);
	} while (spends < TIME_OUT);
	LOGD("Time to search %s is %ld\n", file, spends);

	return -1;

}

void usb_register_uevent()
{
	//封装注册事件
	struct usb_uevent usb_evt;
	usb_evt.subsystem = "usb";
	usb_evt.action = "add";
	usb_evt.usb_path = sys_uevent;

	//向uevent类注册一个usb的热插拔事件
	uevent_register_client(&usb_evt);
}
