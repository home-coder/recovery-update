#include <stdio.h>
#include "usb.h"

#define DEBUG
#ifdef DEBUG
#define dbgprint(format,args...) ({ \
		fprintf (stderr, "[%s] <%d>-->", __func__, __LINE__); \
		fprintf(stderr, format, ##args);})
#else
#define dbgprint(format,args...)
#endif

int main(int arc, char **argv)
{
	char *absolutePath = (char *)malloc(PATH_MAX);

	if ( !search_file_in_usb("update.zip", absolutePath) ) {
		dbgprint("--%s--\n", absolutePath);	
	}
	//ensure_path_mounted(absolutePath);

	return 0;
}
