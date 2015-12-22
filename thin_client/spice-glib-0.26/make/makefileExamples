.PHONY:clean all


LIB_PATH=-L../glib \
			-L../spice-common/common \
			-L../win32\
			-L/usr/local/lib/ \
			-L/usr/lib/ \
			-L/home/lichenxiang/opt/lib


CFLAGS = -std=gnu99

ifeq ($(SPICE_DEBUG), y)
	CFLAGS += -g 
else
	CFLAGS += -O2
endif


INC_PATH=-I../glib \
			-I../spice-common/spice-protocol\
			-I../spice-common\
			-I../win32 \
			-I../logger/ \
			-I/home/lichenxiang/opt/include 


SRC=test_bmp.c ../win32/create_bmp.c

OBJS=test_bmp.o 

#spice-grabsequence.h spice-grabsequence.c 
LIBS=../win32/libspice_glib.a ../glib/libspice_without_gtk.a ../spice-common/common/libspice-common-client.a -lglib-2.0 -lgio-2.0 -lgmodule-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lgobject-2.0 \
	  -lpixman-1 -lssl -lcrypto -lgstbase-0.10 -lgstreamer-0.10 \
	  -lgmodule-2.0 -lgthread-2.0 -lxml2 -lspice-common-client \
	  -lspice_without_gtk  -lusb-1.0 -lusbredirhost -lusbredirparser -lspice_glib -lcelt051 \
	  -lcelt051 -lm -ljpeg -lz -lpixman-1 -lcrypto -lpulse-mainloop-glib -lpulse -lsasl2 -lusbredirhost -lusbredirparser \
	  -pthread -L/home/rock/Proj/ffmpeg/lib -lavdevice -lavfilter -lpostproc -lavformat -lavcodec -lva-x11 -lva -lxcb-shm -lxcb-xfixes -lxcb-render -lxcb-shape -lxcb -lXau -lXdmcp -lpulse-simple -lpulse -lX11 -lXext -lstdc++ -lrt -lx264 -ldl -lpthread -lfontconfig -lexpat -lfreetype -lpng12 -lz -lswresample -lswscale -lavutil -lm -lX11
all:test_bmp


test_bmp:$(SRC)
	$(CC) -o test_bmp $(CFLAGS) $(SRC) $(INC_PATH) $(LIB_PATH) $(LIBS)  




clean:
	rm -rf test_bmp *.o
