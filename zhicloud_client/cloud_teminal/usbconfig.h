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
	
private:
	void rule2string(list<UsbDeviceInfo >& dev_list,QString& rule);
	void init_default(list<UsbDeviceInfo >& dev_list);
	bool usbredirfilter_verify(list<UsbDeviceInfo >& dev_list,UsbDeviceInfo& dev);
	void readfromfile(list<UsbDeviceInfo >& dev_list);
	void writetofile(list<UsbDeviceInfo >& dev_list);
	
	usbutils *m_usb;

	
	const static QString CLASS_CODE;
	const static QString USB_VID;
	const static QString USB_PID;
	const static QString USB_VENDOR;
	const static QString USB_PRODUCT;
	const static QString USB_ENABLE;
		
	
};

#endif

