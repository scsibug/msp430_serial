#include "msp430_serial.h"
#include <stdio.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>

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
  libusb_set_debug(mspContext, 3);
  // print info
  //describe_handle(h);
  // get endpoints
  MSP_get_endpoints(h, &ep_int_in, &ep_bulk_in, &ep_bulk_out);
  // Send magic setup control transfers...
  MSP_setup(h);
  printf("Exiting.\n");
  MSP_uninitialize(h);
  return(0);
}

static void MSP_setup(HANDLE h) {
  //int r;
  //  r = libusb_control_transfer(h, E);
}

static int MSP_get_endpoints(HANDLE h, uint8_t *int_in, uint8_t *bulk_in, uint8_t *bulk_out) {
  int c;
  int j, k, m;
  struct libusb_config_descriptor *config;
  const struct libusb_endpoint_descriptor *eps;
  libusb_device *dev = libusb_get_device(h);
  libusb_get_configuration(h, &c);
  if (c == 0) {
    printf("Device is unconfigured\n");
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
          printf("Found endpoint (0x%x)\n",eps[m].bEndpointAddress);
          if (eps[m].bmAttributes & LIBUSB_TRANSFER_TYPE_INTERRUPT) {
            if (eps[m].bEndpointAddress & LIBUSB_ENDPOINT_IN) {
              printf("Found INTERRUPT IN endpoint (0x%x)\n",eps[m].bEndpointAddress);
              *int_in = eps[m].bEndpointAddress;
            }
          } else if (eps[m].bmAttributes & LIBUSB_TRANSFER_TYPE_BULK) {
            if (eps[m].bEndpointAddress & LIBUSB_ENDPOINT_IN) {
              printf("Found BULK IN endpoint\n");
              *bulk_in = eps[m].bEndpointAddress;
            } else if (eps[m].bEndpointAddress & LIBUSB_ENDPOINT_OUT) {
              printf("Found BULK OUT endpoint\n");
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
HANDLE init_dev(uint16_t DevNum, uint16_t dwReserved, uint32_t ProductID) {
  void * handle = NULL;
  //struct libusb_device_handle *devh = NULL;
  //libusb_device **devs;
  //ssize_t cnt;
  int r = 1;
  //libusb_device *dev;
  //unsigned int i = 0;
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
