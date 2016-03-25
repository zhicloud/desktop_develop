#include "usbutil.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <limits.h> 


#include "fileutil.h"

//#include "log.h"

usbutils::usbutils()
{
   context = NULL;
   hp_handle = 0;

   pthread_mutexattr_t attr;
   pthread_mutexattr_init(&attr);
   pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init(&mutex,&attr);
   pthread_mutex_init(&usbids_load_mutex,&attr);
   pthread_mutexattr_destroy(&attr);
   dev_list.clear();

   

   usbids_vendor_count = 0; 
   usbids_vendor_info = NULL; 
   g_product_info = NULL;
}

usbutils::~usbutils()
{
   libusb_exit(context);
   context = NULL;
   hp_handle = 0;

   for (list<UsbDeviceInfo>::iterator it = dev_list.begin();it != dev_list.end();it++)
   {
      if(it->libdev != NULL)
      {
         libusb_unref_device(it->libdev);
         it->libdev = NULL;
      }
   }
   dev_list.clear();

   if(usbids_vendor_info)
      free(usbids_vendor_info);
   if(g_product_info)
      free(g_product_info);

   pthread_mutex_destroy(&mutex);
   pthread_mutex_destroy(&usbids_load_mutex);
}

void usbutils::get_usb_informations()
{

}

bool usbutils::usbutils_start_event_listening(void * args)
{
   return true;

}

int usbutils::usbutils_hotplug_cb(libusb_context *ctx,libusb_device *device, libusb_hotplug_event  event,void *user_data)
{

}

char *usbutils::usbutils_get_sysfs_attribute(int bus, int address,
                                                const char *attribute)
{
    struct stat stat_buf;
    char filename[256];
    char *contents = NULL;

    snprintf(filename, sizeof(filename), "/dev/bus/usb/%03d/%03d",
             bus, address);
    if (stat(filename, &stat_buf) != 0)
        return NULL;


    snprintf(filename, sizeof(filename), "/sys/dev/char/%d:%d/%s",
             major(stat_buf.st_rdev), minor(stat_buf.st_rdev), attribute);

    if (!file_get_contents(filename, &contents, NULL))
        return NULL;

    contents[strlen(contents) - 1] = '\0';

	trim(contents);
    //printf("contents = %s %d\n",contents,strlen(contents));

    return contents;
}


void usbutils::strfreev (char **str_array)
{
   if (str_array)
   {
      int i;

      for (i = 0; str_array[i] != NULL; i++)
         free (str_array[i]);

      free (str_array);
   }
}

char** usbutils::strsplit(const char *string,const char *delimiter,int max_tokens)
{
   if (max_tokens < 1)
      max_tokens = INT_MAX;
   
   int line_size = 0;
   const char *str0 = string; 
   while((str0 = strstr(str0,delimiter)))
   {
      line_size++;
      str0 += strlen(delimiter);
   }
   line_size += 10;
   
   char **lines = (char **)malloc(sizeof(char *) * line_size);

   char * str = strdup(string);
   char * str1 = str;
   char *key_point;
   int i = 0;

   while((key_point = strsep(&str1,delimiter)) && max_tokens--)
   {
      lines[i] = strdup(key_point);
#ifdef DEBUG
      //printf("***********************\n");
      //printf("%s \n",lines[i] );
      //printf("=======================\n");
#endif 
      i++;
   }
   lines[i] = NULL;

   free(str);
   return lines;
}

