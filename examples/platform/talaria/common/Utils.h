#pragma once

#include <lib/core/CHIPError.h>

namespace chip {
namespace talaria {
namespace talariautils {
void ApplicationInitLog(char * str);
void HeapAvailableLog();
void EnableSuspend();
void FactoryReset();

} // namespace talariautils

namespace matterutils {
enum CommissionedStatus
{
    NOT_COMMISSIONED,
    PRIMARY_COMMISSIONED,
    SECONDARY_COMMISSIONED,
    MULTIPLE_COMMISSIONED,
};

enum CommissioningFlowType
{
    COMMISSIONING_FLOW_TYPE_INVALID = -1,
    COMMISSIONING_FLOW_TYPE_STANDARD,
    COMMISSIONING_FLOW_TYPE_USER_INTENT,
    COMMISSIONING_FLOW_TYPE_CUSTOM,
    COMMISSIONING_FLOW_TYPE_HOST,
    COMMISSIONING_FLOW_TYPE_MAX,
};

void MatterConfigLog();
bool IsNodeCommissioned();
bool IsNodePrimaryCommissioned();
bool IsNodeSecondaryCommissioned();
CommissionedStatus GetNodeCommissionedStatus();
uint8_t FabricCount();
CommissioningFlowType GetCommissioningFlowType();

} // namespace matterutils

} // namespace talaria
} // namespace chip

/*-----------------------------------------------------------*/
