# recovery-update1.了解如何创建自定义分区2.如何操作分区，包括应用层和uboot中，包括节点形式和挂载形式3.训练rubbish的编程方法和编程思路当然，大前提是将原始recovery android标准的流程搞清楚还有问题---->boot-recovery第一次写入BCB流程mtd和block，如何判断有无挂载分区，v->blk_device是怎么做 。volume分区的擦除和判断是否存在mount的使用场景UI的配置get_factory_info如何来如何备份data分区get_bootloader_message注意：---------1. 对结构体里面有指针的情况，最好还是以 b+的方式打开我们在对一个文件进行操作以前，首先，我们要清楚这个文件到底是文本文件还是二进制文件。文件文件用文本方式打开，二进制文件用二进制方式打开如果我们要操作一个二进制文件，那么我们就以二进制方式打开(理论上也可以以文件方式打开，但是如果写的二进制数据里面有45时，会转化成45,42存储，见1.这是很有可能发生的)。同时读写的时候用fread，fwrite这两个函数。如果我要操作一个文本文件，那么我们就以文本的方式打开（理论上也可以以二进制方式打开，但是不保险）。同时读写的时候用读写字符的那些函数fprintf,fscanf ,fgetc,fputc,putw,getw,fgetc,fputs.不过，最好不要在这种结构体里面放指针是最最明智的。。。2. ext4_utils  fs_mgr这两个目录是编译成库的，很多地方用到了，system里面的文件系统操作也用到了.3. ensure_path_mounted里面的挂载为什么比usb.c中的挂载少了一些东西，比如usb.c中子进程所做的事情(MOUNT_EXFAT "/sbin/mount.exfat")相关操作{		vfat 就是 fat32, 但是单个文件最大容量4G 		exfat 是微软  单个文件不限制		ext ...是Linux}完成任务----------------------------------1.练习u盘的识别挂载和文件扫描, 这个任务已经做到可以扫描到文件update.zip残留问题：1.1 分区表在那里，怎么加载，为什么紫禁城pid==0的时候做一些事情，为什么要建立紫禁城。		  1.2 分区挂载以后，创建目录，文件，链接，访问他们的方法要练习		  		  1.3 ----结构体写入文件问题，写入的指针（有一个数据域），那么下次另一台电脑取这个结构体，并解析这个指针，还能找到相应的东西？？不能吧，需要验证。		  :最后我认为还是按照数组的方式来构建结构体，因为指针写入文件不靠谱2.练习PID的各种操作，包括取到id 取到名字之类的操作 .3.完成1问题之后，向u盘挂载目录中导入一些文件和压缩包和图片之类的东西.最后要完成--------------------------1.如何模仿device自定义分区，如何在里面创建global.ini文件，如何读取并配置setenv等2.如何模仿norco创建出写MAC的方式