vpath %.h . ./glib  ./spice-common ./win32/ ./spice-common/common /usr/include/gstreamer-0.10/
vpath %.c . ./glib ./spice-common/common ./win32/ ./spice-common/ 
vpath %.o . ./glib ./spice-common/common ./win32/

APPS=libspice_without_gtk.a
CC=gcc
OBJS:=bio-gio.o glib-compat.o spice-audio.o spice-util.o spice-option.o spice-client.o spice-session.o spice-channel.o gio-coroutine.o channel-base.o channel-webdav.o channel-cursor.o channel-display.o channel-display-mjpeg.o channel-display-h264.o  channel-inputs.o channel-main.o channel-playback.o channel-port.o channel-record.o channel-smartcard.o channel-usbredir.o smartcard-manager.o spice-uri.o usb-device-manager.o usbutil.o vmcstream.o decode-glz.o decode-jpeg.o decode-zlib.o client_sw_canvas.o wocky-http-proxy.o spice-pulse.o continuation.o coroutine_ucontext.o spice-glib-enums.o spice-marshal.o spice-gstaudio.o h264_decoder.o
CFLAGS = -std=gnu99 

ifeq ($(SPICE_DEBUG), y)
	CFLAGS += -g
else
	CFLAGS += -O2
endif

INC_PATH= -I../ \
			 -I../logger \
			 -I../spice-common \
			 -I../spice-common/spice-protocol/ \
			 -I/usr/include/gstreamer-0.10/gst \
			 -I/usr/include/gstreamer-0.10/ \
			 -I/usr/include/pixman-1/ \
			 -I/usr/include/soprano/ \
			 -I/usr/include/libxml2/ \
			 -I/usr/include/libusb-1.0/ \
			 `pkg-config --cflags gobject-2.0`

DEFS = -DHAVE_CONFIG_H -DUSB_IDS=\""$(USB_IDS)"\"
USB_IDS = /var/lib/usbutils/usb.ids

.PHONY:clean all


all:$(APPS)


$(APPS):$(OBJS)	
	ar rcs $@ $(OBJS)

%.o:%.c
	$(CC) $(DEFS) -static -c $< $(CFLAGS) $(CPPFLAGS)  -o $@  $(INC_PATH) 

clean:	
	$(RM) $(OBJS)  $(APPS) *.o
