.PHONY: arm x86 all clean install

arm:
	make clean
	export TARGET=arm && make all install

linux:
	make clean
	export TARGET=linux && make all install

mingw:
	make clean
	export TARGET=mingw && make all install

LIB_INC := ../include
LIB_DIR := ../lib/$(TARGET)
INS_DIR := /home/zsj/zsj_nfs/nfs_test
#INS_DIR := /home/ko/target
INCLUDE = -I$(LIB_INC)

CFLAGS = -O0 -g

#LDFLAGS = -L$(LIB_DIR) -lstreaming -lutils -lasf -lspctdecode -lpthread -ljpeg
LDFLAGS = -L$(LIB_DIR) -lstreaming -lutils -lspctdecode -lpthread -ljpeg

DEMO_SAVEFILE = stream_savefile
DEMO_DISP=stream_display
DEMO_SERVER=stream_server
DEMO_DECODE=demo_decode
DEMO_ASFREAD = demo_filereader
DEMO_ASFDISP = demo_filedisp

ifeq ($(TARGET), arm)
CFLAGS += -static
LDFLAGS += -lm -lz -lstdc++
CC=arm-linux-gcc
# all: makedir $(DEMO_DECODE) $(DEMO_SERVER) $(DEMO_SAVEFILE) $(DEMO_ASFREAD)
###################################################################
XML2_INC = ../include/libxml2 #XML2_INC = /usr/local/arm_linux_4.2/arm-linux/include/libxml2
                              #XML2_LIB = /usr/local/arm_linux_4.2/arm-linux/lib
INCLUDE += -I$(XML2_INC)
#LDFLAGS += -L$(XML2_LIB) -lxml2 -lpthread -lm -lz -lstdc++
LDFLAGS += -lxml2 -lpthread -lm -lgdbm     #LDFLAGS += -L$(XML2_LIB) -lxml2 -lpthread -lm
LDFLAGS += -lRfeabi422d8
#####################################################################
all: makedir $(DEMO_SERVER)
endif

ifeq ($(TARGET), linux)
SDL_INC = /usr/include/SDL				# linux SDL  include
SDL_LIB = /usr/lib						# linux SDL lib
CC=gcc
LDFLAGS += -L$(SDL_LIB) -lSDL -lm -lz -lstdc++
INCLUDE += -I$(SDL_INC)
all:makedir $(DEMO_DECODE) $(DEMO_SERVER) $(DEMO_SAVEFILE) $(DEMO_DISP) $(DEMO_ASFREAD) $(DEMO_ASFDISP) 
endif

ifeq ($(TARGET), mingw)
SDL_INC = /usr/local/i686-mingw32/i686-mingw32/SDL-1.2.13/include/SDL  # mingw SDL include
SDL_LIB = /usr/local/i686-mingw32/i686-mingw32/SDL-1.2.13/lib		# mingw SDL lib
CC=i686-mingw32-gcc
CXX=i686-mingw32-g++
LD=i686-mingw32-ld
AR=i686-mingw32-ar
STRIP=i686-mingw32-strip
CFLAGS += -DSOCKLEN_T=int
TARGET=mingw
LDFLAGS += -L$(SDL_LIB) -lSDL
INCLUDE += -I$(SDL_INC)
DEMO_SAVEFILE = demo_savefile.exe
DEMO_DISP = demo_display.exe
DEMO_SERVER =demo_server.exe
DEMO_DECODE = demo_decode.exe
DEMO_ASFREAD = demo_filereader.exe
DEMO_ASFDISP = demo_filedisp.exe
all:makedir $(DEMO_DECODE) $(DEMO_SAVEFILE) $(DEMO_DISP) $(DEMO_ASFREAD) $(DEMO_ASFDISP)
endif

makedir:
	install -d $(INS_DIR)

clean:
	-rm $(DEMO_SAVEFILE) $(DEMO_SERVER) $(DEMO_ASFREAD) $(DEMO_DISP) $(DEMO_DECODE) $(DEMO_ASFDISP)  *.o *.d *.exe -rf


#$(DEMO_SAVEFILE): asf_save.c
#	$(CC) $(CFLAGS) $(INCLUDE) -o $@  asf_save.c  $(LDFLAGS)

#$(DEMO_DISP): sdl_display.c
#	$(CC) $(CFLAGS) $(INCLUDE) -o $@  sdl_display.c  $(LDFLAGS)

$(DEMO_SERVER): ip_cam_dev.c stream_server.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@  log.c  locallink.c db.c  ip_cam_dev.c RfDemo_linux.c xml.c MYI2C_Ex.c GPIO_Ex.c device.c tcp_connect.c YUV2JPEG.c stream_server.c $(LDFLAGS)
	arm-linux-strip stream_server


$(DEMO_DECODE): decode.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@  decode.c  $(LDFLAGS)

$(DEMO_ASFREAD): asf_read.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@  asf_read.c  $(LDFLAGS)

$(DEMO_ASFDISP): asf_display.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@  asf_display.c  $(LDFLAGS)



install:
#	-cp $(DEMO_SAVEFILE) $(INS_DIR)
	-cp $(DEMO_SERVER) 	$(INS_DIR)
	cd /home/zsj/zsj_nfs/nfs_test/
	tar zcvf /mnt/hgfs/shared/DEVELOPING_VERSION/TEST_VERSION/2013070101 stream_server
#	-cp $(DEMO_DECODE) $(INS_DIR)
#	-cp $(DEMO_DISP) $(INS_DIR)
#	-cp $(DEMO_ASFREAD) $(INS_DIR)
#	-cp $(DEMO_ASFDISP) $(INS_DIR)
