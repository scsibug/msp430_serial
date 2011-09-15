#include <stdbool.h>
#include <stdint.h>
#include <libusb-1.0/libusb.h>

typedef void * HANDLE;

HANDLE init_dev(uint16_t DevNum, uint16_t dwReserved, uint32_t ProductID);
static int MSP_libusb_error(int r);
static void MSP_uninitialize(HANDLE h);
static void MSP_setup(HANDLE h);
static int MSP_get_endpoints(HANDLE h, uint8_t *int_in, uint8_t *bulk_in, uint8_t *bulk_out);
static void describe_handle(HANDLE h);
static void do_bulk_transfer(HANDLE h);
static void bulk_transfer_cb(struct libusb_transfer *transfer);
static void do_control_transfer(HANDLE h, uint8_t reqtype, uint8_t request);
static void do_send_std(HANDLE h, uint8_t reqtype, uint8_t request, bool sendbytes);
static void get_descriptor(HANDLE h);
