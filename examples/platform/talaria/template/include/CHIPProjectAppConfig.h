#pragma once

// include the CHIPProjectConfig from config/standalone
#include <CHIPProjectConfig.h>

#define CHIP_DEVICE_CONFIG_ENABLE_COMMISSIONER_DISCOVERY 0

#define CHIP_DEVICE_CONFIG_ENABLE_COMMISSIONER_DISCOVERY_CLIENT 1

#define CHIP_DEVICE_CONFIG_ENABLE_EXTENDED_DISCOVERY 1

#define CHIP_DEVICE_CONFIG_ENABLE_COMMISSIONABLE_DEVICE_TYPE 1

/**************** Placeholder: Application Specific device type macro ************/
/* e.g.
#define CHIP_DEVICE_CONFIG_DEVICE_TYPE 0x202 // Matter Window Covering
#define CHIP_DEVICE_CONFIG_DEVICE_NAME "Test Window App"
*/
/********************************************************************************/

#define CHIP_DEVICE_CONFIG_ENABLE_COMMISSIONABLE_DEVICE_NAME 1

#define CHIP_DEVICE_ENABLE_PORT_PARAMS 1

#define CHIP_CONFIG_MAX_FABRICS 8
