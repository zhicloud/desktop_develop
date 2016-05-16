#include "usbconfig.h"
#include <QDebug>


#define USBFILE "/home/cloud_teminal/usb.ini"

const QString usbonfig::CLASS_CODE = "class_code";
const QString usbonfig::USB_VID = "vid";
const QString usbonfig::USB_PID = "pid";
const QString usbonfig::USB_VENDOR = "vendor_name";
const QString usbonfig::USB_PRODUCT = "product_name";
const QString usbonfig::USB_ENABLE = "usb_enable";
	

usbonfig::usbonfig()
{
	m_usb = new usbutils(); 
	
	m_usb_AddOrRemove_b = true;//by xzg
	if(m_usb)
	 m_usb->usbutils_init();
}


usbonfig::~usbonfig()
{
	if(m_usb){
	  delete m_usb;
	  m_usb = NULL;
	}
}


void usbonfig::rule2string(list<UsbDeviceInfo >& dev_list,QString& rule)
{
	list<UsbDeviceInfo>::iterator iter;
	
 	for(iter = dev_list.begin();iter!=dev_list.end();iter++)  
 	{
 		char tmp[50];
		if(iter->class_code == -1)
			sprintf(tmp,"%d",iter->class_code);
		else
			sprintf(tmp,"0x%02x",iter->class_code);

		if(iter->vid == -1)
			sprintf(tmp,"%s,%d",tmp,iter->vid);
		else
			sprintf(tmp,"%s,0x%02x",tmp,iter->vid);

		if(iter->pid == -1)
			sprintf(tmp,"%s,%d,-1",tmp,iter->pid);
		else
			sprintf(tmp,"%s,0x%02x,-1",tmp,iter->pid);
		
		sprintf(tmp,"%s,%d",tmp,iter->enable);
		
		
		if(rule.length() > 0)
		  rule = rule +"|" + tmp;
		else
		  rule = tmp;
 	}

	rule += "|-1,-1,-1,-1,0";
}


void usbonfig::init_default(list<UsbDeviceInfo >& dev_list)
{
	//0x00ff,-1,-1,-1,1|-1,0x08e2,0x0008,-1,1|-1,7104,32787,-1,1|0x07,-1,-1,-1,1|0x08,-1,-1,-1,1|0x02,-1,-1,-1,1|-1,-1,-1,-1,0
	UsbDeviceInfo tmp;
	memset(&tmp,0,sizeof(UsbDeviceInfo));

	tmp.class_code = 0x07;
	tmp.vid = -1; 
	tmp.pid = -1;
	tmp.enable = true;
	dev_list.push_back(tmp);

	tmp.class_code = 0x08;
	tmp.vid = -1; 
	tmp.pid = -1;
	tmp.enable = true;
	dev_list.push_back(tmp);
        
	tmp.class_code = 0x02;
	tmp.vid = -1; 
	tmp.pid = -1;
	tmp.enable = true;
	dev_list.push_back(tmp);

}

bool usbonfig::usbredirfilter_verify(list<UsbDeviceInfo >& dev_list,UsbDeviceInfo& dev)
{	
    list<UsbDeviceInfo>::iterator iter;
   //qDebug() << "AAAAAAAAAAAAAA::::" << dev.product_name;
    for(iter = dev_list.begin();iter!=dev_list.end();iter++)  
    {
 	if ((iter->class_code == -1 ||
                iter->class_code == dev.class_code) &&
            (iter->vid == -1 ||
                iter->vid == dev.vid) &&
            (iter->pid == -1 ||
                iter->pid == dev.pid)) {
            /* Found a match ! */
            return iter->enable ? true : false;
        }
    }
    return false;
}

