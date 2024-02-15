# CHIP Talaria Light Switch Example

An example showing the use of CHIP on the Talaria Two platform. The document will describe how
to build and run CHIP Light Switch Example on Talaria Two platform. This doc is tested
on **Ubuntu 22.04.3 LTS (x86_64)**

## Building

-   Setup the environment by running the boostrap.sh script as menionend in the FreeRTOS_sdk_3.0_master_matter/matter/README.md **Building the Example application** secion

-   Build the example application:

          $ cd connectedhomeip/examples/light-switch-app/talaria/
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make

-   To delete generated executable, libraries and object files use:

          $ cd connectedhomeip/examples/light-switch-app/talaria/
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
          $ sudo python3 script/boot.py --device /dev/ttyUSB2 --reset=evk42_bl ./apps/gordon.elf
- Run the following command to update the partition table

          $ cd <FreeRTOS_sdk_3.0_master_matter-root-directory>
          $ sudo python3 ./script/flash.py --device /dev/ttyUSB2 from_json ./tools/partition_files/matter_demo_partition.json
          
#### Programming image elf
- Connect T2 EVB with the system
- Run the T2DownloadTool_Linux tool with sudo access from FreeRTOS_sdk_3.0_master_matter/pc_tools/Download_Tool/bin/ and select following fields in the GUI tool
- Boot Target: Select the appropriate EVK from the drop-down
- ELF Input: Load the out/test/light-switch-app.elf by clicking on Select ELF File
- Boot Arguments: Pass the following boot arguments

          matter.discriminator=1122 matter.commissioning.flow_type=0 suspend=1 matter.factory_reset=0
- Programming: Prog RAM or Prog Flash as per requirement.
**For more details on using the Download tool, refer to the document: UG_Download_Tool.pdf (path:
FreeRTOS_sdk_3.0_master_matter/pc_tools/Download_Tool/doc/UG_Download_Tool.pdf).**

## Commissioning and Controlling the device
- Run generated chip-tool binary using following commands. Steps to build chip-tool is provided in FreeRTOS_sdk_3.0_master_matter/matter/README.md

          $ out/debug/chip-tool interactive start
          >>> pairing ble-wifi <node-id> <wifi-ssid> <wifi-passwd> 20202021 <discriminator>
          e.g. pairing ble-wifi 2222 tplinkc6_iop InnoQA2023$ 20202021 1122
- Once the commissioning is completed successfully you can use following commands to create the binding of Light Switch Application and Lighting Application. For the following commands it has been considered that the lighting application is commissioned with node-id 1111 and light-switch application is commissioned with node-id 2222 by the same chip-tool.

          ## Command to give the access to Light-Switch node
          >>> accesscontrol write acl '[{"privilege": 5, "authMode": 2, "subjects": [ 112233, 2222 ], "targets": null}]' 1111 0x0
          ## Command to create binding with lighting application
          >>> binding write binding '[{"node":1111, "endpoint":1, "cluster":6}]' 2222 0x1
- Once the AccessControl and binding commands are successful, the LED of Lighting Application can be controlled by Changing the state of GPIO18 of Light-Switch Application.

          # Connect the GPIO18 to GND using a jumper to turn off the light
          # Disconnect the GPIO18 from GND to turn on the light
