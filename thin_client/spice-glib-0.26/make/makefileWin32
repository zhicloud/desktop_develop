vpath %.h ./glib  ./spice-common ./spice-common/common
vpath %.c ./glib ./spice-common/common ./spice-common/ 
vpath %.o ./glib ./spice-common/common 

.PHONY:clean all

LIB_PATH=-L../glib \
			-L../spice-common/common


LIB_DEFINE_FLAG=

SPICE_GTK_LOCALEDIR = /usr/local/share/locale

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
			-I/usr/include/glib-2.0/ \
			-I/usr/lib/i386-linux-gnu/glib-2.0/include/ \
			-I/usr/include/libxml2/ \
			-I/usr/lib/arm-linux-gnueabihf/glib-2.0/include/

SRC=win32-spicy.c win32-spicy.h win32-spice-widget.h win32-spice-widget.c win32-spice-widget-priv.h config.h \
	 spice-gtk-session.h spice-common.h spice-cmdline.c spice-cmdline.h spice-widget-enums.c spice-widget-enums.h \
	 win32_service.c win32_service.h virt-viewer-file.c virt-viewer-file.h virt-viewer-util.c virt-viewer-util.h \
	 win32_io.c win32_io.h create_bmp.c create_bmp.h win32_spice_interface.c win32_spice_interface.h \
	 win32_spice_interface_priv.h win32_spice_interface_types.h

OBJS= win32-spicy.o spice-cmdline.o spice-widget-enums.o win32_service.o virt-viewer-file.o virt-viewer-util.o \
		win32_io.o create_bmp.o win32_spice_interface.o win32-spice-widget.o spice-cmdline.o win32_service.o virt-viewer-file.o virt-viewer-util.o win32_io.o create_bmp.o win32_spice_interface.o  

#spice-grabsequence.h spice-grabsequence.c 
LIBS=-lglib-2.0 -lgio-2.0 -lgmodule-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lgobject-2.0 -lglib-2.0 \
	  -lpixman-1 -lssl -lcrypto -lgstbase-0.10 -lgstreamer-0.10 \
	  -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lglib-2.0 -lxml2 -lspice-common-client \
	  -lspice_without_gtk  -lusb-1.0 -lusbredirhost -lusbredirparser
CC=gcc

APP=libspice_glib.a
all:$(APP) 

$(APP):$(OBJS)
	ar rcs $@ $(OBJS)
#$(CC) -shared -fpic -o $@ $(CFLAGS)  $(INC_PATH)  $(LIB_PATH) $(INC_PATH) $(LIB_DEFINE_FLAG) $^ \
		$(LIBS) -W

%.o:%.c
	$(CC) -static -c $^ -o $@ $(INC_PATH) $(CFLAGS)


clean:
	rm -rf  *.o libspice_glib.a
