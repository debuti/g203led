#include <stdio.h>
#include <string.h>
#include <libusb.h>
#include <unistd.h>
#include <inttypes.h>

#define G203_VENDOR_ID   0x046d
#define G203_PRODUCT_ID  0xc31c //0xc084

#define DEBUG 1
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
  fprintf(stderr, "\t-c <hex-str>\n");
  fprintf(stderr, "\t\tSpecifies a fixed RGB color in hex mode. i.e. AA00FF\n");
  fprintf(stderr, "\t-b <brightness>\n");
  fprintf(stderr, "\t\tSpecifies a brightness value, from 1 to 99. i.e. 50\n");
  fprintf(stderr, "\t-r <rate>\n");
  fprintf(stderr, "\t\tSpecifies a effect change rate, in secs, from 1 to 20. i.e. 5\n");
  fprintf(stderr, "\t-h\n");
  fprintf(stderr, "\t\tShows this message\n");
  fprintf(stderr, "The mode selected will depend upon the given options. Here is a table\n");
  fprintf(stderr, "\t\t\tColor\tBright\tRate\n");
  fprintf(stderr, "\tFixed value\tX\n");
  fprintf(stderr, "\tCarousel\t\tX\tX\n");
  fprintf(stderr, "\tBreathing\tX\tX\tX\n");
}

int doGetopt (int argc, char* argv[], unsigned char** color, uint8_t* brightness, uint16_t* rate) {
  int c;
  optind = 0;
  while((c = getopt(argc, argv, "c:b:r:h")) != -1) {
    switch(c) {
      case 'c': {
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
		*color = optarg;
        break;
      }
      case 'b': {
		if(optarg[0] == '-') {
		  fprintf(stderr, "Option -%c requires an valid brightness value.\n", c);
		  return 1;
		}
        if(sscanf(optarg, "%2" SCNu8, brightness) != 1 || *brightness<1 || *brightness>99) {
          fprintf(stderr, "Dude, the brightness must be a positive integer between 1 and 99. The more the number is, the more bright the led\n");
          return 1;
        }
        log_("brightness 0x%02hhx\n", *brightness);
        break;
      }
      case 'r': {
		if(optarg[0] == '-') {
		  fprintf(stderr, "Option -%c requires an valid brightness value.\n", c);
		  return 1;
		}
        if(sscanf(optarg, "%4" SCNu16, rate) != 1 || *rate<1 || *rate>20) {
          fprintf(stderr, "Dude, the rate must be a positive integer between 1 and 20. The more the number is, the slower it would change\n");
          return 1;
        }
        log_("rate 0x%04hhx\n", *rate);
    uint16_t onesec = 0x03e8;
    uint16_t twentysec = 0x4e20;
    uint16_t transfRate = ((twentysec-onesec)/20*(*rate))+onesec;
        log_("transfRate 0x%04hhx\n", transfRate);
        log_("transfRate %u\n", transfRate);
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

  return 0;
}
                    
int sendusb(struct libusb_device_handle *handle, unsigned char* usb_data, size_t usb_data_len){
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

int fixedColor(struct libusb_device_handle *handle, unsigned char *color) {
    unsigned char usb_data[] = { 0x11, 0xff, 0x0e, 0x3b, 0x00, 0x01, 
                                 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint16_t usb_data_len=20;

    usb_data[6] = get_hex8(color);
    usb_data[7] = get_hex8(color+2);
    usb_data[8] = get_hex8(color+4);

    return sendusb(handle, (char*)usb_data, usb_data_len);
}

  
int carousel(struct libusb_device_handle *handle, uint8_t brightness, uint16_t rate) {
    unsigned char usb_data[] = { 0x11, 0xff, 0x0e, 0x3b, 0x00, 0x02, 
                                 0x00, 0x00, 0x00, 0x00, 0x00, 
                                 0x00, 0x00,  //Rate
                                 0x00,        //Brightness
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint16_t usb_data_len=20;

    
    usb_data[13] = brightness;
    usb_data[11] = rate;
    usb_data[12] = rate;

    
   
    return sendusb(handle, (char*)usb_data, usb_data_len);
}


int breath(struct libusb_device_handle *handle, unsigned char *color, uint8_t brightness, uint16_t rate) {
    unsigned char usb_data[] = { 0x11, 0xff, 0x0e, 0x3b, 0x00, 0x03, 
                                 0x00, 0x00, 0x00, //Color
                                 0x00, 0x00,  //Rate
                                 0x00,  
                                 0x00,        //Brightness
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint16_t usb_data_len=20;

    usb_data[6] = get_hex8(color);
    usb_data[7] = get_hex8(color+2);
    usb_data[8] = get_hex8(color+4);
    usb_data[9] = rate;
    usb_data[10] = rate;
    usb_data[12] = brightness;
   
    return sendusb(handle, (char*)usb_data, usb_data_len);
}

int main(int argc, char **argv) {
    struct libusb_context *ctx;
    struct libusb_device_handle *usb_handle;
	int ret;

    unsigned char* color = NULL;
    uint8_t brightness = -1;
    uint16_t rate = -1;

    ret = doGetopt (argc, argv, &color, &brightness, &rate);
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

    if (color && brightness!=-1 && rate!=-1) 
      ret = breath(usb_handle, color, brightness, rate);
    else if (brightness!=-1 && rate!=-1) 
      ret = carousel(usb_handle, brightness, rate);
    else if (color) 
      ret = fixedColor(usb_handle, color);

    libusb_exit(ctx);

    return ret;
}

