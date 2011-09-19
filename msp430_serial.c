#include "msp430_serial.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

bool isLibUSBInitialized = false;
struct libusb_context *mspContext = NULL;
uint8_t ep_int_in, ep_bulk_in, ep_bulk_out;

int main() {
  printf("msp430_serial starting...\n");
  printf("libusb initing...\n");
  // initialize USB context, and get a device handle.
  HANDLE h = init_dev(1, 0, 0xf432);
  if (h == NULL) {
    fprintf(stderr, "Handle is NULL, initialization failed.\n");
    return(1);
  }
  // enable debugging
#ifdef DEBUG
  libusb_set_debug(mspContext, 3);
#endif
  // get endpoints
  MSP_get_endpoints(h, &ep_int_in, &ep_bulk_in, &ep_bulk_out);
  // Send magic setup control transfers...
  MSP_setup(h);
  get_descriptor(h);
  // TI driver does this 26 times... not sure why
  // read status
  do_control_transfer(h,0xa1,0x21);
  // write back to device
  do_send_std(h,0x21,0x20,true);

  while (1) {
    do_bulk_transfer(h);
    libusb_handle_events(mspContext);
  }
  printf("Exiting.\n");
  MSP_uninitialize(h);
  return(0);
}

static void MSP_setup(HANDLE h) {
  int r;
  // claim interface, but check if kernel has this device first:
  r = libusb_kernel_driver_active(h, 1);
  if (r == 0) {
    // interface is not in use
  } else if (r == 1) {
    printf("Kernel driver is active, exiting.\n");
    exit(1);
  } else {
    printf("found error claiming interface\n");
    MSP_libusb_error(r);
    exit(1);
  }
  r = libusb_claim_interface(h, 0);
  if (r != 0) {
    printf("found error claiming interface\n");
    MSP_libusb_error(r);
    exit(1);
  }
}

static void get_descriptor(HANDLE h) {
  int i, r;
  unsigned char *bdata;
  bdata = malloc(sizeof(*bdata) * 255);
  printf("======== Getting device descriptor string ========\n");
  uint8_t reqtype = LIBUSB_ENDPOINT_IN;
  uint8_t request = LIBUSB_REQUEST_GET_DESCRIPTOR;
  r = libusb_control_transfer(h, reqtype, request,
                              (LIBUSB_DT_STRING << 8) | 0x05, 0, bdata, 255, 0);
  for (i = 2; i < (r-1); i++) {
    printf("%c",bdata[i]);
  }
  printf("\n");
  free(bdata);
  if (r <= 0) {
    MSP_libusb_error(r);
    exit(1);
  } else {
    printf("Received %d bytes\n",r);
  }
}

static void do_control_transfer(HANDLE h, uint8_t reqtype, uint8_t request) {
  int i, r;
  unsigned char *bdata;
  bdata = malloc(sizeof(*bdata) * 0x7);
  printf("======== Control Transfer ========\n");
  printf("Sending request (no data)\n");
  r = libusb_control_transfer(h, reqtype, request, 0, 0, bdata, 0x7, 1000);
  printf("Control Transfer returned\n");
  free(bdata);
  if (r <= 0) {
    MSP_libusb_error(r);
    exit(1);
  } else {
    printf("Received %d bytes\n",r);
    for (i = 0; i < r; i++) {
      printf("%x ",bdata[i]);
    }
    printf("\n");
  }
}

static void do_send_std(HANDLE h, uint8_t reqtype, uint8_t request, bool sendbytes) {
  int i, r;
  unsigned char bpattern[7] = {0x60, 0x09, 0x00, 0x00, 0x00, 0x00, 0x08};
  unsigned char *bdata;
  bdata = malloc(sizeof(*bdata) * 0x7);
  printf("======== Control Transfer ========\n");
  //  uint8_t reqtype = 0xa1; //LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR;
  //  uint8_t request = 0x21;
  if (sendbytes) {
    r = libusb_control_transfer(h, reqtype, request, 0, 0, bpattern, 0x7, 1000);
  } else {
    r = libusb_control_transfer(h, reqtype, request, 0, 0, bdata, 0x7, 1000);
  }
  printf("Control Transfer returned\n");
  free(bdata);
  if (r <= 0) {
    MSP_libusb_error(r);
    exit(1);
  } else {
    printf("Received %d bytes\n",r);
    for (i = 0; i < r; i++) {
      printf("%x ",bdata[i]);
    }
    printf("\n");
  }
}