bool usbutils::usbutils_parse_usbids(char *path)
{
   char *contents, *line, **lines;
   UsbProductInfo * product_info;
   int i, j, id, product_count = 0;

   usbids_vendor_count = 0;
   if (!file_get_contents(path, &contents, NULL)) {
      usbids_vendor_count = -1;
      return false;
   }

   lines = strsplit(contents, "\n", -1);

   for (i = 0; lines[i]; i++) {
      if (!isxdigit(lines[i][0]) || !isxdigit(lines[i][1]))
         continue;

      for (j = 1; lines[i + j] &&
            (lines[i + j][0] == '\t' ||
             lines[i + j][0] == '#'  ||
             lines[i + j][0] == '\0'); j++) {
         if (lines[i + j][0] == '\t' && isxdigit(lines[i + j][1]))
            product_count++;
      }
      i += j - 1;

      usbids_vendor_count++;
   }

   usbids_vendor_info = (UsbVendorInfo*)malloc(sizeof(UsbVendorInfo) * usbids_vendor_count) ;
   product_info = (UsbProductInfo*)malloc(sizeof(UsbProductInfo) * product_count);

   g_product_info = product_info;

   usbids_vendor_count = 0;
   for (i = 0; lines[i]; i++) {
      line = lines[i];

      if (!isxdigit(line[0]) || !isxdigit(line[1]))
         continue;

      id = strtoul(line, &line, 16);
      while (isspace(line[0]))
         line++;

      usbids_vendor_info[usbids_vendor_count].vendor_id = id;
      snprintf(usbids_vendor_info[usbids_vendor_count].name,
            VENDOR_NAME_SIZE, "%s", line);

      product_count = 0;
      for (j = 1; lines[i + j] &&
            (lines[i + j][0] == '\t' ||
             lines[i + j][0] == '#'  ||
             lines[i + j][0] == '\0'); j++) {
         line = lines[i + j];

         if (line[0] != '\t' || !isxdigit(line[1]))
            continue;

         id = strtoul(line + 1, &line, 16);
         while (isspace(line[0]))
            line++;
         product_info[product_count].product_id = id;
         snprintf(product_info[product_count].name,
               PRODUCT_NAME_SIZE, "%s", line);

         product_count++;
      }
      i += j - 1;

      usbids_vendor_info[usbids_vendor_count].product_count = product_count;
      usbids_vendor_info[usbids_vendor_count].product_info  = product_info;
      product_info += product_count;
      usbids_vendor_count++;
   }

   strfreev(lines);
   free(contents);

#ifdef DEBUG
   /*
   for (i = 0; i < usbids_vendor_count; i++) {
      printf("%04x  %s\n", usbids_vendor_info[i].vendor_id,
            usbids_vendor_info[i].name);
      product_info = usbids_vendor_info[i].product_info;
      for (j = 0; j < usbids_vendor_info[i].product_count; j++) {
         printf("\t%04x  %s\n", product_info[j].product_id,
                   product_info[j].name);
        }
    }
    */
#endif
    return true;
}




bool usbutils::usbutils_load_usbids()
{
    bool success = false;

    pthread_mutex_lock(&usbids_load_mutex);
    if (usbids_vendor_count) {
        success = usbids_vendor_count > 0;
        goto leave;
    }

/*#ifdef WITH_USBIDS

#ifdef DEBUG
    printf("=== %s ===\n",(char*)USB_IDS);
#endif 
    success = usbutils_parse_usbids((char*)USB_IDS);
#else
    {
        const gchar * const *dirs = g_get_system_data_dirs();
        gchar *path = NULL;
        int i;

        for (i = 0; dirs[i]; ++i) {
            path = g_build_filename(dirs[i], "hwdata", "usb.ids", NULL);
            success = spice_usbutil_parse_usbids(path);
            g_free(path);

            if (success)
                goto leave;
        }
    }
#endif
*/
leave:
    pthread_mutex_unlock(&usbids_load_mutex);
    return success;
}



