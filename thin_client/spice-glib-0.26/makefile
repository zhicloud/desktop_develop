SUBDIRS :=  ./spice-common ./glib ./win32 ./examples 


STRIP=$(CC)-strip

.PHONY : all install strip installerpacket help 
	
all : subdirs

subdirs :
ifeq ($(WINDOWS), y)
	cp -a ./make/wmakefileCommon ./spice-common/common/makefile
	cp -a ./make/wmakefileGlib ./glib/makefile
	cp -a ./make/wmakefileWin32 ./win32/makefile
	cp -a ./make/wmakefileExamples ./examples/makefile
	cp -a ./make/wconfigwin32.h ./win32/config.h
	cp -a ./make/wconfigglib.h ./glib/config.h
	cp -a ./make/wconfigcommon.h ./spice-common/config.h
	cp -a ./make/wconfig.h ./config.h
	cp -a ./make/wconfigprotocol.h ./spice-common/spice-protocol/config.h
	cp -a ./make/wbitopscommon.h ./spice-common/common/bitops.h
else
ifeq ($(GST),y)
	cp -a ./make/makefileCommon ./spice-common/common/makefile
	cp -a ./make/makefileGlib_gst ./glib/makefile
	cp -a ./make/makefileWin32_gst ./win32/makefile
	cp -a ./make/makefileExamples_gst ./examples/makefile
	cp -a ./make/configwin32_gst.h ./win32/config.h
	cp -a ./make/configglib_gst.h  ./glib/config.h
	cp -a ./make/configcommon.h ./spice-common/config.h
	cp -a ./make/config_gst.h ./config.h
	cp -a ./make/configprotocol.h ./spice-common/spice-protocol/config.h
	cp -a ./make/bitopscommon.h ./spice-common/common/bitops.h
else
	##default configure
	cp -a ./make/makefileCommon ./spice-common/common/makefile
	cp -a ./make/makefileGlib ./glib/makefile
	cp -a ./make/makefileWin32 ./win32/makefile
	cp -a ./make/makefileExamples ./examples/makefile
	cp -a ./make/configwin32.h ./win32/config.h
	cp -a ./make/configglib.h ./glib/config.h
	cp -a ./make/configcommon.h ./spice-common/config.h
	cp -a ./make/config.h ./config.h
	cp -a ./make/configprotocol.h ./spice-common/spice-protocol/config.h
	cp -a ./make/bitopscommon.h ./spice-common/common/bitops.h
endif

endif
	#for n in $(SUBDIRS); do echo $$n; done
	for n in $(SUBDIRS); do  $(MAKE) -C $$n ; done

clean :
	    for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done
		 rm -rf ../../zhicloud_client/packet/

install:
	mkdir -p ../../zhicloud_client/packet
	mkdir -p ../../zhicloud_client/packet/lib
	mkdir -p ../../zhicloud_client/packet/bin
	mkdir -p ../../zhicloud_client/packet/include
	cp -rf ./README ../../zhicloud_client/packet/
ifeq ($(WINDOWS), y)
	cp -rf ./spice-common/common/libspice-common-client.dll ./packet/lib/
	cp -rf ./spice-common/common/libspice-common-client.def ./packet/lib/
	cp -rf ./spice-common/common/libspice-common-client.dll.a ./packet/lib/
	cp -rf ./glib/libspice_without_gtk.def ./packet/lib/
	cp -rf ./glib/libspice_without_gtk.dll ./packet/lib/
	cp -rf ./glib/libspice_without_gtk.dll.a ./packet/lib/
	cp -rf ./win32/libspice_glib.def ./packet/lib
	cp -rf ./win32/libspice_glib.dll ./packet/lib
	cp -rf ./win32/libspice_glib.dll.a ./packet/lib
	cp -rf ./examples/test_bmp.exe ./packet/bin/
	cp -rf ./win32/win32_spice_interface.h ./packet/include/
	cp -rf ./win32/win32_spice_interface_types.h ./packet/include/
else
	cp -rf ./spice-common/common/libspice-common-client.a ../../zhicloud_client/packet/lib/
	cp -rf ./glib/libspice_without_gtk.a ../../zhicloud_client/packet/lib/
	cp -rf ./win32/libspice_glib.a ../../zhicloud_client/packet/lib
	cp -rf ./examples/test_bmp ../../zhicloud_client/packet/bin/
	cp -rf ./win32/win32_spice_interface.h ../../zhicloud_client/packet/include/
	cp -rf ./win32/win32_spice_interface_types.h ../../zhicloud_client/packet/include/
endif


strip:
	$(STRIP) --strip-all ./packet/lib/libspice-common-client.dll
	$(STRIP) --strip-all ./packet/lib/libspice_without_gtk.dll
	$(STRIP) --strip-all ./packet/bin/test_bmp.exe
	$(STRIP) --strip-all ./packet/lib/libspice_glib.dll

installerpacket:
	mkdir -p ./packet/installer
	makensis ./data/spice_glib.nsis
	mv ./data/test_bmp-0.26.exe ./packet/installer

help:
	@echo "***************************************************************"
	@echo "版本创建：李陈祥"
	@echo	"make help 帮助"
	@echo "make 编译"
	@echo "make clean 清除编译"
	@echo	"make install 安装所有生成文件"
	@echo	"make strip 生成release 版本的文件"
	@echo	"make installerpacket 生成安装包"
	@echo "***************************************************************"




