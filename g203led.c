#include <stdio.h>
#include <string.h>
#include <usb.h>

#define G203_VENDOR_ID   0x046d
#define G203_PRODUCT_ID  0xc084

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
                    
static void change_color(struct usb_dev_handle *handle, unsigned char *color) {
    unsigned char usb_data[] = { 0x11, 0xff, 0x0e, 0x3b, 0x00, 0x01, 
                                 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    usb_data[6] = get_hex8(color);
    usb_data[7] = get_hex8(color+2);
    usb_data[8] = get_hex8(color+4);

    uint8_t request = 0x09;
    uint8_t reqtype = 0x21;
    uint16_t value = 0x0211;
    uint16_t index = 0x01;
    usb_control_msg(handle, reqtype, request, value, index, (char*)usb_data, sizeof(usb_data), 5000);
}

static struct usb_device *device_init(void) {
    struct usb_bus *usb_bus;
    struct usb_device *dev;

    usb_init();
    usb_find_busses();
    usb_find_devices();

    for (usb_bus = usb_busses; usb_bus; usb_bus = usb_bus->next) {
        for (dev = usb_bus->devices; dev; dev = dev->next) {
            if ((dev->descriptor.idVendor == G203_VENDOR_ID) && (dev->descriptor.idProduct == G203_PRODUCT_ID))
        	    return dev;
        }
    }
    return NULL;
}

void print_help(const char* program) {
  fprintf(stderr, "Usage: %s <RGB>\n", program);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t<RGB>\tSpecifies a RGB color in hex mode. i.e. AA00FF\n");
}

int main(int argc, char **argv) {
    struct usb_device *usb_dev;
    struct usb_dev_handle *usb_handle;

    if ((argc != 2) || (strlen(argv[1]) != 6) || !is_hex(argv[1])) {
        print_help(argv[0]);
        return 1;
    } 

    usb_dev = device_init();
    if (usb_dev == NULL) {
        fprintf(stderr, "Device not found!\n");
        return 2;
    }

    usb_handle = usb_open(usb_dev);
    if (usb_handle == NULL) {
        fprintf(stderr, "Unable to open USB device!\n");
        return 3;
    }

    change_color(usb_handle, argv[1]);
    usb_close(usb_handle);
    return 0;
}