void usbutils::usbutils_get_device_strings(int bus, int address,
                                       int vendor_id, int product_id,
                                       char  *manufacturer, char  *product)
{
    UsbProductInfo *product_info;
    int i, j;

    char *pmanufacturer = NULL;
    char *pproduct = NULL;

   pmanufacturer = usbutils_get_sysfs_attribute(bus, address, "manufacturer");
   pproduct = usbutils_get_sysfs_attribute(bus, address, "product");


    if ((!pmanufacturer || !pproduct) &&
        usbutils_load_usbids()) {

        for (i = 0; i < usbids_vendor_count; i++) {
            if ((int)usbids_vendor_info[i].vendor_id != vendor_id)
                continue;

            if (!pmanufacturer && usbids_vendor_info[i].name[0])
                pmanufacturer = strdup(usbids_vendor_info[i].name);

            product_info = usbids_vendor_info[i].product_info;
            for (j = 0; j < usbids_vendor_info[i].product_count; j++) {
                if ((int)product_info[j].product_id != product_id)
                    continue;

                if (!pproduct && product_info[j].name[0])
                    pproduct = strdup(product_info[j].name);

                break;
            }
            break;
        }
    }

    if (!pmanufacturer)
        pmanufacturer = strdup("USB");
    if (!pproduct)
        pproduct = strdup("Device");

    strcpy(product,pproduct);
    strcpy(manufacturer,pmanufacturer);
    free(pproduct);
    free(pmanufacturer);



#if 0

    /* Some devices have unwanted whitespace in their strings */
    g_strstrip(*manufacturer);
    g_strstrip(*product);

    /* Some devices repeat the manufacturer at the beginning of product */
    if (g_str_has_prefix(*product, *manufacturer) &&
            strlen(*product) > strlen(*manufacturer)) {
        gchar *tmp = g_strdup(*product + strlen(*manufacturer));
        g_free(*product);
        *product = tmp;
        g_strstrip(*product);
    }
#endif 
}



void usbutils::usbutils_device_get_description(UsbDeviceInfo &device)
{
   int bus, address, vid, pid;
   char *description, *descriptor, *manufacturer = NULL, *product = NULL;


   bus     = usbutils_device_get_busnum(device);
   address = usbutils_device_get_devaddr(device);
   vid     = usbutils_device_get_vid(device);
   pid     = usbutils_device_get_pid(device);

   usbutils_get_device_strings(bus, address,vid, pid,device.vendor_name, device.product_name);


}

unsigned char usbutils::usbutils_device_get_busnum(const UsbDeviceInfo &device)
{
    return device.busnum;
}

unsigned char usbutils::usbutils_device_get_devaddr(const UsbDeviceInfo &device)
{
    return device.devaddr;
}

unsigned short usbutils::usbutils_device_get_vid(const UsbDeviceInfo &device)
{
    return device.vid;
}

unsigned short usbutils::usbutils_device_get_pid(const UsbDeviceInfo &device)
{
    return device.pid;
}

list<UsbDeviceInfo >& usbutils::get_dev_list()
{
	return dev_list;
}

bool usbutils::usbutils_device_get_interfaceclass(libusb_device *dev,UsbDeviceInfo &device)
{
	struct libusb_config_descriptor *config;
	int rc = libusb_get_config_descriptor(dev, 0, &config);
	if (rc) {
			fprintf(stderr, "Couldn't get configuration descriptor 0, "
					"some information will be missing\n");
			return false;
	} else {
	
		const struct libusb_interface *interface = &config->interface[0];
		for (int i = 0; i < interface->num_altsetting; i++)
		{	if(interface->altsetting[i].bInterfaceClass > 0)
				device.interface_class = interface->altsetting[i].bInterfaceClass;
		}
			
			
		//printf("interface class %d \n",device.interface_class);	
		libusb_free_config_descriptor(config);
	}
	return true;
}

void usbutils::usbutils_free_dev()
{
   for (list<UsbDeviceInfo>::iterator it = dev_list.begin();it != dev_list.end();it++)
   {
      if(it->libdev != NULL)
      {
         libusb_unref_device(it->libdev);
         it->libdev = NULL;
      }
   }
   dev_list.clear();
}

