


vpath %.h . ./glib  ./spice-common ./win32/ ./spice-common/common /usr/include/gstreamer-0.10/
vpath %.c . ./glib ./spice-common/common ./win32/ ./spice-common/ 
vpath %.o . ./glib ./spice-common/common ./win32/

APPS=libspice_without_gtk.a
CC=gcc

#coroutine_ucontext.o
OBJS:=bio-gio.o glib-compat.o spice-audio.o spice-util.o spice-option.o spice-client.o spice-session.o spice-channel.o gio-coroutine.o channel-base.o channel-webdav.o channel-cursor.o channel-display.o channel-display-mjpeg.o channel-display-h264.o channel-inputs.o channel-main.o channel-playback.o channel-port.o channel-record.o channel-smartcard.o channel-usbredir.o smartcard-manager.o spice-uri.o usb-device-manager.o usbutil.o vmcstream.o decode-glz.o decode-jpeg.o decode-zlib.o client_sw_canvas.o wocky-http-proxy.o spice-pulse.o continuation.o  spice-glib-enums.o spice-marshal.o h264_decoder.o coroutine_gthread.o

CFLAGS = -std=gnu99

ifeq ($(SPICE_DEBUG), y)
	CFLAGS += -g
else
	CFLAGS += -O2
endif


DEFINE_FLAG=
#-DG_OS_WIN32 -DSW_CANVAS_CACHE

INC_PATH= -I../ \
			 -I../logger \
			 -I../spice-common \
			 -I../spice-common/spice-protocol/ \
			 -I/usr/include/gstreamer-0.10/gst \
			 -I/usr/include/gstreamer-0.10/ \
			 -I/usr/include/glib-2.0/\
			 -I/usr/lib/i386-linux-gnu/glib-2.0/include/ \
			 -I/usr/include/pixman-1/ \
			 -I/usr/include/soprano/ \
			 -I/usr/include/libxml2/ \
			 -I/usr/include/libusb-1.0/ \
			 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include/ \
			 -I/home/lichenxiang/opt/include

LIB_PATH=-L/lib \
			-L../spice-common/common \
			-L/home/lichenxiang/opt/include

DEFS = -DHAVE_CONFIG_H -DUSB_IDS=\""$(USB_IDS)"\"
USB_IDS = /var/lib/usbutils/usb.ids

LIBS:=-L/usr/local/lib /usr/local/lib/libcelt051.so -lm -lgio-2.0 -lgobject-2.0 -ljpeg -lz -lpixman-1 -lssl -lcrypto -lpulse-mainloop-glib -lpulse -lglib-2.0 -lsasl2 /usr/local/lib/libusbredirhost.so -lusb-1.0 /usr/local/lib/libusbredirparser.so \
	-L/home/rock/Proj/ffmpeg/lib -lavdevice -lavfilter -lpostproc -lavformat -lavcodec -lva-x11 -lva -lxcb-shm -lxcb-xfixes -lxcb-render -lxcb-shape -lxcb -lXau -lXdmcp -lpulse-simple -lpulse -lX11 -lXext -lstdc++ -lrt -lx264 -ldl -lpthread -lfontconfig -lexpat -lfreetype -lpng12 -lz -lswresample -lswscale -lavutil -lm

#LIBS=../spice-common/common/libspice-common-client.so -lglib-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lgobject-2.0 -lglib-2.0 -ljpeg -lz -lpixman-1 -lssl -lcrypto -lpulse-mainloop-glib -lpulse -lglib-2.0 -lsasl2 -L/usr/local/lib -lusb-1.0 -lusbredirhost -lusbredirparser

.PHONY:clean all


all:$(APPS)


$(APPS):$(OBJS)	
	ar rcs $@ $(OBJS)
	#ar rcs -o $@ $(OBJS) $(CFLAGS) $(LIB_PATH) $(INC_PATH) $(DEFINE_FLAG) $(LIBS) `pkg-config --cflags --libs glib-2.0` 

%.o:%.c
	$(CC) $(DEFS) -static -c $< $(CFLAGS) $(CPPFLAGS)  -o $@ -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include  -lglib-2.0 $(INC_PATH) 

clean:	
	$(RM) $(OBJS)  $(APPS) *.o libspice_without_gtk.so
