#ifndef MULTI_DEVICE_H_
#define MULTI_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

int do_md5(const char *path, char *outMD5);
int updateFromMutliDevice(const char *opt , char *outpath);

#ifdef __cplusplus
}
#endif

#endif  // USB_H_
