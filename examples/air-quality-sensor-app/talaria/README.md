# CHIP Talaria Air-Quality Sensor Example

An example showing the use of CHIP on the Talaria Two platform. The document will describe how
to build and run CHIP Air-Quality Sensor Example on Talaria Two platform. In this example, utilized rand function to generate random air quality index values between 0 and 500 for updating air quality status value every 5sec (can be configurable) from software timer callback as actual air quality sensor is not available.
-   Reference for air quality index range values to determine the air quality status: https://www.airnow.gov/aqi/aqi-basics

This doc is tested on **Ubuntu 22.04.3 LTS (x86_64)**

## Building

-   Setup the environment by running the boostrap.sh script as menionend in the FreeRTOS_sdk_3.0_master_matter/matter/README.md **Building the Example application** secion

-   Build the example application:

          $ cd connectedhomeip/examples/air-quality-sensor-app/talaria
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make

-   To delete generated executable, libraries and object files use:

          $ cd connectedhomeip/examples/air-quality-sensor-app/talaria
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
          $ sudo python3 script/boot.py --device /dev/ttyUSB2 ./apps/gordon.elf
- Run the following command to update the partition table

          $ cd <FreeRTOS_sdk_3.0_master_matter-root-directory>
          $ sudo python3 ./script/flash.py --device /dev/ttyUSB2 from_json ./tools/partition_files/matter_demo_partition.json

#### Programming image elf
- Connect T2 EVB with the system
- Run the T2DownloadTool_Linux tool with sudo access from FreeRTOS_sdk_3.0_master_matter/pc_tools/Download_Tool/bin/ and select following fields in the GUI tool
- Boot Target: Select the appropriate EVK from the drop-down
- ELF Input: Load the out/test/air-quality-sensor-app.elf by clicking on Select ELF File
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
- Once the commissioning is completed successfully you can use following commands to perform Air-Quality Sensor operations on Talaria Two EVB. Here the 1111 is the node-id given at the time of commissioning to the device

          ## Command to subscribe for air-quality status
          >>> airquality subscribe air-quality 5 10 1111 1
          ## Command to read the air-quality status
          >>> airquality read air-quality 1111 1
          ## Command to read the air-quality cluster version
          >>> airquality read cluster-revision 1111
          ## Command to subscribe for temperaturemeasurement data
          >>> temperaturemeasurement subscribe measured-value 5 10 1111 1
          ## Command to read the temperaturemeasurement data
          >>> temperaturemeasurement read measured-value 1111 1
          ## Command to subscribe for relativehumiditymeasurement data
          >>> relativehumiditymeasurement subscribe measured-value 5 10 1111 1
          ## Command to read the relativehumiditymeasurement data
          >>> relativehumiditymeasurement read measured-value 1111 1
- To change the temperaturemeasurement measured-value to lowest, connect the GPIO18 to ground.
- To change the temperaturemeasurement measured-value to highest, connect the GPIO18 to VCC.
- To change the relativehumiditymeasurement measured-value to lowest, connect the GPIO18 to ground.
- To change the relativehumiditymeasurement measured-value to highest, connect the GPIO18 to VCC.
