#include <stdio.h>
#include <string.h>

int yang_write_block(FILE *fp, int offset, const void *buffer, int len)
{
	int ret = 0;

	fseek(fp, offset, SEEK_SET);
	ret = fwrite(buffer, len, 1, fp);
	return ret;
}

int yang_read_block(FILE *fp, int offset, void *buffer, int ret)
{
	int rret = 0;
	
	fseek(fp, offset, SEEK_SET);
	rret = fread(buffer, ret, 1, fp);
	return rret;
}
