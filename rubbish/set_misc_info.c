/*
 * Copyright (C) 2008 The Android Open Source Project
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUF_SIZE 1024
#define MD5_DIGEST_LENGTH 16
#define MISC_PARTITION "/dev/block/by-name/misc"

static char md5_XOR[MD5_DIGEST_LENGTH * 2] = {'a', '1', 'f', '6', 'y', 'g', 'd', '8',
                                              't', '2', 'c', 'h', 'r', 's', 'l', 'o',
                                              'e', 'm', 'c', '3', '5', 'p', 'b', 'n',
                                              'u', 'q', 'v', '4', 'i', '7', '9', 'd'};
/* Bootloader Message
 *
 * This structure describes the content of a block in flash
 * that is used for recovery and the bootloader to talk to
 * each other.
 *
 * The command field is updated by linux when it wants to
 * reboot into recovery or to update radio or bootloader firmware.
 * It is also updated by the bootloader when firmware update
 * is complete (to boot into recovery for any final cleanup)
 *
 * The status field is written by the bootloader after the
 * completion of an "update-radio" or "update-hboot" command.
 *
 * The recovery field is only written by linux and used
 * for the system to send a message to recovery or the
 * other way around.
 *
 * The stage field is written by packages which restart themselves
 * multiple times, so that the UI can reflect which invocation of the
 * package it is.  If the value is of the format "#/#" (eg, "1/3"),
 * the UI will add a simple indicator of that status.
 */
struct bootloader_message {
    char command[32];
    char status[32];

    // The 'recovery' field used to be 1024 bytes.  It has only ever
    // been used to store the recovery command line, so 768 bytes
    // should be plenty.  We carve off the last 256 bytes to store the
    // stage string (for multistage packages) and possible future
    // expansion.
    char recovery[1024];
    char sign[32];
    char md5_offset[2048];
    char md5_result[32];
};

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

// ------------------------------------
// for misc partitions on block devices
// ------------------------------------
static void wait_for_device(const char* fn) {
    int tries = 0;
    int ret;
    struct stat buf;

    do {
        ++tries;
        ret = stat(fn, &buf);
        if (ret) {
            printf("stat %s try %d: %s\n", fn, tries, strerror(errno));
            sleep(1);
        }
    } while (ret && tries < 10);
    if (ret) {
        printf("failed to stat %s\n", fn);
    }
}

static int get_bootloader_message(struct bootloader_message *out) {
    wait_for_device(MISC_PARTITION);
    FILE* f = fopen(MISC_PARTITION, "rb");
    if (f == NULL) {
        printf("Can't open %s\n(%s)\n", MISC_PARTITION, strerror(errno));
        return -1;
    }
    struct bootloader_message temp;
    int count = fread(&temp, sizeof(temp), 1, f);
    if (count != 1) {
        fclose(f);
        printf("Failed reading %s\n(%s)\n", MISC_PARTITION, strerror(errno));
        return -1;
    }
    if (fclose(f) != 0) {
        printf("Failed closing %s\n(%s)\n", MISC_PARTITION, strerror(errno));
        return -1;
    }
    memcpy(out, &temp, sizeof(temp));

    return 0;
}

static int set_bootloader_message(const struct bootloader_message *in) {
    wait_for_device(MISC_PARTITION);
    FILE* f = fopen(MISC_PARTITION, "wb");
    if (f == NULL) {
        printf("Can't open %s\n(%s)\n", MISC_PARTITION, strerror(errno));
        return -1;
    }
    int count = fwrite(in, sizeof(*in), 1, f);
    if (count != 1) {
        fclose(f);
        printf("Failed writing %s\n(%s)\n", MISC_PARTITION, strerror(errno));
        return -1;
    }
    fflush(f);
    fsync(fileno(f));
    if (fclose(f) != 0) {
        printf("Failed closing %s\n(%s)\n", MISC_PARTITION, strerror(errno));
        return -1;
    }

    return 0;
}

static inline int read_misc(struct bootloader_message *out) {
    return get_bootloader_message(out);
}

static int write_misc(const struct bootloader_message *in) {
    int retry = 5;
    struct bootloader_message temp;

    while (retry-- > 0) {
        if (set_bootloader_message(in)) {
            printf("set bootloader message failed!\n");
            return -1;
        }
        if (get_bootloader_message(&temp)) {
            printf("get bootloader message failed!\n");
            return -1;
        }
        if (!memcmp(in, &temp, sizeof(temp)))
            break;
    }
    if (retry < 0) {
        printf("write misc failed!\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    int i;
    char md5[MD5_DIGEST_LENGTH*2 + 1];
    char parent_name[128];
    struct bootloader_message boot;

    if (argc != 2)
        return -1;

    memset(parent_name, 0, sizeof(parent_name));
    getNameByPid(getppid(), parent_name);
    if (strstr(parent_name, "otaservice") == NULL)
        return -1;

    memset(md5, 0, MD5_DIGEST_LENGTH*2 + 1);
    strncpy(md5, argv[1], MD5_DIGEST_LENGTH*2);
    for (i = 0; i < MD5_DIGEST_LENGTH * 2; i++) {
        md5[i] ^= md5_XOR[i];
    }

    memset(&boot, 0, sizeof(boot));
    read_misc(&boot);
    strncpy(boot.sign, "recovery", sizeof(boot.sign));
    memcpy(boot.md5_result, md5, sizeof(boot.md5_result));
    write_misc(&boot);

    /*test*/
    /*
    memset(&boot, 0, sizeof(boot));
    read_misc(&boot);
    if (!strncmp(boot.sign, "recovery", strlen("recovery")))
        printf("************boot.sign has been set************\n");
    else
        printf("************boot.sign has not been set************\n");

    if (!memcmp(boot.md5_result, md5, sizeof(boot.md5_result)))
        printf("************boot.md5_result has been set************\n");
    else
        printf("************boot.md5_result has not been set************\n");
    */

    return 0;
}
