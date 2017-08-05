#include <stdio.h>
#include <string.h>
#include <errno.h>
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

int main(int arc, char **argv)
{
	char *absolutePath = (char *)malloc(PATH_MAX);

#if 0
	if ( !search_file_in_usb("update.zip", absolutePath) ) {
		dbgprint("--%s--\n", absolutePath);	
	}
	//ensure_path_mounted(absolutePath);
#endif

#if 1
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
#endif

	return 0;
fail1:
	exit(-1);
}
