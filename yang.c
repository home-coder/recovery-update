#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "usb.h"

#define DEBUG
#ifdef DEBUG
#define dbgprint(format,args...) ({ \
		fprintf (stderr, "[%s] <%d>-->", __func__, __LINE__); \
		fprintf(stderr, format, ##args);})
#else
#define dbgprint(format,args...)
#endif

#define USBPATH  "/dev/sdd1"

#define TEST_MOUNT  1 //测试挂载一个分区节点到系统的目录
#define TEST_STR    0 //测试写入分区一个字符串
#define TEST_STRUCT 0 //测试写入分区一个结构体，最好不要含有指针
#define TEST_PID    0 //测试通过进程名字找到进程id，反之通过id找到进程的名字
#define TEST_FSTAB  1 //测试文件系统一系列操作

typedef struct _usb_info {
	char no;
	char name[32];
	int speed;
	char attr[64];
} usb_info;

int
main(int arc, char **argv)
{
	int ret = 0;
	char *absolutePath = (char *) malloc(PATH_MAX);

#if TEST_MOUNT
	if (!search_file_in_usb("update.zip", absolutePath)) {
		dbgprint("--%s--\n", absolutePath);
	}
#endif
#if TEST_FSTAB
	load_volume_table();
	//test usb
	/*
	1.根据fstab配置文件挂载U盘(重点是 label=usbhost)，目前主动调用并挂载U盘，以后uevent回调即可
	2.挂载U盘后，重新写入fstab的配置，将auto等替换成挂载点比如/mnt/usbhost/MyStorage
	3.测试U盘是否已经挂载，调用ensure_path_mounted或者自己写的那个usb.c中的方法
	*/
	//第一次挂载
	usb_mount();
	//后期热插拔
	usb_register_uevent();
#endif

#if TEST_STR
	const char *writebuf =
	    "--------------------hello world-------hello world---hello";
	int offset = 32;
	char *readbuf = NULL;
	int ret = 0;

	FILE *fp = fopen(USBPATH, "w+");
	if (!fp) {
		fprintf(stderr, "open failed (%s)", strerror(errno));
		goto fail1;
	}
	ret = yang_write_block(fp, offset, writebuf, strlen(writebuf) + 1);
	dbgprint("ret = %d\n", ret);

	readbuf = (char *) malloc(ret + 1);
	memset(readbuf, 0, ret + 1);
	fseek(fp, 0L, SEEK_SET);
	//rewind(fp);
	yang_read_block(fp, offset, readbuf, ret);

	printf("%s\n", readbuf);
	fclose(fp);

	return 0;
      fail2:
	fclose(fp);
      fail1:
	exit(-1);
#endif
	printf("======================================\n");
#if TEST_STRUCT
	int ret = 0;
	int offset = 32;
	char *readbuf = NULL, *writebuf = NULL;
	usb_info *pinfo;

	usb_info thuinfo = {
		.no = 'A',
		.name = "this is my usb",
		.speed = 115200,
		.attr = "i am a USB-3.0 Device",
	};
	usb_info *uinfo = &thuinfo;

	ret = wait_for_device(USBPATH);
	if (ret != 0) {
		dbgprint("can not get valid device, exit\n");
		goto fail1;
	}

	FILE *fp = fopen(USBPATH, "wb+");
	if (!fp) {
		fprintf(stderr, "open failed (%s)", strerror(errno));
		goto fail1;
	}

	ret = yang_write_block(fp, offset, uinfo, sizeof (usb_info));
	//如果ret与usb_info的大小不同，说明没有很好的写入，破坏了结构体，最好就放弃
	if (ret != sizeof (usb_info)) {
		dbgprint
		    ("failed to write over ret = %d, sizeof(usb_info) = %ld\n",
		     ret, sizeof (usb_info));
		goto fail2;
	}
	dbgprint("ret = %d\n", ret);

	readbuf = (char *) malloc(ret + 1);
	memset(readbuf, 0, ret + 1);
	fseek(fp, 0L, SEEK_SET);
	//rewind(fp);
	yang_read_block(fp, offset, readbuf, ret);

	pinfo = (usb_info *) readbuf;
	dbgprint("no:%c, name:%s, speed:%d, attr:%s\n", pinfo->no, pinfo->name,
		 pinfo->speed, pinfo->attr);
	fclose(fp);

#endif

#if TEST_PID
#endif
	return 0;
      fail2:
#if TEST_STR || TEST_STRUCT
	fclose(fp);
#endif
      fail1:
	exit(-1);

}
