/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#include <fs_mgr.h>
#include "mtdutils/mtdutils.h"
#include "mtdutils/mounts.h"
#include "roots.h"
#include "make_ext4fs.h"

static struct fstab *fstab = NULL;
static const char *root_fstab_sunxi = "fstab.sun8i";

extern struct selabel_handle *sehandle;

#if 0
recovery filesystem table
=========================
0 /system ext4 /dev/block/by-name/system 0
1 /cache ext4 /dev/block/by-name/cache 0
2 /data ext4 /dev/block/by-name/UDISK 0
3 /mnt/Reserve0 vfat /dev/block/by-name/Reserve0 0
4 auto vfat /devices/platform/sunxi-mmc.0/mmc_host 0
5 auto vfat /devices/platform/sunxi-ehci.1 0
6 auto vfat /devices/platform/sunxi-ehci.2 0
7 auto vfat /devices/platform/sunxi_hcd_host0 0
8 auto vfat /devices/platform/sunxi-ohci.1 0
9 auto vfat /devices/platform/sunxi-ohci.2 0
10 none swap /dev/block/zram0 0
======================================

#endif
void
load_volume_table()
{
	int i;
	int ret;

	fstab = fs_mgr_read_fstab(root_fstab_sunxi);
	if (!fstab) {
		dbgprint("failed to read %s\n", root_fstab_sunxi);
		return;
	}
#if 0
	ret = fs_mgr_add_entry(fstab, "/tmp", "ramdisk", "ramdisk", 0);
	if (ret < 0) {
		LOGE("failed to add /tmp entry to fstab\n");
		fs_mgr_free_fstab(fstab);
		fstab = NULL;
		return;
	}
#endif
	printf("recovery filesystem table\n");
	printf("=========================\n");
	for (i = 0; i < fstab->num_entries; ++i) {
		Volume *v = &fstab->recs[i];
		printf("  %d %s %s %s %lld %s\n",
		 i, v->mount_point, v->fs_type, v->blk_device, v->length, v->label);
	}
	printf("\n");
}

Volume *
volume_for_path(const char *path)
{
	return fs_mgr_get_entry_for_mount_point(fstab, path);
}

int
ensure_path_mounted(const char *path)
{
	Volume *v = volume_for_path(path);
	if (v == NULL) {
		LOGE("unknown volume for path [%s]\n", path);
		return -1;
	}
	if (strcmp(v->fs_type, "ramdisk") == 0) {
		// the ramdisk is always mounted.
		return 0;
	}

	int result;
	result = scan_mounted_volumes();
	if (result < 0) {
		LOGE("failed to scan mounted volumes\n");
		return -1;
	}

	const MountedVolume *mv =
	    find_mounted_volume_by_mount_point(v->mount_point);
	if (mv) {
		// volume is already mounted
		return 0;
	}

	mkdir(v->mount_point, 0755);	// in case it doesn't already exist

	if (strcmp(v->fs_type, "yaffs2") == 0) {
		// mount an MTD partition as a YAFFS2 filesystem.
		mtd_scan_partitions();
		const MtdPartition *partition;
		partition = mtd_find_partition_by_name(v->blk_device);
		if (partition == NULL) {
			LOGE("failed to find \"%s\" partition to mount at \"%s\"\n", v->blk_device, v->mount_point);
			return -1;
		}
		return mtd_mount_partition(partition, v->mount_point,
					   v->fs_type, 0);
	} else if (strcmp(v->fs_type, "ext4") == 0
		   || strcmp(v->fs_type, "vfat") == 0) {
		result =
		    mount(v->blk_device, v->mount_point, v->fs_type,
			  MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
		if (result == 0)
			return 0;

		LOGE("failed to mount %s (%s)\n", v->mount_point,
		     strerror(errno));
		return -1;
	}

	LOGE("unknown fs_type \"%s\" for %s\n", v->fs_type, v->mount_point);
	return -1;
}

int
ensure_path_unmounted(const char *path)
{
	Volume *v = volume_for_path(path);
	if (v == NULL) {
		LOGE("unknown volume for path [%s]\n", path);
		return -1;
	}
	if (strcmp(v->fs_type, "ramdisk") == 0) {
		// the ramdisk is always mounted; you can't unmount it.
		return -1;
	}

	int result;
	result = scan_mounted_volumes();
	if (result < 0) {
		LOGE("failed to scan mounted volumes\n");
		return -1;
	}

	const MountedVolume *mv =
	    find_mounted_volume_by_mount_point(v->mount_point);
	if (mv == NULL) {
		// volume is already unmounted
		return 0;
	}

	return unmount_mounted_volume(mv);
}

int
format_volume(const char *volume)
{
	Volume *v = volume_for_path(volume);
	if (v == NULL) {
		LOGE("unknown volume \"%s\"\n", volume);
		return -1;
	}
	if (strcmp(v->fs_type, "ramdisk") == 0) {
		// you can't format the ramdisk.
		LOGE("can't format_volume \"%s\"", volume);
		return -1;
	}
	if (strcmp(v->mount_point, volume) != 0) {
		LOGE("can't give path \"%s\" to format_volume\n", volume);
		return -1;
	}

	if (ensure_path_unmounted(volume) != 0) {
		LOGE("format_volume failed to unmount \"%s\"\n",
		     v->mount_point);
		return -1;
	}

	if (strcmp(v->fs_type, "yaffs2") == 0 || strcmp(v->fs_type, "mtd") == 0) {
		mtd_scan_partitions();
		const MtdPartition *partition =
		    mtd_find_partition_by_name(v->blk_device);
		if (partition == NULL) {
			LOGE("format_volume: no MTD partition \"%s\"\n",
			     v->blk_device);
			return -1;
		}

		MtdWriteContext *write = mtd_write_partition(partition);
		if (write == NULL) {
			LOGW("format_volume: can't open MTD \"%s\"\n",
			     v->blk_device);
			return -1;
		} else if (mtd_erase_blocks(write, -1) == (off_t) - 1) {
			LOGW("format_volume: can't erase MTD \"%s\"\n",
			     v->blk_device);
			mtd_write_close(write);
			return -1;
		} else if (mtd_write_close(write)) {
			LOGW("format_volume: can't close MTD \"%s\"\n",
			     v->blk_device);
			return -1;
		}
		return 0;
	}

	if (strcmp(v->fs_type, "ext4") == 0) {
		int result =
		    make_ext4fs(v->blk_device, v->length, volume, sehandle);
		if (result != 0) {
			LOGE("format_volume: make_extf4fs failed on %s\n",
			     v->blk_device);
			return -1;
		}
		return 0;
	}

	if (strcmp(v->fs_type, "vfat") == 0) {
		int result = make_ext4fs(v->blk_device, v->length, volume, NULL);	//not real format it setupfs will format it
		if (result != 0) {
			LOGE("format_volume: new_msdos failed on %s\n",
			     v->blk_device);
			return -1;
		}
		return 0;
	}
	LOGE("format_volume: fs_type \"%s\" unsupported\n", v->fs_type);
	return -1;
}

int
setup_install_mounts()
{
	if (fstab == NULL) {
		LOGE("can't set up install mounts: no fstab loaded\n");
		return -1;
	}
	for (int i = 0; i < fstab->num_entries; ++i) {
		Volume *v = fstab->recs + i;

		if (strcmp(v->mount_point, "/tmp") == 0 ||
		    strcmp(v->mount_point, "/cache") == 0) {
			if (ensure_path_mounted(v->mount_point) != 0)
				return -1;

		} else {
			if (ensure_path_unmounted(v->mount_point) != 0)
				return -1;
		}
	}
	return 0;
}
