#ifndef SPCT6100_DEV_H
#define SPCT6100_DEV_H

#ifdef __arm__
typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#endif
#endif

#include <linux/videodev2.h>

#define V4L2_CID_REQIFRAME                (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_MODE                     (V4L2_CID_PRIVATE_BASE + 6)
#define V4L2_CID_STATUS                   (V4L2_CID_PRIVATE_BASE + 7)
#define V4L2_CID_EQUALITY                 (V4L2_CID_PRIVATE_BASE + 9)
#define V4L2_CID_EENSPEED                 (V4L2_CID_PRIVATE_BASE + 11)
#define V4L2_CID_MOSAIC                   (V4L2_CID_PRIVATE_BASE + 15)
#define V4L2_CID_MOTIONDETECT             (V4L2_CID_PRIVATE_BASE + 16)		//使能动作检测
#define V4L2_CID_OSDTIMESYNC              (V4L2_CID_PRIVATE_BASE + 17)
#define V4L2_CID_DISABLEMOSAIC            (V4L2_CID_PRIVATE_BASE + 23)
#define V4L2_CID_STOPBOARD                (V4L2_CID_PRIVATE_BASE + 25)
#define V4L2_CID_MINTERVAL                (V4L2_CID_PRIVATE_BASE + 27)		//设置动态检测时间间隔
#define V4L2_CID_MTHRESHOLD               (V4L2_CID_PRIVATE_BASE + 28)		//设置动态采集阈值
#define V4L2_CID_OSDSET                   (V4L2_CID_PRIVATE_BASE + 36)
#define V4L2_CID_WI2C                     (V4L2_CID_PRIVATE_BASE + 31)
#define V4L2_CID_RI2C                     (V4L2_CID_PRIVATE_BASE + 32)
#define V4L2_CID_SGPIOD                   (V4L2_CID_PRIVATE_BASE + 33)
#define V4L2_CID_WGPIO                    (V4L2_CID_PRIVATE_BASE + 34)
#define V4L2_CID_VLOSTEN                  (V4L2_CID_PRIVATE_BASE + 35)
#define V4L2_CID_GOPSET                   (V4L2_CID_PRIVATE_BASE + 37)
#define V4L2_CID_RGPIO                    (V4L2_CID_PRIVATE_BASE + 40)
#define V4L2_CID_WATCHEN                  (V4L2_CID_PRIVATE_BASE + 41)
#define V4L2_CID_WATCHSET                 (V4L2_CID_PRIVATE_BASE + 42)
#define V4L2_CID_AUDIO                    (V4L2_CID_PRIVATE_BASE + 45)

#define V4L2_CID_INITENV                  (V4L2_CID_PRIVATE_BASE + 1)
#define V4L2_CID_SHOWSIZE                 (V4L2_CID_PRIVATE_BASE + 10)
#define V4L2_CID_DOWNLOAD                 (V4L2_CID_PRIVATE_BASE + 18)
#define V4L2_CID_DSPSTATUS                (V4L2_CID_PRIVATE_BASE + 20)
#define V4L2_CID_UPLOAD                   (V4L2_CID_PRIVATE_BASE + 22)
#define V4L2_CID_RUNCODE                  (V4L2_CID_PRIVATE_BASE + 24)

#define V4L2_BUF_FLAG_SPCT6100_SPS        (0x01)
#define V4L2_BUF_FLAG_SPCT6100_AUDIO      (0x02)
#define V4L2_BUF_FLAG_SPCT6100_MOTION     (0x04)
#define V4L2_BUF_FLAG_SPCT6100_VLOST      (0x10)
#define V4L2_BUF_FLAG_SPCT6100_VIDEO      (0x20)

#endif // SPCT6100_DEV_H
