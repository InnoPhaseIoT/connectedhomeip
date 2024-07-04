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
-   `matter.enable_cli=<value>`

    To enable/disable the Command line Interface(CLI).
    0: Disable Command line Interface (Default)
    1: Enable Command line Interface

    Note that with CLI interface enabled DUT may not go in suspend state.

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
          e.g. pairing ble-wifi 2222 tplinkc6_iop InnoQA2023$ 20202021 1122
- Once the commissioning is completed successfully you can use following commands to create the binding of Light Switch Application and Lighting Application. For the following commands it has been considered that the lighting application is commissioned with node-id 1111 and light-switch application is commissioned with node-id 2222 by the same chip-tool.

          ## Command to give the access to Light-Switch node
          >>> accesscontrol write acl '[{"privilege": 5, "authMode": 2, "subjects": [ 112233, 2222 ], "targets": null}]' 1111 0x0
          ## Command to create binding with lighting application
          >>> binding write binding '[{"node":1111, "endpoint":1, "cluster":6}]' 2222 0x1
- Once the AccessControl and binding commands are successful, the LED of Lighting Application can be controlled by Changing the state of GPIO18 of Light-Switch Application.

          # Connect the GPIO18 to GND using a jumper to turn off the light
          # Disconnect the GPIO18 from GND to turn on the light
## Dimmer Switch Usage:
#### Configuration:
- To enable the Dimmer Switch functionality, set "ENABLE_DIMMER_SWITCH" to 1 in the "inc/LightSwitch_ProjecConfig.h" configuration file.
- To Control Level-Control cluster in lighting-application through binding commands, enable the dimmer switch functionality and then set "ENABLE_LEVEL_CONTROL" to 1 in the "inc/LightSwitch_ProjecConfig.h" configuration file.
- To Control Colour-Control cluster in lighting-application through binding commands, enable the dimmer switch functionality and then set "ENABLE_COLOUR_CONTROL" to 1 in the "inc/LightSwitch_ProjecConfig.h" configuration file.

#### Implementation and Controlling:
- Implemented Dimmer Switch functionality for 4 switch positions (can be configurable as per requirement).
- Utilized ADC to get input voltage and map it to switch position values to perform switch actions and for updating current switch position status every 10sec(can be configurable) from software timer callback as actual dimmer switch is not available.
    - Enable by setting 'SWITCH_CONTROL_USING_ADC' to 1
    - Maximum ADC input voltage is 1V.
    - Implemented 4 switch positions as below (can be configurable)
        - 0V    (Switch Position - 0)
        - 0.25V (Switch Position - 1)
        - 0.5V  (Switch Position - 2)
        - 0.75V (Switch Position - 3)
        - 1.0V  (Switch Position - 4)
        - Connect input to ADC PIN and vary the input voltage between 0 and 1V to change the switch positions.
- Utilized rand function to generate random switch position values from 0 to 4 to perform switch actions and for updating current switch position status every 10sec(can be configurable) from software timer callback as actual Dimmer switch is not available.
- Dimmer switch utilizes Endpoint ID 2 in Switch Cluster.

                ## Command to read current switch position
                >>> switch read current-position 2222 2
                ## Command to read number of switch positions
                >>> switch read number-of-positions 2222 2
                ## Command to subscribe for current switch position
                >>> switch subscribe current-position 10 10 2222 2
                ## Command to subscribe for number of switch positions
                >>> switch subscribe number-of-positions 10 10 2222 2


- Once the commissioning is completed successfully you can use following commands to create the binding of Light Switch Application and Lighting Application. For the following commands it has been considered that the lighting application is commissioned with node-id 1111 and light-switch application is commissioned with node-id 2222 by the same chip-tool.

          ## Command to give the access to Light-Switch node.
          >>> accesscontrol write acl '[{"privilege": 5, "authMode": 2, "subjects": [ 112233, 2222 ], "targets": null}]' 1111 0x0
          ## Command to create binding with Level-Control cluster in lighting application
          >>> binding write binding '[{"node":1111, "endpoint":1, "cluster":8}]' 2222 0x1
          ## Command to create binding with Colour-Control cluster in lighting application
          >>> binding write binding '[{"node":1111, "endpoint":1, "cluster":768}]' 2222 0x1
          ## Command to create binding with both Level and Colour Control clusters in lighting application
          >>> binding write binding '[{"node":1111, "endpoint":1, "cluster":8}, {"node":1111, "endpoint":1, "cluster":768}]' 2222 0x1
          ## Command to create binding with OnOff, Level and Colour Control clusters in lighting application
          >>> binding write binding '[{"node":1111, "endpoint":1, "cluster":6}, {"node":1111, "endpoint":1, "cluster":8}, {"node":1111, "endpoint":1, "cluster":768}]' 2222 0x1
- Once the AccessControl and binding commands are successful, OnOff/ Level-Control/ Color-Control Applications can be controlled by changing the state of Switch in light-switch application based on configuration.

#### Command Line Interface:
- Open the /dev/ttyUSB2 or equivalent port in windows. NOTE: Usage of CLI must be enabled using boot argument matter.enable_cli=1.
- The baudrate the port should be open is 2457600
- The commands used in CLI are as below:

		## Command to on light in lighting-app
		>>> onoff on
		## Command to off light in lighting-app
		>>> onoff off
		## Command to toggle light in lighting-app
		>>> onoff toggle
