#ifndef USBCONFIG_H
#define USBCONFIG_H

#include "usbutil.h"
#include <QSettings>
#include <QString>  
#include <set>




class usbonfig
{
public:

	usbonfig();
	~usbonfig();

	void read_usb_list();
	void get_usb_rule(QString& rule);
	inline void setUsbAddOrRemove(bool b){ m_usb_AddOrRemove_b = b; }//by xzg
	void modify_usb_enable(bool f, int vid, int pid);
	
	
	void readfromfile(list<UsbDeviceInfo >& dev_list);
	void writetofile(list<UsbDeviceInfo >& dev_list);
private:
	void rule2string(list<UsbDeviceInfo >& dev_list,QString& rule);
	void init_default(list<UsbDeviceInfo >& dev_list);
	bool usbredirfilter_verify(list<UsbDeviceInfo >& dev_list,UsbDeviceInfo& dev);
	//void readfromfile(list<UsbDeviceInfo >& dev_list);
	//void writetofile(list<UsbDeviceInfo >& dev_list);
	
	usbutils *m_usb;
	bool m_usb_AddOrRemove_b;//by xzg
	
	const static QString CLASS_CODE;
	const static QString USB_VID;
	const static QString USB_PID;
	const static QString USB_VENDOR;
	const static QString USB_PRODUCT;
	const static QString USB_ENABLE;
		
	
};

#endif