static void do_bulk_transfer(HANDLE h) {
  int r;
  unsigned char* bdata;
  // allocation
  struct libusb_transfer* transfer = libusb_alloc_transfer(0);
  //  printf("======== Bulk Transfer ========\n");
  if (transfer == NULL) {
    fprintf(stderr, "failed to allocate transfer\n");
  }
  bdata = malloc(sizeof(*bdata) * 1000);
  if (bdata == NULL) {
    fprintf(stderr, "failed to allocate memory for bulk transfer buffer\n");
  }
  // filling
  libusb_fill_bulk_transfer(transfer, h, ep_bulk_in, bdata, 1000, bulk_transfer_cb, 0, 0);
  // submission
  r = libusb_submit_transfer(transfer);
  //printf("bulk transfer submitted\n");
}

static void bulk_transfer_cb(struct libusb_transfer *transfer) {
  int i;
  //printf("======== Bulk Transfer (RETURN) ========\n");
  for (i = 0; i < transfer->actual_length; i++) {
    printf("%d",transfer->buffer[i]);
  }
  printf("\n");
  free(transfer->buffer);
  libusb_free_transfer(transfer);
}

static int MSP_get_endpoints(HANDLE h, uint8_t *int_in, uint8_t *bulk_in, uint8_t *bulk_out) {
  int r;
  int c;
  int j, k, m;
  struct libusb_config_descriptor *config;
  const struct libusb_endpoint_descriptor *eps;
  libusb_device *dev = libusb_get_device(h);
  libusb_get_configuration(h, &c);
  if (c == 0) {
    printf("Device is unconfigured\n");
    r = libusb_set_configuration(h, 1);
    if (r == 0) {
      printf("Successfully configured device for configuration 1\n");
    } else {
      MSP_libusb_error(r);
    }
  }
  libusb_get_active_config_descriptor(dev, &config);
  for (j = 0; j < config->bNumInterfaces; j++) {
    const struct libusb_interface *intf = &config->interface[j];
    for (k = 0; k < intf->num_altsetting; k++) {
      const struct libusb_interface_descriptor *intf_desc;
      intf_desc = &intf->altsetting[k];
      if (intf_desc->bInterfaceClass == LIBUSB_CLASS_COMM) {
        // This is the interface we are interested in
        eps = intf_desc->endpoint;
        for (m = 0; m < intf_desc->bNumEndpoints; m++) {
          if ((eps[m].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_INTERRUPT) {
            if ((eps[m].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
              printf("Found INTERRUPT IN endpoint (0x%x)\n",eps[m].bEndpointAddress);
              *int_in = eps[m].bEndpointAddress;
            }
          } else if (eps[m].bmAttributes & LIBUSB_TRANSFER_TYPE_BULK) {
            if ((eps[m].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
              printf("Found BULK IN endpoint (0x%x)\n",eps[m].bEndpointAddress);
              *bulk_in = eps[m].bEndpointAddress;
            } else if ((eps[m].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
              printf("Found BULK OUT endpoint (0x%x)\n",eps[m].bEndpointAddress);
              *bulk_out = eps[m].bEndpointAddress;
            }
          }
        }
      }
    }
  }
  return 0;
}

static void describe_handle(HANDLE h) {
  int c;
  int j, k, m;
  struct libusb_config_descriptor *config;
  const struct libusb_endpoint_descriptor *eps;
  libusb_device *dev = libusb_get_device(h);
  libusb_get_configuration(h, &c);
  if (c == 0) {
    printf("Device is unconfigured\n");
  } else {
    // We expect the configuration to be 1
    printf("Currently active configuration: %d\n",c);
  }
  libusb_get_active_config_descriptor(dev, &config);
  for (j = 0; j < config->bNumInterfaces; j++) {
    printf("Looking at interface #%d\n",j);
    const struct libusb_interface *intf = &config->interface[j];
    for (k = 0; k < intf->num_altsetting; k++) {
      printf("Looking at interface #%d, altsetting #%d\n",j,k);
      const struct libusb_interface_descriptor *intf_desc;
      intf_desc = &intf->altsetting[k];
      printf("Interface class: %d\n", intf_desc->bInterfaceClass);
      if (intf_desc->bInterfaceClass == LIBUSB_CLASS_COMM) {
        // This is the interface we are interested in
        printf(" Communications Device Class\n");
        // Enumerate endpoints
        eps = intf_desc->endpoint;
        for (m = 0; m < intf_desc->bNumEndpoints; m++) {
          printf("Endpoint: %d, address: %d\n",m, eps[m].bEndpointAddress);
          printf(" polling interval: %d\n", eps[m].bInterval);
        }
      } else if (intf_desc->bInterfaceClass == LIBUSB_CLASS_HID) {
        printf(" Human Interface Device Class\n");
      }
    }
  }
}
HANDLE init_dev() {
  void * handle = NULL;
  int r = 1;
  //unsigned int mspFoundCount = 0;
  //struct libusb_device_descriptor desc;

  if (!isLibUSBInitialized) {
    r = libusb_init(&mspContext);
    if (r < 0) {
      fprintf(stderr, "failed to initialize libusb\n");
      return NULL;
    } else {
      fprintf(stderr, "successfully initialized libusb\n");
    }
    isLibUSBInitialized = true;
  }
  // directly open the first device with a matching vendor/product ID
  handle = libusb_open_device_with_vid_pid(mspContext, 1105, 62514);
  return handle;
}

static void MSP_uninitialize(HANDLE h) {
  if (isLibUSBInitialized) {
    if (h != NULL) {
      libusb_close(h);
    }
    libusb_exit(mspContext);
    mspContext = NULL;
    isLibUSBInitialized = false;
  }
}

static int MSP_libusb_error(int r) {
  switch (r) {
  case LIBUSB_SUCCESS:
    // No error
    return 0;
    break;
  case LIBUSB_ERROR_IO:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_IO\n");
    errno = EIO;
    break;
  case LIBUSB_ERROR_INVALID_PARAM:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_INVALID_PARAM\n");
    errno = EINVAL;
    break;
  case LIBUSB_ERROR_ACCESS:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_ACCESS\n");
    errno = EACCES;
    break;
  case LIBUSB_ERROR_NO_DEVICE:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_NO_DEVICE\n");
    errno = ENXIO;
    break;
  case LIBUSB_ERROR_NOT_FOUND:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_NOT_FOUND\n");
    errno = ENOENT;
    break;
  case LIBUSB_ERROR_BUSY:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_BUSY\n");
    errno = EBUSY;
    break;
  case LIBUSB_ERROR_TIMEOUT:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_TIMEOUT\n");
    errno = ETIMEDOUT;
    break;
  case LIBUSB_ERROR_OVERFLOW:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_OVERFLOW\n");
    errno = EOVERFLOW;
    break;
  case LIBUSB_ERROR_PIPE:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_PIPE\n");
    errno = EPIPE;
    break;
  case LIBUSB_ERROR_INTERRUPTED:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_INTERRUPTED\n");
    errno = EINTR;
    break;
  case LIBUSB_ERROR_NO_MEM:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_NO_MEM\n");
    errno = ENOMEM;
    break;
  case LIBUSB_ERROR_NOT_SUPPORTED:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_NOT_SUPPORTED\n");
    errno = ENOSYS;
    break;
  case LIBUSB_ERROR_OTHER:
    fprintf(stderr, "libusb error: LIBUSB_ERROR_OTHER\n");
    if (errno == 0) {
      errno = ENOSYS;
    }
    break;
  default:
    fprintf(stderr, "libusb error: Unexpected error code: %d.\n", r);
    if (errno == 0) {
      errno = ENOSYS;
    }
    break;
  }
  return -1;
}
