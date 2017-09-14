#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include<errno.h>

#define DEBUG
#ifdef DEBUG
#define dbgprint(format,args...) ({ \
		fprintf (stderr, "[%s] <%d>-->", __func__, __LINE__); \
		fprintf(stderr, format, ##args);})
#else
#define dbgprint(format,args...)
#endif

static const char *COMMAND_LINE = "console=ttyS0,115200 root=/dev/block/system init=/init loglevel=1 vmalloc=384M partitions=bootloader@nanda:env@nandb:boot@nandc:system\
@nandd:misc@nande:recovery@nandf:sysrecovery@nandg:private@nandh:Reserve0@nandi:klog@nandj:Reserve1@nandk:Reserve2@nandl:cache@nandm:nandm@nandn:UDISK@nando \
mac_addr= wifi_mac= bt_mac= specialstr= serialno= inside_model= business_model= launcher_channelid= fake_flash=2 boot_type=0 \
disp_para=20b0000 init_disp=20b0404 tv_vdid=0 fb_base=0x46400000 config_size=49152";

static void make_device(const char *path,
		int major, int minor)
{
	unsigned int dev = makedev(major, minor);
	//setegid(gid);
	mknod(path, 0755, dev);
	//chown(path, uid, -1); 
}

static void creat_link(const char *partition, const char *link)
{
	char buf[80];   
	getcwd(buf,sizeof(buf));   
	dbgprint("current working directory: %s\n", buf); 
	char real_partition[64] = {0};
	
	sprintf(real_partition, "%s/%s", buf, partition);
	dbgprint("real_partition %s\n", real_partition);

	/*
	*此处注意创建软链接注意路径写法  
	*http://blog.163.com/wang_ly2442/blog/static/94943407201297115318735/
	*/
	symlink(real_partition, link);
}

int main()
{
	char *pt = NULL;
	char *delim_part = ":";
	char *delim_link = "@";
	char *token = NULL;
	char *node = NULL;
	char *ppt = NULL;
	int flag = 1;
	char partition[64] = {0}, link[64] = {0};


	/* 忽略返回值error，因为每一次程序执行都执行，如果已经存在就会报error，忽略  */
	mkdir("./dev", 0755);
	mkdir("./dev/block", 0755);
	mkdir("./dev/block/by-name", 0755);

	pt = strstr(COMMAND_LINE, "partitions=");
	pt = pt + strlen("partitions=");

	if (pt) {
		ppt = (char *)malloc(strlen(pt));
		memccpy(ppt, pt, ' ', strlen(pt));
		dbgprint("ppt = %s\n", ppt);
	}

	/* 创建设备节点，并通过link对节点进行抽象，这样device下的fstab文件便可以写好接口不再因为底层设备变化如nand或者emmc而变化*/
	while ( (token = strsep(&ppt, delim_part)) != NULL ) {
		//dbgprint("token = %s\n", token);	
		while ( (node = strsep(&token, delim_link)) != NULL  ) {
			dbgprint("node = %s\n", node);	
			if (flag % 2) {
				snprintf(link, sizeof(link), "dev/block/by-name/%s", node);		
			} else {
				snprintf(partition, sizeof(partition), "dev/block/%s", node);		
				make_device(partition, 199, 29);
				creat_link(partition, link);
			}

			flag += 1;
		}
	}

	return 0;
}
