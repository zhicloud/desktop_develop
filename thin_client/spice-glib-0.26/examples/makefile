.PHONY:clean all


LIB_PATH=-L../glib \
			-L../spice-common/common \
			-L../win32\
			-L/usr/local/lib/ \
			-L/usr/lib/ \
			-L../ffmpeg/lib/


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
			-I../ffmpeg/include/


SRC=test_bmp.c ../win32/create_bmp.c

OBJS=test_bmp.o 

#-ldl -llzma -lbz2 -lz
#spice-grabsequence.h spice-grabsequence.c 
LIBS=-pthread \
	 -lspice_glib -lspice_without_gtk -lspice-common-client \
	 -lglib-2.0 -lgio-2.0 -lgmodule-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0 \
	 -lpixman-1 -lssl -lcrypto -lxml2 \
	 -lgstapp-0.10 -lgstaudio-0.10 -lgstbase-0.10 -lgstinterfaces-0.10 -lgstreamer-0.10 \
	 -lusb-1.0 -lusbredirhost -lusbredirparser -lcelt051 \
	 -lm -ljpeg -lz -lpulse-mainloop-glib -lpulse -lsasl2 \
	 -lavformat -lavcodec -lx264 -lpthread -lva -lswresample -lswscale -lavutil
	# -lva-x11 -lva -lxcb-shm -lxcb-xfixes -lxcb-render \
	# -lxcb-shape -lxcb -lXau -lXdmcp -lSDL -lasound \
	# -lpulse-simple -lpulse -lX11 -lXext -lcaca -lx264 -lpthread -ldl -lz -lswresample -lswscale -lavutil -lm 
	# -lavformat -lavcodec -lx264 -lpthread -lva -llzma -lbz2  -lswresample -lswscale -lavutil

all:test_bmp


test_bmp:$(SRC)
	$(CC) -o test_bmp $(CFLAGS) $(SRC) $(INC_PATH) $(LIB_PATH) $(LIBS)  




clean:
	rm -rf test_bmp *.o
