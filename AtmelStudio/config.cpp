#include "config.h"

eeconfig_t eeconfig EEMEM = {

HWADDRESS,		//address
MODE,		//mode
20,		//trigger time 100 ms
1<<RELAY_FLAG_POLARITY | 1<<RELAY_FLAG_RETRIGGERABLE
};