void usbonfig::readfromfile(list<UsbDeviceInfo >& dev_list)
{
	init_default(dev_list);
	
	QSettings* usb_file = new QSettings(USBFILE, QSettings::IniFormat);
	set<QString> usb_exist;
	if (usb_file)
	{
		foreach(QString group, usb_file->childGroups()) 
 		{
 			UsbDeviceInfo tmp;
			memset(&tmp,0,sizeof(UsbDeviceInfo));
 			usb_file->beginGroup(group);
			QString keyhead(group);
			keyhead.append("/");
			strcpy(tmp.usbrule_key, group.toLatin1().data());
			tmp.class_code=usb_file->value(keyhead + QString(usbonfig::CLASS_CODE)).toInt();
			tmp.vid=usb_file->value(keyhead + QString(usbonfig::USB_VID)).toInt();
			tmp.pid=usb_file->value(keyhead + QString(usbonfig::USB_PID)).toInt();
			tmp.enable=usb_file->value(keyhead + QString(usbonfig::USB_ENABLE)).toInt();
			QString vendor_str=usb_file->value(keyhead + QString(usbonfig::USB_VENDOR)).toString();
			strcpy(tmp.vendor_name,vendor_str.toLatin1().data());
			QString product_str=usb_file->value(keyhead + QString(usbonfig::USB_PRODUCT)).toString();
			strcpy(tmp.product_name,product_str.toLatin1().data());			
			usb_file->endGroup();
			//printf("[%s] [%s] [0x%02x_0x%04x_0x%04x] [%d]\n",tmp.vendor_name,tmp.product_name,tmp.class_code,tmp.vid,tmp.pid,tmp.enable);

		//	if(!usbredirfilter_verify(dev_list,tmp)){
				dev_list.push_back(tmp);
		//	}
 		}
	}
	delete usb_file;
}
void usbonfig::modify_usb_enable(bool f, int vid, int pid)
{
	QSettings *usb_file = new QSettings(USBFILE, QSettings::IniFormat);
	foreach(QString group, usb_file->childGroups()){
		usb_file->beginGroup(group);
		QString keyhead(group);
		keyhead.append("/");
		int cur_vid = usb_file->value(keyhead + QString(usbonfig::USB_VID)).toInt();
		int cur_pid = usb_file->value(keyhead + QString(usbonfig::USB_PID)).toInt();
		if(vid == cur_vid && pid == cur_pid)
			usb_file->setValue(keyhead + QString(usbonfig::USB_ENABLE), f);
		usb_file->endGroup();
	}

}
void usbonfig::writetofile(list<UsbDeviceInfo >& dev_list)
{
	QSettings* usb_file = new QSettings(USBFILE, QSettings::IniFormat);
	set<QString> usb_exist;
	if (usb_file)
	{
		//if(!m_usb_AddOrRemove_b)
			//usb_file->clear();//by xzg
		foreach(QString group, usb_file->childGroups()) 
 		{
 			usb_exist.insert(group);
		}

		list<UsbDeviceInfo>::iterator iter;
		if(m_usb_AddOrRemove_b){

 			for(iter = dev_list.begin();iter!=dev_list.end();iter++)  
 			{  
 				if(usb_exist.find(iter->usbrule_key) != usb_exist.end() || strcmp(iter->usbrule_key, "")==0)
 				{
 					//printf("exist key %s\n",iter->usbrule_key);
					continue;
 				}else{
					usb_file->beginGroup(iter->usbrule_key);
					QString keyhead(iter->usbrule_key);
					keyhead.append("/");
					usb_file->setValue(keyhead + QString(usbonfig::USB_VENDOR), iter->vendor_name);
					usb_file->setValue(keyhead + QString(usbonfig::USB_PRODUCT), iter->product_name);
					usb_file->setValue(keyhead + QString(usbonfig::CLASS_CODE), iter->class_code);
					usb_file->setValue(keyhead + QString(usbonfig::USB_VID), iter->vid);
					usb_file->setValue(keyhead + QString(usbonfig::USB_PID), iter->pid);
					usb_file->setValue(keyhead + QString(usbonfig::USB_ENABLE), /*0*/iter->enable);
					usb_file->endGroup();
				}
 			}
		}else{
			bool find = false;
			set<QString>::iterator its = usb_exist.begin();
			for(; its != usb_exist.end(); its++){
				find = false;
				for(iter = dev_list.begin(); iter != dev_list.end(); iter++){
					if(*its == QString(iter->usbrule_key))
						find = true;
				}
				if(!find){
					usb_file->beginGroup(*its);
					usb_file->remove("");
					usb_file->endGroup();	
				}
				//qDebug() << "HHHHHFFFFFDDDDDDD#######" << *its;
			}
		}
		usb_file->sync();
	}

	delete usb_file;
}

void usbonfig::read_usb_list()
{
	if(!m_usb) return;

  
   m_usb->usbutils_listdevs();
   //list<UsbDeviceInfo> dev_list_from_file;
   list<UsbDeviceInfo> &dev_list_from_sys = m_usb->get_dev_list();
   /*readfromfile(file_dev_list);
   if(!tmp_dev_list.empty()){
       list<UsbDeviceInfo>::iterator it_sys = dev_list_from_sys.begin();
       list<UsbDeviceInfo>::iterator it_file;// = dev_list_from_file.begin();
	   for(; it_sys != dev_list_from_sys.end(); it_sys++){
	       it_file = dev_list_from_file.begin();
		   for(; it_file != dev_list_from_file.end(); it_file++){
		       if(strcmp(it_file->usbrule_key, it_sys->usbrule_key) == 0){
			       it_sys->enable = it_file->enable;
				   break;
			   }
		   }
	   }
   }*/
   writetofile(dev_list_from_sys);
	
}

void usbonfig::get_usb_rule(QString& rule)
{
  list<UsbDeviceInfo > dev_list;
  readfromfile(dev_list);
  rule2string(dev_list,rule);
}



