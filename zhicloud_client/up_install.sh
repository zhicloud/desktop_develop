#! /bin/bash

export DOWNLOADDIR=/home/download
export INSTALLDIR=/home/cloud_teminal

function install_main() 
{
   mkdir $DOWNLOADDIR
   mkdir $INSTALLDIR
   
   echo "begin to install ..."
   echo $DOWNLOADDIR/upgrade.tgz
   if [ -f "$DOWNLOADDIR/upgrade.tgz" ];then
      echo "find upgrade.tgz begin to upgrade!!!"
      if [ -d "$INSTALLDIR" ];then
		 cd /home/download
        	 tar -xvf upgrade.tgz
		 cd upgrade/
		 chmod 777 start.sh
		 ./start.sh
		 cd ..
	         rm -rf upgrade/
		 rm upgrade.tgz
      fi
   fi
	ldconfig
   echo "end install ..."
}

install_main
