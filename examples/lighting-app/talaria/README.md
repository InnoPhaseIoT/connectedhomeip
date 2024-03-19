# CHIP Talaria Lighting Example

An example showing the use of CHIP on the Talaria Two platform. The document will describe how
to build and run CHIP Lighting Example on Talaria Two platform. This doc is tested
on **Ubuntu 22.04.3 LTS (x86_64)**

## Building

-   Setup the environment by running the boostrap.sh script as menionend in the FreeRTOS_sdk_3.0_master_matter/matter/README.md **Building the Example application** secion

-   Build the example application:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make EXT_FLASH=true

-   To delete generated executable, libraries and object files use:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ make clean

## Boot arguments
-   `matter.commissioning.flow_type=<value>`

    0: Standard commissioning flow (Default)
    1: User-intended commissioning flow. To enable this, GPIO3 should be connected to GND for 5 seconds and then disconnected from GND

-   `suspend=<value>`

    Puts the device in suspend state
    0: Disable suspend (Default)
    1: Enable suspend

-   `matter.enable_factory_data_provider=<value>`

    To enable usage of generated certificates. Make sure the certificates are pushed in the data partition. For more information on certificate generate refer from FreeRTOS_sdk_x.x/matter/README.md section #Generating Factory Configuraion Data using the generated certificates
    0: Uses chip example certificate (Default)
    1: Uses configured generated certificates

-   `matter.enable_device_instance_info_provider=<value>`

    To enable configuration of vendor-id and product-id etc. through filesystem. This option will have effect only if matter.enable_factory_data_provider boot argument value is set to 1.
    To generate the data parition content refer from FreeRTOS_sdk_x.x/matter/README.md section #Generating Factory Configuraion Data using the generated certificates
    0: Uses default vendor-id and product-id details (Default)
    1: Uses configured vendor-id and product-id details

-   `matter.factory_reset=<value>`

    To enable/disable the flow for factory resetting the device.
    0: Disable Factory Reset (Default)
    1: Reset to Factory Defaults (e.g. chip-counters and chip-config content will be reset)
    2: Enable full factory reset (In this case, contents of the data partition table (SSID, passphrase, chip-tool keys) on     Talaria TWO will be erased)
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
- ELF Input: Load the out/test/on-off-light-app.elf by clicking on Select ELF File
- Boot Arguments: Pass the following boot arguments

          matter.commissioning.flow_type=0 suspend=1 matter.factory_reset=0
- Programming: Prog RAM or Prog Flash as per requirement.
**For more details on using the Download tool, refer to the document: UG_Download_Tool.pdf (path:
FreeRTOS_sdk_3.0_master_matter/pc_tools/Download_Tool/doc/UG_Download_Tool.pdf).**

#### Programming Factory Data
- To generate the Factory data for application refer steps from FreeRTOS_sdk_3.0_master_matter/matter/README.md #'Generating Factory Configuraion Data'
- Once the factory data is generated (Considered that it has been generated at FreeRTOS_sdk_3.0_master_matter/data), select the path to the FreeRTOS_sdk_3.0_master_matter/data directory in "Write Files from a Directory:" field
- Click on "Write Files" Button and wait for the data to be written successfuly
- Press the "Reset" button to see the effect of the same

## Commissioning and Controlling the device
- Run generated chip-tool binary using following commands. Steps to build chip-tool is provided in FreeRTOS_sdk_3.0_master_matter/matter/README.md

          $ out/debug/chip-tool interactive start
          >>> pairing ble-wifi <node-id> <wifi-ssid> <wifi-passwd> 20202021 <discriminator>
          e.g. pairing ble-wifi 1111 tplinkc6_iop InnoQA2023$ 20202021 1122
- Once the commissioning is completed successfully you can use following commands to control the D1 LED of Talaria Two EVB. Here the 1111 is the node-id given at the time of commissioning to the device

          >>> onoff on 1111 1     ## To Turn on the LED
          >>> onoff off 1111 1     ## To Turn off the LED
          >>> onoff toggle 1111 1     ## To toggle the LED
          >>> identify read identify-type 1111 1 # To know the identify-type our case it is light output
          >>> identify write identify-time <seconds-to-blink> 1111 1 # This command is used to blink led to identify itself for <senonds-to-blink> seconds
	  >>> identify read identify-time 1111 1 # This command is used to know the seconds left to blink the led. 
