#include <stdio.h>
#include <string.h>
#include <libusb.h>
#include <unistd.h>

#define G203_VENDOR_ID   0x046d
#define G203_PRODUCT_ID  0xc084

#define DEBUG 0
#define log_(fmt, ...) if(DEBUG) printf(fmt, ## __VA_ARGS__)

unsigned int get_hex4(unsigned char *p) {
  return (*p > '9' ? *p - 'A' + 10 : *p - '0') & 15;
}

unsigned int get_hex8(unsigned char *p) {
  return (get_hex4(p) << 4) + get_hex4(p+1);
}

int is_hex(unsigned char *hex) {
  char buf[7] = {0};  
  return (strtok(strncpy(buf, hex, sizeof(buf)), "0123456789ABCDEFabcdef") == NULL);
}

static void printHelp (const char* program) {
  fprintf(stderr, "Usage: %s [options]\n", program);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t-f <hex-str>\n");
  fprintf(stderr, "\t\tSpecifies a fixed RGB color in hex mode. i.e. AA00FF\n");
  fprintf(stderr, "\t-h\n");
  fprintf(stderr, "\t\tShows this message\n");
}

int doGetopt (int argc, char* argv[], char** fixedColour) {
  int c;
  optind = 0;
  while((c = getopt(argc, argv, "f:h")) != -1) {
    switch(c) {
      case 'f': {
		if(optarg[0] == '-') {
		  fprintf(stderr, "Option -%c requires an valid hex value.\n", c);
		  return 1;
		}
        if(strlen(optarg) != 6) {
		  fprintf(stderr, "The hex value should be 6 bytes long\n");
          return 1;
        } 
        if(!is_hex(optarg)) {
		  fprintf(stderr, "That\'s not an hex value\n");
          return 1;
        } 
		*fixedColour = optarg;
		return 0;
        break;
      }
      case 'h': {
        printHelp(argv[0]);
        return 2;
        break;
      }
      case '?':
      default: {
        printHelp(argv[0]);
        return 1;
        break;
      }
    }
  }
  printHelp(argv[0]);
  return 1;
}
                    
int fixedColor(struct libusb_device_handle *handle, unsigned char *color) {
    unsigned char usb_data[] = { 0x11, 0xff, 0x0e, 0x3b, 0x00, 0x01, 
                                 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint16_t usb_data_len=20;

    usb_data[6] = get_hex8(color);
    usb_data[7] = get_hex8(color+2);
    usb_data[8] = get_hex8(color+4);

    uint8_t reqtype = 0x21; // Host-to-device, Class, to Interface
    uint8_t request = 0x09; 
    uint16_t value = 0x0211;
    uint16_t index = 0x0001;  
    int ret = 0, krn = 0;

    krn = libusb_kernel_driver_active(handle, index);
    if (krn < 0) {
      fprintf(stderr, "Unable to know whether the kernel driver is active or not. %s:%s\n", libusb_error_name(krn), libusb_strerror(krn));
      return krn;
    } 
    if (krn == 1) {
      ret = libusb_detach_kernel_driver(handle, index);
      if (ret < 0) {
        fprintf(stderr, "Unable to detach the kernel driver. %s:%s\n", libusb_error_name(ret), libusb_strerror(ret));
        return ret;
      }
    }

    ret = libusb_claim_interface(handle, index);
    if (ret < 0) {
      fprintf(stderr, "Unable to claim the interface. %s:%s\n", libusb_error_name(ret), libusb_strerror(ret));
      return ret;
    }

    {
      log_("Sending rtype %x req %x val %x idx %x \n", reqtype, request, value, index);
      for(int i=0; i<usb_data_len;i++)
        log_("%02x", (unsigned int)usb_data[i]);
      log_("\n");
    }
  
    ret = libusb_control_transfer(handle, reqtype, request, value, index, (char*)usb_data, usb_data_len, 5000);
    if (ret < 0) {
      fprintf(stderr, "Unable to send the control message. %s:%s\n", libusb_error_name(ret), libusb_strerror(ret));
      return ret;
    }
    
    ret = libusb_release_interface(handle, index);
    if (ret < 0) {
      fprintf(stderr, "Unable to release the interface. %s:%s\n", libusb_error_name(ret), libusb_strerror(ret));
      return ret;
    }

    if (krn == 1) {
      ret = libusb_attach_kernel_driver(handle, index);
      if (ret < 0) {
        fprintf(stderr, "Unable to reattach the kernel driver. %s:%s\n", libusb_error_name(ret), libusb_strerror(ret));
        return ret;
      }
    }

    return 0;
}

int main(int argc, char **argv) {
    struct libusb_context *ctx;
    struct usb_device *usb_dev;
    struct libusb_device_handle *usb_handle;
	int ret;

    char* fixedColour = NULL;

    ret = doGetopt (argc, argv, &fixedColour);
    if (ret != 0) {
      log_("getOpt failed \n");
      return ret;
    }

    ret = libusb_init(&ctx);
    if (ret != 0) {
      fprintf(stderr, "Unable to init library. Reason %s:%s\n",libusb_error_name(ret), libusb_strerror(ret));
      return ret;
    }

    usb_handle = libusb_open_device_with_vid_pid (ctx, G203_VENDOR_ID, G203_PRODUCT_ID);
    if (usb_handle == NULL) {
      fprintf(stderr, "Unable to open USB device!\n");
      return -1;
    }

    if (fixedColour) 
      ret = fixedColor(usb_handle, fixedColour);

    libusb_exit(ctx);

    return ret;
}

