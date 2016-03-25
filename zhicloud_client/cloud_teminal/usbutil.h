#ifndef USBUTIL_H
#define USBUTIL_H

#include <errno.h>
#include <stdbool.h>
#include <list>
#include <pthread.h>
using namespace std;
#ifdef __cplusplus
extern "C" {
#endif

#include <libusb.h>
#include <usbredirfilter.h>
#include "usbredirhost.h"


#ifdef __cplusplus
}
#endif

#define MAX_USB_DEVICE 100

#define VENDOR_NAME_SIZE (122 - sizeof(void *))
#define PRODUCT_NAME_SIZE 126
#define USB_RULE_SIZE 100

typedef struct UsbDeviceInfo
{
   unsigned char busnum;
   unsigned char   devaddr;
   int  vid;
   int pid;
   libusb_device *libdev;
   int class_code;
   unsigned short interface_class;
   char vendor_name[VENDOR_NAME_SIZE];
   char product_name[PRODUCT_NAME_SIZE];
   char usbrule_key[USB_RULE_SIZE];
   bool enable;
}UsbDeviceInfo;

typedef struct UsbProductInfo {
   unsigned short product_id;
   char name[PRODUCT_NAME_SIZE];
} UsbProductInfo;


typedef struct UsbVendorInfo {
   UsbProductInfo *product_info;
   int product_count;
   unsigned short vendor_id;
   char name[VENDOR_NAME_SIZE];
} UsbVendorInfo;


class usbutils
{
   public:
      usbutils();
      ~usbutils();
      int usbutils_init();
      int usbutils_init_hotplug();
      void get_usb_informations();

      bool usbutils_start_event_listening(void * args);


      const char *usbutils_strerror(enum libusb_error error_code);
      void usbutils_listdevs();
      bool usbutils_load_usbids();
      bool usbutils_parse_usbids(char *path);

      void usbutils_device_get_description(UsbDeviceInfo &device);
      unsigned char usbutils_device_get_busnum(const UsbDeviceInfo &device);
      unsigned char usbutils_device_get_devaddr(const UsbDeviceInfo &device);
      unsigned short usbutils_device_get_vid(const UsbDeviceInfo &device);
      unsigned short usbutils_device_get_pid(const UsbDeviceInfo &device);
	  bool usbutils_device_get_interfaceclass(libusb_device *dev,UsbDeviceInfo &device);
      void usbutils_get_device_strings(int bus, int address,int vendor_id, int product_id,char  *manufacturer, char  *product);
      char *usbutils_get_sysfs_attribute(int bus, int address,const char *attribute);
      void strfreev (char **str_array);
      char** strsplit (const char *string,const char *delimiter,int max_tokens);

	  list<UsbDeviceInfo >& get_dev_list();
	  void usbutils_free_dev();

      static int usbutils_hotplug_cb(libusb_context *ctx,libusb_device *device, libusb_hotplug_event  event,void *user_data);
   private:
      libusb_context *context; 
      libusb_hotplug_callback_handle hp_handle;

      list<UsbDeviceInfo > dev_list;

      pthread_t thotplug;
      pthread_mutex_t mutex;

      pthread_mutex_t usbids_load_mutex;
      int usbids_vendor_count; 
      UsbVendorInfo *usbids_vendor_info; 
      UsbProductInfo *g_product_info;
};



#endif
