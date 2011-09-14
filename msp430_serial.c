#include "msp430_serial.h"
#include <stdio.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>

bool isLibUSBInitialized = false;
struct libusb_context *mspContext = NULL;

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
  printf("Exiting.\n");
  return(0);
}


HANDLE init_dev(UINT DevNum, unsigned int dwReserved, unsigned long ProductID) {
  void * handle = NULL;
  //struct libusb_device_handle *devh = NULL;
  libusb_device **devs;
  ssize_t cnt;
  int r = 1;
  libusb_device *dev;
  unsigned int i = 0;
  //unsigned int mspFoundCount = 0;
  struct libusb_device_descriptor desc;

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

static void MSP_uninitialize(void) {
  if (isLibUSBInitialized) {
    libusb_exit(mspContext);
    mspContext = NULL;
    isLibUSBInitialized = false;
  }
}

static int MSP_libusbError(int r) {
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
