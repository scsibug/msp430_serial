#include "msp430_serial.h"
#include <stdio.h>
#include <libusb-1.0/libusb.h> 

bool isLibUSBInitialized = false;
struct libusb_context *mspContext = NULL;

int main() {
  printf("msp430_serial starting...\n");
  printf("libusb initing...\n");
  init_dev(1, 0, 0xf432);
  printf("Exiting.\n");
  return(0);
}


HANDLE init_dev(UINT DevNum, unsigned int dwReserved, unsigned long ProductID) {
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
  return handle;
}

