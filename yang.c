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
#define TEST_STRUCT 1

typedef struct _usb_info {
	char no;
	char name[32];
	int speed;
	char attr[64];
}usb_info;

int main(int arc, char **argv)
{
	char *absolutePath = (char *)malloc(PATH_MAX);

#if 0
	if ( !search_file_in_usb("update.zip", absolutePath) ) {
		dbgprint("--%s--\n", absolutePath);	
	}
	//ensure_path_mounted(absolutePath);
#endif

#if TEST_STR
	const char *writebuf = "--------------------hello world-------";
	int offset = 32;
	char *readbuf = NULL;
	int ret = 0;

	FILE *fp = fopen(USBPATH, "w+");
	if (!fp) {
		fprintf(stderr, "open failed (%s)", strerror(errno));
		goto fail1;
	}
	ret = yang_write_block(fp, offset, writebuf);
	dbgprint("ret = %d\n", ret);

	readbuf = (char *)malloc(ret + 1);
	memset(readbuf, 0, ret + 1);
	fseek(fp, 0L, SEEK_SET);
	//rewind(fp);
	yang_read_block(fp, offset, readbuf, ret);

	printf("%s\n", readbuf);
	fclose(fp);
#endif

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

	ret = yang_write_block(fp, offset, uinfo, sizeof(usb_info));
	//如果ret与usb_info的大小不同，说明没有很好的写入，破坏了结构体，最好就放弃
	if (ret != sizeof(usb_info)) {
		dbgprint("failed to write over ret = %d, sizeof(usb_info) = %ld\n", ret, sizeof(usb_info));
		goto fail2;
	}
	dbgprint("ret = %d\n", ret);

	readbuf = (char *)malloc(ret + 1);
	memset(readbuf, 0, ret + 1);
	fseek(fp, 0L, SEEK_SET);
	//rewind(fp);
	yang_read_block(fp, offset, readbuf, ret);

	pinfo = (usb_info *)readbuf;
	dbgprint("no:%c, name:%s, speed:%d, attr:%s\n", pinfo->no, pinfo->name, pinfo->speed, pinfo->attr);
	fclose(fp);
#endif

	return 0;
fail2:
	fclose(fp);
fail1:
	exit(-1);
}
