# CHIP Talaria Smoke-CO-Alarm Sensor Example

An example showing the use of CHIP on the Talaria Two platform. The document will describe how
to build and run CHIP Smoke-CO-Alarm Sensor Example on Talaria Two platform. In this example, utilized rand function to generate random Smoke-CO-Alarm attribute values  for updating Smoke CO Alarm Sensor status every 5sec (can be configurable) from software timer callback as actual Smoke-CO-Alarm Sensor is not available.

This doc is tested on **Ubuntu 22.04.3 LTS (x86_64)**

## Building

-   Setup the environment by running the boostrap.sh script as menionend in the FreeRTOS_sdk_3.0_master_matter/matter/README.md **Building the Example application** secion

-   Build the example application:

          $ cd connectedhomeip/examples/smoke-co-alarm-app/talaria
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make

-   To delete generated executable, libraries and object files use:

          $ cd connectedhomeip/examples/smoke-co-alarm-app/talaria
          $ make clean

## Boot arguments

-   `matter.discriminator=<value>`

    Setting the discriminator value to identify the device. Unisigned short integer value. Default: 3840.

-   `matter.commissioning.flow_type=<value>`

    0: Standard commissioning flow (Default)
    1: User-intended commissioning flow. To enable this, GPIO3 should be connected to GND for 5 seconds and then disconnected from GND

-   `suspend=<value>`

    Puts the device in suspend state
    0: Disable suspend (Default)
    1: Enable suspend

-   `matter.factory_reset=<value>`

    To enable/disable the flow for factory resetting the device.
    0: Disable Factory Reset (Default)
    1: Enable factory reset (In this case, contents of the data partition table (SSID, passphrase, chip-tool keys) on     Talaria TWO will be erased)
       Note: In case of connecting to a new AP, ensure to erase the contents of the data partition table before initiating a new connection.


## Programming the Example on Talaria Two Platform
The Linux Tool is provided in FreeRTOS_sdk_3.0_master_matter/pc_tools/Download_Tool/bin/T2DownloadTool_Linux to program the Talaria Two device. Following are the steps to program the device.

#### Updating the partition table of T2 EVB (One Time)
- Connect T2 EVB with the system
- Run following command to flash gordon.elf

          $ cd <FreeRTOS_sdk_3.0_master_matter-root-directory>
          $ sudo python3 ./script/boot.py --device /dev/ttyUSB2 ./apps/gordon.elf --reset=evk42_bl
- Run the following command to update the partition table

          $ cd <FreeRTOS_sdk_3.0_master_matter-root-directory>
          $ sudo python3 ./script/flash.py --device /dev/ttyUSB2 from_json ./tools/partition_files/matter_demo_partition.json

#### Programming image elf
- Connect T2 EVB with the system
- Run the T2DownloadTool_Linux tool with sudo access from FreeRTOS_sdk_3.0_master_matter/pc_tools/Download_Tool/bin/ and select following fields in the GUI tool
- Boot Target: Select the appropriate EVK from the drop-down
- ELF Input: Load the out/test/smoke-co-alarm-app.elf by clicking on Select ELF File
- Boot Arguments: Pass the following boot arguments

          matter.discriminator=1122 matter.commissioning.flow_type=0 suspend=1 matter.factory_reset=0
- Programming: Prog RAM or Prog Flash as per requirement.
**For more details on using the Download tool, refer to the document: UG_Download_Tool.pdf (path:
FreeRTOS_sdk_3.0_master_matter/pc_tools/Download_Tool/doc/UG_Download_Tool.pdf).**

## Commissioning and Controlling the device
- Run generated chip-tool binary using following commands. Steps to build chip-tool is provided in FreeRTOS_sdk_3.0_master_matter/matter/README.md

          $ out/debug/chip-tool interactive start
          >>> pairing ble-wifi <node-id> <wifi-ssid> <wifi-passwd> 20202021 <discriminator>
          e.g. pairing ble-wifi 1111 tplinkc6_iop InnoQA2023$ 20202021 1122
