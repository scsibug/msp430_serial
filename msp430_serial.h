#include <stdbool.h>


typedef void * HANDLE;
typedef unsigned int UINT;
typedef unsigned char BYTE;

HANDLE init_dev(UINT DevNum, unsigned int dwReserved, unsigned long ProductID);
