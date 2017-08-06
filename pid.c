#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>

void getNameByPid(pid_t pid, char *task_name) {
	char proc_pid_path[BUF_SIZE];
	char title[128];
	char buf[BUF_SIZE];

	sprintf(proc_pid_path, "/proc/%d/status", pid);
	FILE* fp = fopen(proc_pid_path, "r");
	if (NULL != fp) {
		while (fgets(buf, BUF_SIZE-1, fp) != NULL) {
			sscanf(buf, "%s %*s", title);
			if (!strncasecmp(title, "Name", strlen("Name"))) {
				sscanf(buf, "%*s %s", task_name);
				break;
			}
		}
		fclose(fp);
	}
}

char *basename(const char *path)
{
	register const char *s;
	register const char *p;

	p = s = path;

	while (*s) {
		if (*s++ == '/') {
			p = s;
		}
	}

	return (char *) p;
}

/* find all pid of process by name, only compare base name of pid_name
  * pid_list: caller malloc pid_t array
  * list_size: the size of pid_list
  * RETURN:
  *        < 0: error number
  *        >=0: how many pid found, pid_list will store the founded pid
  */
int get_pid_by_name(const char* process_name, pid_t pid_list[], int list_size)
{
#define  MAX_BUF_SIZE       256

    DIR *dir;
    struct dirent *next;
    int count=0;
    pid_t pid;
    FILE *fp;
    char *base_pname = NULL;
    char *base_fname = NULL;
    char cmdline[MAX_BUF_SIZE];
    char path[MAX_BUF_SIZE];

    if(process_name == NULL || pid_list == NULL)
        return -EINVAL;

    base_pname = basename(process_name);
    if(strlen(base_pname) <= 0)
        return -EINVAL;

    dir = opendir("/proc");
    if (!dir)
    {
        return -EIO;
    }
    while ((next = readdir(dir)) != NULL) {
        /* skip non-number */
        if (!isdigit(*next->d_name))
            continue;

        pid = strtol(next->d_name, NULL, 0);
        sprintf(path, "/proc/%u/cmdline", pid);
        fp = fopen(path, "r");
	if(fp == NULL)
		continue;

        memset(cmdline, 0, sizeof(cmdline));
        if(fread(cmdline, MAX_BUF_SIZE - 1, 1, fp) < 0){ 
            fclose(fp);
            continue;
        }
        fclose(fp);
        base_fname = basename(cmdline);

        if (strcmp(base_fname, base_pname) == 0 )
        {
            if(count >= list_size){
                break;
            }else{
                pid_list[count] = pid;
                count++;
            }
        }
    }
    closedir(dir) ;
    return count;
}

/* If process is existed, return true */
int is_process_exist(const char* process_name)
{
    pid_t pid;

    return (get_pid_by_name(process_name, &pid, 1) > 0);
}

#define MAX_PID_NUM 64

int getPID(const char* process){
	int ret = 0;  
	int n;  
	pid_t pid[MAX_PID_NUM];  
	ret = get_pid_by_name(process, pid, MAX_PID_NUM);  
	printf("process '%s' is existed? (%d): %c\n", process, ret, (ret > 0)?'y':'n');  
	for(n=0;n<ret;n++){  
		printf("%u\n", pid[n]);  
	}  
	return ret;
}

int killPID(const char* process){
	int ret = 0;  
	int n;  
	pid_t pid[MAX_PID_NUM];  
	char buff[512];

	ret = get_pid_by_name(process, pid, MAX_PID_NUM);  
	printf("kill process '%s' is existed? (%d): %c\n", process, ret, (ret > 0)?'y':'n');  
	for(n=0;n<ret;n++){  
		printf("%u\n", pid[n]);  
		memset(buff, 0, sizeof(buff));
		sprintf(buff, "kill %u\n", pid[n]);
		system(buff);
	}  
	return ret;
}
