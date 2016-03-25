#include "usbconfig.h"

#define USBFILE "./usb.ini"

const QString usbonfig::CLASS_CODE = "class_code";
const QString usbonfig::USB_VID = "vid";
const QString usbonfig::USB_PID = "pid";
const QString usbonfig::USB_VENDOR = "vendor_name";
const QString usbonfig::USB_PRODUCT = "product_name";
const QString usbonfig::USB_ENABLE = "usb_enable";
	

usbonfig::usbonfig()
{
	m_usb = new usbutils(); 
	
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
			tmp.class_code=usb_file->value(usbonfig::CLASS_CODE).toInt();
			tmp.vid=usb_file->value(usbonfig::USB_VID).toInt();
			tmp.pid=usb_file->value(usbonfig::USB_PID).toInt();
			tmp.enable=usb_file->value(usbonfig::USB_ENABLE).toInt();
			QString vendor_str=usb_file->value(usbonfig::USB_VENDOR).toString();
			strcpy(tmp.vendor_name,vendor_str.toLatin1().data());
			QString product_str=usb_file->value(usbonfig::USB_PRODUCT).toString();
			strcpy(tmp.product_name,product_str.toLatin1().data());			
			usb_file->endGroup();
			printf("[%s] [%s] [0x%02x_0x%04x_0x%04x] [%d]\n",tmp.vendor_name,tmp.product_name,tmp.class_code,tmp.vid,tmp.pid,tmp.enable);

			if(!usbredirfilter_verify(dev_list,tmp))
				dev_list.push_back(tmp);
 		}
	}
	delete usb_file;
}

void usbonfig::writetofile(list<UsbDeviceInfo >& dev_list)
{
	QSettings* usb_file = new QSettings(USBFILE, QSettings::IniFormat);
	set<QString> usb_exist;
	if (usb_file)
	{
		foreach(QString group, usb_file->childGroups()) 
 		{
 			usb_exist.insert(group);
		}

		list<UsbDeviceInfo>::iterator iter;
 		for(iter = dev_list.begin();iter!=dev_list.end();iter++)  
 		{  
 			if(usb_exist.find(iter->usbrule_key) != usb_exist.end() )
 			{
 				//printf("exist key %s\n",iter->usbrule_key);
 				continue;
 			}
			
			usb_file->beginGroup(iter->usbrule_key);
			usb_file->setValue(usbonfig::USB_VENDOR, iter->vendor_name);
			usb_file->setValue(usbonfig::USB_PRODUCT, iter->product_name);
			usb_file->setValue(usbonfig::CLASS_CODE, iter->class_code);
			usb_file->setValue(usbonfig::USB_VID, iter->vid);
			usb_file->setValue(usbonfig::USB_PID, iter->pid);
			usb_file->setValue(usbonfig::USB_ENABLE, 0);
			usb_file->endGroup();
			usb_file->sync();
 		}

	}

	delete usb_file;
}

void usbonfig::read_usb_list()
{
	if(!m_usb) return;

  
   m_usb->usbutils_listdevs();
   writetofile(m_usb->get_dev_list());
	
}

void usbonfig::get_usb_rule(QString& rule)
{
  list<UsbDeviceInfo > dev_list;
  readfromfile(dev_list);
  rule2string(dev_list,rule);
}



