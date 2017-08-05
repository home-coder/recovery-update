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
	usb_info thuinfo = {
		.no = 'A',
		.name = "this is my usb",
		.speed = 115200,
	};
	int ret = 0;
	int offset = 32;
	usb_info *pinfo;
	char *readbuf = NULL, *writebuf = NULL;
	usb_info *uinfo = &thuinfo;

	FILE *fp = fopen(USBPATH, "w+");
	if (!fp) {
		fprintf(stderr, "open failed (%s)", strerror(errno));
		goto fail1;
	}
	writebuf = (char *)malloc(sizeof(usb_info));
	memcpy(writebuf, uinfo, sizeof(usb_info));
	pinfo = (usb_info *)writebuf;
	dbgprint("no:%c, name:%s, speed:%d\n", pinfo->no, pinfo->name, pinfo->speed);

	ret = yang_write_block(fp, offset, writebuf);
	dbgprint("ret = %d\n", ret);

	readbuf = (char *)malloc(ret + 1);
	memset(readbuf, 0, ret + 1);
	fseek(fp, 0L, SEEK_SET);
	//rewind(fp);
	yang_read_block(fp, offset, readbuf, ret);

	printf("%s\n", readbuf);
	pinfo = (usb_info *)readbuf;
	dbgprint("no:%c, name:%s, speed:%d\n", pinfo->no, pinfo->name, pinfo->speed);
	fclose(fp);
#endif

	return 0;
fail1:
	exit(-1);
}
