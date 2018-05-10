#include <stdio.h>
#include <string.h>
#include <libusb.h>

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
                    
static void change_color(struct libusb_device_handle *handle, unsigned char *color) {
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
    int ret = 0;

ret = libusb_detach_kernel_driver(handle, index);
    if (ret<0) {
      printf("libusb_detach_kernel_driver %d %s %s \n", ret, libusb_error_name(ret), libusb_strerror(ret));
      //return;
    }

    ret = libusb_claim_interface(handle, index);
    if (ret<0) {
      printf("libusb_claim_interface %d %s %s \n", ret, libusb_error_name(ret), libusb_strerror(ret));
      return;
    }

    printf("Sending rtype %x req %x val %x idx %x \n",reqtype, request, value, index);
    char* toprint = usb_data;
    for(int i=0; i<usb_data_len;i++)
      printf("%02x", (unsigned int)usb_data[i]);
    printf("\n");
  
    ret = libusb_control_transfer(handle, reqtype, request, value, index, (char*)usb_data, usb_data_len, 5000);
    if (ret<0) {
      printf("libusb_control_transfer %d %s %s \n", ret, libusb_error_name(ret), libusb_strerror(ret));
      return;
    }
    
    ret = libusb_release_interface(handle, index);
    if (ret<0) {
      printf("libusb_release_interface %d %s %s \n", ret, libusb_error_name(ret), libusb_strerror(ret));
      return;
    }
ret = libusb_attach_kernel_driver(handle, index);

    if (ret<0) {
      printf("libusb_attach_kernel_driver %d %s %s \n", ret, libusb_error_name(ret), libusb_strerror(ret));
      return;
    }

    printf("All good\n");
}


void print_help(const char* program) {
  fprintf(stderr, "Usage: %s <RGB>\n", program);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t<RGB>\tSpecifies a RGB color in hex mode. i.e. AA00FF\n");
}

int main(int argc, char **argv) {
    struct libusb_context *ctx;
    struct usb_device *usb_dev;
    struct libusb_device_handle *usb_handle;

    if ((argc != 2) || (strlen(argv[1]) != 6) || !is_hex(argv[1])) {
        print_help(argv[0]);
        return 1;
    } 

    if (libusb_init(&ctx) != 0) {
        fprintf(stderr, "Unable to init library\n");
        return 3;
    }

    usb_handle = libusb_open_device_with_vid_pid (ctx, G203_VENDOR_ID, G203_PRODUCT_ID);
    if (usb_handle == NULL) {
        fprintf(stderr, "Unable to open USB device!\n");
        return 3;
    }

    change_color(usb_handle, argv[1]);
    libusb_exit(ctx);
    return 0;
}