void usbutils::usbutils_listdevs()
{
   int  cnt;
   int i = 0;
   int rc = 0;
   libusb_device **devs; 
   libusb_device *dev;

   usbutils_free_dev();
   
   cnt = libusb_get_device_list(NULL,&devs);
   if (cnt < 0)
   {
      //MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("libusb_get_device_list error"));
      printf("libusb_get_device_list error\n");
   }
   while ((dev = devs[i++]) != NULL)
   {
      struct libusb_device_descriptor desc;
      rc = libusb_get_device_descriptor(dev, &desc);
      if (rc < 0)
      {
         dev_list.clear();
         libusb_free_device_list(devs, 1);
         const char *desc_str = usbutils_strerror((enum libusb_error)rc);
         printf("%s [%d]\n",desc_str,rc);

         return;
      }

      if (desc.bDeviceClass == LIBUSB_CLASS_HUB)
      {
         libusb_unref_device(dev);
         continue;
      }

	  UsbDeviceInfo tmp;
      memset(&tmp,0,sizeof(UsbDeviceInfo));
	  if(desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE && desc.bNumConfigurations)
	  {
	  	usbutils_device_get_interfaceclass(dev,tmp);
		if(tmp.interface_class == LIBUSB_CLASS_PER_INTERFACE)
	  	{
			libusb_unref_device(dev);
         	continue;
		}
		
	  }else if(desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE){
		 libusb_unref_device(dev);
         continue;
	  }
	 
      
      tmp.busnum = libusb_get_bus_number(dev);
      tmp.devaddr = libusb_get_device_address(dev);
      tmp.vid = desc.idVendor;
      tmp.pid = desc.idProduct;
      tmp.libdev = dev;    
      tmp.class_code = desc.bDeviceClass > LIBUSB_CLASS_PER_INTERFACE ? desc.bDeviceClass : tmp.interface_class ;
      usbutils_device_get_description(tmp);      
	  sprintf(tmp.usbrule_key,"0x%02x_0x%04x_0x%04x",tmp.class_code,tmp.vid,tmp.pid);
	  dev_list.push_back(tmp);
#ifdef DEBUG
      printf("[%s] [%s] [%s] at %d-%d\n",tmp.vendor_name,tmp.product_name,tmp.usbrule_key,tmp.busnum,tmp.devaddr);
#endif
   }

   free(devs);
   
}


int  usbutils::usbutils_init()
{	
   int rc = 0;
   rc = libusb_init(&context);

   if (rc < 0)
   {
      const char *strdesc = usbutils_strerror((enum libusb_error)rc);
      //MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("%1 %2 [%3]").arg(__FUNCTION__).arg(strdesc).arg(rc));
      printf("%s %s [%d]\n",__FUNCTION__,strdesc,rc);
   }
   return rc;
}



int usbutils::usbutils_init_hotplug()
{
   int rc = 0;
   rc = libusb_hotplug_register_callback(context,
         (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
         LIBUSB_HOTPLUG_ENUMERATE, (int)LIBUSB_HOTPLUG_MATCH_ANY,
         (int)LIBUSB_HOTPLUG_MATCH_ANY, (int)LIBUSB_HOTPLUG_MATCH_ANY,
         usbutils_hotplug_cb, this, &hp_handle);
   if (rc < 0)
   {
      const char *strdesc = usbutils_strerror((enum libusb_error)rc);
      //MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("%1 %2 [%3]").arg(__FUNCTION__).arg(strdesc).arg(rc));
      printf("%s %s [%d]\n",__FUNCTION__,strdesc,rc);
   }
   rc = usbutils_start_event_listening(NULL);

}

const char *usbutils::usbutils_strerror(enum libusb_error error_code)
{
   switch (error_code) {
      case LIBUSB_SUCCESS:
         return "Success";
      case LIBUSB_ERROR_IO:
         return "Input/output error";
      case LIBUSB_ERROR_INVALID_PARAM:
         return "Invalid parameter";
      case LIBUSB_ERROR_ACCESS:
         return "Access denied (insufficient permissions)";
      case LIBUSB_ERROR_NO_DEVICE:
         return "No such device (it may have been disconnected)";
      case LIBUSB_ERROR_NOT_FOUND:
         return "Entity not found";
      case LIBUSB_ERROR_BUSY:
         return "Resource busy";
      case LIBUSB_ERROR_TIMEOUT:
         return "Operation timed out";
      case LIBUSB_ERROR_OVERFLOW:
         return "Overflow";
      case LIBUSB_ERROR_PIPE:
         return "Pipe error";
      case LIBUSB_ERROR_INTERRUPTED:
         return "System call interrupted (perhaps due to signal)";
      case LIBUSB_ERROR_NO_MEM:
         return "Insufficient memory";
      case LIBUSB_ERROR_NOT_SUPPORTED:
         return "Operation not supported or unimplemented on this platform";
      case LIBUSB_ERROR_OTHER:
         return "Other error";
   }
   return "Unknown error";
}



