#pragma once

#include <lib/core/CHIPError.h>



#if TALARIA_TEST_EVENT_TRIGGER_ENABLED
#include <app/TestEventTriggerDelegate.h>
#endif /* TALARIA_TEST_EVENT_TRIGGER_ENABLED */

namespace chip {
namespace talaria {
namespace talariautils {
void ApplicationInitLog(char * str);
void HeapAvailableLog();
void EnableSuspend();
void FactoryReset(int factory_reset_level);
void UserButtonFactoryReset(int factory_reset_level);

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

#if TALARIA_TEST_EVENT_TRIGGER_ENABLED

#define TALARIA_TEST_EVENT_TRIGGER_ENABLE_KEY "00112233445566778899aabbccddeeff"
static uint8_t sTestEventTriggerEnableKey[TestEventTriggerDelegate::kEnableKeyLength] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
                                                                                          0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,
                                                                                          0xcc, 0xdd, 0xee, 0xff };
class TalariaTestEventTriggerDelegate : public TestEventTriggerDelegate
{
public:
    explicit TalariaTestEventTriggerDelegate(const ByteSpan & enableKey) : mEnableKey(enableKey) {}

    /**
     * @brief Checks to see if `enableKey` provided matches value chosen by the manufacturer.
     * @param enableKey Buffer of the key to verify.
     * @return True or False.
     */
    bool DoesEnableKeyMatch(const ByteSpan & enableKey) const override;

    /**
     * @brief User handler for handling the test event trigger based on `eventTrigger` provided.
     * @param eventTrigger Event trigger to handle.
     * @return CHIP_NO_ERROR on success or CHIP_ERROR_INVALID_ARGUMENT on failure.
     */
    CHIP_ERROR HandleEventTrigger(uint64_t eventTrigger) override;

private:
    ByteSpan mEnableKey;
};
#endif /* TALARIA_TEST_EVENT_TRIGGER_ENABLED */

} // namespace talaria
} // namespace chip

/*-----------------------------------------------------------*/
