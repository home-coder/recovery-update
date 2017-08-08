#include <stdio.h>

int main()
{
	const char *buf = "/dev/block/mmcblk0p1 /sdcard vfat rw,sync,dirsync,fmask=0000,dmask=0000,codepage=cp437,iocharset=iso8859-1,utf8 0 0";

	char device[32];
	char mount_point[64];
	char filesystem[64];
	char flags[128];
	int matches;

	/* %as is a gnu extension that malloc()s a string for each field.
	*/
	//matches = sscanf(buf, "%63s %63s %63s %127s",
	matches = sscanf(buf, "%s %s %s %127s",
			device, mount_point, filesystem, flags);

	printf("%s\n%s\n%s\n%s\n", device, mount_point, filesystem, flags);
	
	return 0;
}