- Once the commissioning is completed successfully you can use following commands to perform Smoke-CO-Alarm Sensor operations on Talaria Two EVB. Here the 1111 is the node-id given at the time of commissioning to the device

          ## Command to send self-test request
          >>> smokecoalarm self-test-request  1111 1
          ## Commands to read the Smoke-CO-Alarm Sensor status
          >>> smokecoalarm read expressed-state 1111 1
          >>> smokecoalarm read smoke-state 1111 1
          >>> smokecoalarm read costate 1111 1
          >>> smokecoalarm read battery-alert 1111 1
          >>> smokecoalarm read device-muted 1111 1
          >>> smokecoalarm read test-in-progress 1111 1
          >>> smokecoalarm read hardware-fault-alert 1111 1
          >>> smokecoalarm read end-of-service-alert 1111 1
          >>> smokecoalarm read interconnect-smoke-alarm 1111 1
          >>> smokecoalarm read interconnect-coalarm 1111 1
          >>> smokecoalarm read contamination-state 1111 1
          >>> smokecoalarm read smoke-sensitivity-level 1111 1
          >>> smokecoalarm read expiry-date 1111 1
          >>> smokecoalarm read cluster-revision 1111 1

          ## Commands to subscribe for Smoke-CO-Alarm Sensor status
          >>> smokecoalarm subscribe expressed-state 1111 1
          >>> smokecoalarm subscribe smoke-state 1111 1
          >>> smokecoalarm subscribe costate 1111 1
          >>> smokecoalarm subscribe battery-alert 1111 1
          >>> smokecoalarm subscribe device-muted 1111 1
          >>> smokecoalarm subscribe test-in-progress 1111 1
          >>> smokecoalarm subscribe hardware-fault-alert 1111 1
          >>> smokecoalarm subscribe end-of-service-alert 1111 1
          >>> smokecoalarm subscribe interconnect-smoke-alarm 1111 1
          >>> smokecoalarm subscribe interconnect-coalarm 1111 1
          >>> smokecoalarm subscribe contamination-state 1111 1
          >>> smokecoalarm subscribe smoke-sensitivity-level 1111 1
          >>> smokecoalarm subscribe expiry-date 1111 1

### Enabling test event trigger
- Enable "TALARIA_TEST_EVENT_TRIGGER_ENABLED" to trigger the test events for Smoke-CO-Alarm Sensor. It is enabled by default.
- Below are the test events available for Smoke-CO-Alarm Sensor

    // Force alarm commands
    kForceSmokeWarning           = 0xffffffff00000090,
    kForceCOWarning              = 0xffffffff00000091,
    kForceSmokeInterconnect      = 0xffffffff00000092,
    kForceMalfunction            = 0xffffffff00000093,
    kForceCOInterconnect         = 0xffffffff00000094,
    kForceLowBatteryWarning      = 0xffffffff00000095,
    kForceSmokeContaminationHigh = 0xffffffff00000096,
    kForceSmokeContaminationLow  = 0xffffffff00000097,
    kForceSmokeSensitivityHigh   = 0xffffffff00000098,
    kForceSmokeSensitivityLow    = 0xffffffff00000099,
    kForceEndOfLife              = 0xffffffff0000009a,
    kForceSilence                = 0xffffffff0000009b,
    kForceSmokeCritical          = 0xffffffff0000009c,
    kForceCOCritical             = 0xffffffff0000009d,
    kForceLowBatteryCritical     = 0xffffffff0000009e,

    // Clear alarm commands
    kClearSmoke             = 0xffffffff000000a0,
    kClearCO                = 0xffffffff000000a1,
    kClearSmokeInterconnect = 0xffffffff000000a2,
    kClearMalfunction       = 0xffffffff000000a3,
    kClearCOInterconnect    = 0xffffffff000000a4,
    kClearBatteryLevelLow   = 0xffffffff000000a5,
    kClearContamination     = 0xffffffff000000a6,
    kClearSensitivity       = 0xffffffff000000a8,
    kClearEndOfLife         = 0xffffffff000000aa,
    kClearSilence           = 0xffffffff000000ab

          ## Command to trigger test event
          >>> generaldiagnostics test-event-trigger <EnableKey> <EventTrigger> <destination-id> <endpoint-id>
          ## Example:
          >>> generaldiagnostics test-event-trigger hex:00112233445566778899aabbccddeeff 0xffffffff00000090 1111 0
- Here "hex:00112233445566778899aabbccddeeff" is the default EnableKey programmed, can be configurable.
