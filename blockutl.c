#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int
wait_for_device(const char *fn)
{
	struct stat statbuf;
	int ret = 10;

	while (ret-- > 0) {
		if (!stat(fn, &statbuf)) {
			break;
		}
	}
	if (ret == 0) {
		printf("failed get device\n");
		return -1;
	}

	return 0;
}

int
yang_write_block(FILE * fp, int offset, const void *buffer, int len)
{
	int ret = 0;

	fseek(fp, offset, SEEK_SET);
	ret = fwrite(buffer, 1, len, fp);
	fflush(fp);
	fsync(fileno(fp));

	return ret;
}

int
yang_read_block(FILE * fp, int offset, void *buffer, int ret)
{
	int rret = 0;

	fseek(fp, offset, SEEK_SET);
	rret = fread(buffer, ret, 1, fp);
	return rret;
}
