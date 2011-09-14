#include <stdbool.h>
#include <stdint.h>

typedef void * HANDLE;

HANDLE init_dev(uint16_t DevNum, uint16_t dwReserved, uint32_t ProductID);
static int MSP_libusb_error(int r);
static void MSP_uninitialize(HANDLE h);
static void MSP_setup(HANDLE h);
static int MSP_get_endpoints(HANDLE h, uint8_t *int_in, uint8_t *bulk_in, uint8_t *bulk_out);
static void describe_handle(HANDLE h);
