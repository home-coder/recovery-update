static int
copy_databk_to_data(){
	// if it is secure boot , return
	if (secureBootFlag>-1)
		return 0;
	// if it is multi-users , return
	if (volume_for_path("/sdcard")==NULL)
		return 0;
	printf("begin copy databk to data\n");
	char *argv_execv[] = {"data_resume.sh", NULL};
	ensure_path_mounted("/data");
	ensure_path_unmounted("/system");
	int re = mount("/dev/block/by-name/system","/system","ext4",MS_RDONLY,"");
	if (re<0)
		return -1;
	pid_t pid =fork();
	if(pid==0){
		execv("/system/bin/data_resume.sh",argv_execv);
		_exit(-1);
	}
	int status;
	waitpid(pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		printf("Error (Status %d),fail to resume data\n", WEXITSTATUS(status));
		ensure_path_unmounted("/data");
		ensure_path_unmounted("/system");
		return -1;
	}
	printf("copy databk to data succeed\n");
	ensure_path_unmounted("/data");
	ensure_path_unmounted("/system");
	return 0;
}                                                                                         
