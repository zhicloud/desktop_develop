SUBDIRS :=  ./common  


.PHONY : all
	
all : subdirs

subdirs :
	#for n in $(SUBDIRS); do echo $$n; done
	for n in $(SUBDIRS); do  $(MAKE) -C $$n ; done

clean :
	    for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done
