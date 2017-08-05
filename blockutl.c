#include <stdio.h>
#include <string.h>

int yang_write_block(FILE *fp, int offset, const void *buffer)
{
	int ret = 0;

	fseek(fp, offset, SEEK_SET);
	ret = fwrite(buffer, 1, strlen(buffer), fp);
	return ret;
}

int yang_read_block(FILE *fp, int offset, void *buffer, int ret)
{
	int rret = 0;
	
	fseek(fp, offset, SEEK_SET);
	rret = fread(buffer, 1, ret, fp);
	return rret;
}
