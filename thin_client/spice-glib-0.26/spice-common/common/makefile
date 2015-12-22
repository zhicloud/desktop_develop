



#vpath %.h ./glib  ./spice-common ./win32/ ./spice-common/common
#vpath %.c ./glib ./spice-common/common ./win32/ ./spice-common/ 
#vpath %.o ./glib ./spice-common/common ./win32/

#APPS=libspice-common-client.a
COMMON=libspice-common.so
COMMONCLIENT=libspice-common-client.a
APPS=$(COMMONCLIENT)

#OBJS=generated_client_demarshallers.o generated_client_demarshallers1.o generated_client_marshallers.o generated_client_marshallers1.o backtrace.o canvas_utils.o lines.o log.o lz.o marshaller.o mem.o pixman_utils.o quic.o region.lo rop3.o snd_codec.o ssl_verify.o

OBJS=generated_server_marshallers.o   mem.o backtrace.o generated_server_demarshallers.o \
	  lz.o  log.o rop3.o pixman_utils.o lines.o glc.o  \
	  generated_client_marshallers.o generated_client_demarshallers1.o quic.o \
	  ssl_verify.o canvas_utils.o  generated_client_demarshallers.o region.o marshaller.o generated_client_marshallers1.o snd_codec.o

#OBJS=$(patsubst %.c,%.o,$(wildcard *.c))
#OBJS+=$(patsubst ./glib/%.c,./glib/%.o,$(wildcard ./glib/*.c))
#OBJS+=$(patsubst ./spice-common/common/%.c,./spice-common/common/%.o,$(wildcard ./spice-common/common/*.c))
#OBJS+=$(patsubst ./glib/%.c,./win32/%.o,$(wildcard ./win32/*.c))

CFLAGS = -std=c99 
LIBADD=-L/usr/local/lib -lcelt051 -lm
LDFLAGS=-shared -fpic
CC=gcc
AR=ar cr
DEFS = -DHAVE_CONFIG_H

ifeq ($(SPICE_DEBUG), y)
	CFLAGS += -g
else
	CFLAGS += -O2
endif


DEFINE_FLAG= 
#-DG_OS_WIN32 -DSW_CANVAS_CACHE

INC_PATH=-I.. -I../spice-protocol/ -I. -I/usr/include/pixman-1/ 

LIB_PATH=


#OBJS+=$(patsubst %.c, %.o, .)

LIBS= -lglib-2.0 -lgio-2.0 -lgmodule-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lgobject-2.0 -lglib-2.0 \
		-lpixman-1 -lssl -lcrypto -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lglib-2.0 -lxml2 -lopus -lcelt051 -lm \
		-lGLESv2 -lpixman-1 -ljpeg
.PHONY:clean all


all:$(APPS)


$(COMMONCLIENT):$(OBJS)
	ar rcs $@  $^
	#$(CC) -o $@ $(LDFLAGS) $(LIBADD) $(CFLAGS) $(INCLUDES)  $(LIB_PATH) $(DEFINE_FLAG) $^ $(LIBS) $(DEFS)
	#/bin/sh ../libtool --silent --tag=CC   --mode=link i686-w64-mingw32-gcc -std=gnu99  -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4   -o libspice-common-client.a $(OBJS)   

%.o:%.c
	$(CC) $(CFLAGS) -static -c $^ -o $@  $(INCLUDES) $(LIBS) $(LIB_PATH) $(INC_PATH) $(DEFINE_FLAG) $(DEFS)


clean:	
	$(RM) $(OBJS)  $(APPS) *.o $(APPS)
