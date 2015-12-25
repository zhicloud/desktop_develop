vpath %.h ./glib  ./spice-common ./spice-common/common
vpath %.c ./glib ./spice-common/common ./spice-common/ 
vpath %.o ./glib ./spice-common/common 

.PHONY:clean all

LIB_PATH=-L../glib \
			-L../spice-common/common
CFLAGS = -std=gnu99 -DSPICE_GTK_LOCALEDIR=\""${SPICE_GTK_LOCALEDIR}"\"

ifeq ($(SPICE_DEBUG), y)
	CFLAGS += -g
else
	CFLAGS += -O2
endif


INC_PATH=-I../glib \
			-I../spice-common/spice-protocol\
			-I../spice-common \
			-I../logger/ \
			-I/usr/include/libxml2/ \
			`pkg-config --cflags gobject-2.0`
OBJS= win32-spicy.o spice-cmdline.o spice-widget-enums.o win32_service.o virt-viewer-file.o virt-viewer-util.o \
		win32_io.o create_bmp.o win32_spice_interface.o win32-spice-widget.o spice-cmdline.o win32_service.o virt-viewer-file.o virt-viewer-util.o win32_io.o create_bmp.o win32_spice_interface.o  
CC=gcc

APP=libspice_glib.a
all:$(APP) 

$(APP):$(OBJS)
	ar rcs $@ $(OBJS)

%.o:%.c
	$(CC) -static -c $^ -o $@ $(INC_PATH) $(CFLAGS)

clean:
	rm -rf $(APP) $(OBJS)
