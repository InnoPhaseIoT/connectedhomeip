# CHIP Talaria Lighting Example

An example showing the use of CHIP on the Talaria Two platform. The document will describe how
to build and run CHIP Lighting Example on Talaria Two platform. This doc is tested
on **Ubuntu 22.04.3 LTS (x86_64)**

## Building

-   Setup the environment by running the boostrap.sh script as menionend in the FreeRTOS_sdk_3.0_master_matter/matter/README.md **Building the Example application** secion

### Building Matter Lighting Application
The lighting application can be built in multiple modes as per the use case for Talaria TWO EVB:

- Standalone app without SSBL (Second Stage Boot Loader): For non FOTA support usecase
- Lighting application for non-secure SSBL mode: FOTA support with SSBL with non-secureboot
- Lighting application for secure SSBL mode: FOTA support with SSBL with secureboot

Common make command usage with this application:
-   To delete generated executable, libraries and object files use. Cleaning is must when you are planning tobuild application with different compile time flags:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ make clean
- FOTA will be enabled by default when building the Matter Lighting Application. To disable FOTA, build the example application with "make ENABLE_OTA_REQUESTOR=false".

#### Standalone app without SSBL
-   Build the example application to be used without SSBL:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make clean && make

#### Lighting application for non-secure SSBL mode
-   Build the example application to be used with non secure SSBL:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make clean && make EXT_FLASH=true

#### Lighting application for secure SSBL mode
-   Build the example application to be used with secure SSBL:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make clean && make EXT_FLASH=true SECURED=true


### Building Matter Lighting Application + AWS/MQTT/HTTP Application

#### Building Matter Lighting Application + HTTP/HTTPS Application
-   Build the example application:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make clean && make ENABLE_HTTP_APP=true

-   To delete generated executable, libraries and object files use:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ make clean

- FOTA will be disabled when building the Matter lighting application with the HTTP/HTTPS application.
- Compilation/build will fail if more than one application is enabled.

#### Building Matter Lighting Application + MQTT Application
-   Build the example application:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make clean && make ENABLE_MQTT_APP=true

-   To delete generated executable, libraries and object files use:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ make clean

- FOTA will be disabled when building the Matter lighting application with the MQTT application.
- Compilation/build will fail if more than one application is enabled.

#### Building Matter Lighting Application + AWS-IoT Subscribe and Publish Application
-   Note: The AWS IoT SDK for Talaria TWO Platform needs be set up before compiling the application,
    Please refer to the document below for setup instructions for the AWS IoT SDK for Talaria TWO Platform ("talaria_two_aws" repository).
    (Document: "<freertos_sdk_x.y>/apps/iot_aws/doc/Application_for_IoT_AWS.pdf",
    Section: "AWS IoT Device SDK Embedded C On Talaria TWO Platform")

-   Build the example application:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make clean && make ENABLE_AWS_IOT_APP=true

-   To delete generated executable, libraries and object files use:

          $ cd connectedhomeip/examples/lighting-app/talaria
          $ make clean

- FOTA will be disabled when building the Matter lighting application with the AWS-IoT application.
- Compilation/build will fail if more than one application is enabled.


## Boot arguments

### Boot arguments for Matter Lighting Application
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

### Boot arguments for Matter Lighting Application + AWS/MQTT/HTTP Application

#### Boot arguments for Matter Lighting Application + HTTP/HTTPS Application
- Refer to the instructions in the Example_using_HTTP_Client.pdf document found in the freertos_sdk_x.y/examples/http_client/doc directory, sections "Running the Application" and "Using the Application."
- Matter lighting application boot arguments should be passed along with the HTTP/HTTPS application boot arguments.

#### Boot arguments for Matter Lighting Application + MQTT Application
- Refer to the instructions in the Example_for_MQTT.pdf document found in the freertos_sdk_x.y/ examples/mqtt/doc directory, sections "Running the Application using Mosquitto Project’s Test Server and Evaluating the Application using Mosquitto Local Server"
- Matter lighting application boot arguments should be passed along with the MQTT application boot arguments.

#### Boot arguments for Matter Lighting Application + AWS-IoT Subscribe and Publish Application
- Refer to the instructions in the Application_for_IoT_AWS.pdf document found in the freertos_sdk_x.y/apps/iot_aws/doc directory, sections "AWS Set-up" and "MQTT Publish and Subscribe"
- Matter lighting application boot arguments should be passed along with the AWS-IoT subscribe and publish application boot arguments.

## Programming the Example on Talaria Two Platform
### Programming steps for Standalone app without SSBL
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
- ELF Input: Load the out/test/lighting-app.elf by clicking on Select ELF File
- Boot Arguments: Pass the following boot arguments

          matter.commissioning.flow_type=0 suspend=1 matter.factory_reset=0
- Programming: Prog RAM or Prog Flash as per requirement.
**For more details on using the Download tool, refer to the document: UG_Download_Tool.pdf (path:
FreeRTOS_sdk_3.0_master_matter/pc_tools/Download_Tool/doc/UG_Download_Tool.pdf).**

#### Programming Factory Data
- To generate the Factory data for application refer steps from connectedhomeip/docs/guides/talaria/README.md #'Generating Factory Configuraion Data'
- Once the factory data is generated (Considered that it has been generated at FreeRTOS_sdk_3.0_master_matter/data), select the path to the FreeRTOS_sdk_3.0_master_matter/data directory in "Write Files from a Directory:" field
- Click on "Write Files" Button and wait for the data to be written successfuly
- Press the "Reset" button to see the effect of the same

### Programming steps for App with secureboot and non-secureboot SSBL
- Refer to the steps from FreeRTOS_sdk/apps/ssbl_spi_ext_flash/README.md
- Generating Facory Data steps can be referred from connectedhomeip/docs/guides/talaria/README.md #'Generating Factory Configuraion Data' section

## Commissioning and Controlling the device
- Run generated chip-tool binary using following commands. Steps to build chip-tool is provided in FreeRTOS_sdk_3.0_master_matter/matter/README.md

          $ out/debug/chip-tool interactive start
          >>> pairing ble-wifi <node-id> <wifi-ssid> <wifi-passwd> 20202021 <discriminator>
          e.g. pairing ble-wifi 1111 tplinkc6_iop InnoQA2023$ 20202021 1122
- Once the commissioning is completed successfully you can use following commands to control the D1 LED of Talaria Two EVB. Here the 1111 is the node-id given at the time of commissioning to the device
- Note: Programming T2 EVB with a factory_reset=2, removes the kvs and persisted attributes. On next factory_reset=0 boot, the light will turn on with the brightness set to default value 0. Therefore, set the brightness level first using the level control commands mentioned below, and then turn on the light.

          >>> onoff on 1111 1     ## To Turn on the LED
          >>> onoff off 1111 1     ## To Turn off the LED
          >>> onoff toggle 1111 1     ## To toggle the LED
          >>> identify read identify-type 1111 1 # To know the identify-type our case it is light output
          >>> identify write identify-time <seconds-to-blink> 1111 1 # This command is used to blink led to identify itself for <senonds-to-blink> seconds
	  >>> identify read identify-time 1111 1 # This command is used to know the seconds left to blink the led. 


#### Level Control Commands:


          ## Command to set the brightness level without transition
          >>> levelcontrol move-to-level <Level> <TransitionTime> <OptionsMask> <OptionsOverride> <destination-id> <endpoint-id>
          e.g. levelcontrol move-to-level 100 0 1 1 1111 1
          ## Command to set the brightness level with on-off command
          >>> levelcontrol move-to-level-with-on-off <Level> <TransitionTime> <OptionsMask> <OptionsOverride> <destination-id> <endpoint-id>
          e.g. levelcontrol move-to-level-with-on-off 100 0 1 1 1111 1
          ## Command to read the current level
          >>> levelcontrol read <attribute-name> <destination-id> <endpoint-id>
          e.g. levelcontrol read current-level 1111 1
          ## Command to subscribe for current level
          >>> levelcontrol subscribe <attribute-name> <min-interval> <max-interval> <destination-id> <endpoint-id>
          e.g. levelcontrol subscribe current-level 5 10 1111 1


#### Color Control Commands:


          ## Command to set the hue level
          >>> colorcontrol move-to-hue <Hue> <Direction TransitionTime> <OptionsMask> <OptionsOverride> <destination-id> <endpoint-id>
          e.g. colorcontrol move-to-hue 40 1 1 1 1 1111 1
          ## Command to set the saturation level
          >>> colorcontrol move-to-saturation <Saturation> <TransitionTime> <OptionsMask> <OptionsOverride>  <destination-id> <endpoint-id>
          e.g. colorcontrol move-to-saturation 60 1 1 1 1111 1
           ## Command to set both hue and saturation levels
          >>> colorcontrol move-to-hue-and-saturation <Hue> <Saturation> <TransitionTime> <OptionsMask> <OptionsOverride>  <destination-id> <endpoint-id>
          e.g. colorcontrol move-to-hue-and-saturation 40 60 1 1 1 1111 1
          ## Command to read the current hue/saturaion
          >>> colorcontrol read <attribute-name> <destination-id> <endpoint-id>
          e.g. colorcontrol read current-hue 1111 1
          e.g. colorcontrol read current-saturation 1111 1
          ## Command to subscribe for current hue/saturaion
          >>> colorcontrol subscribe <attribute-name> <min-interval> <max-interval> <destination-id> <endpoint-id>
          e.g. colorcontrol subscribe current-hue 5 10 1111 1
          e.g. colorcontrol subscribe current-saturation 5 10 1111 1

#### Occupancy-Sensor Commands:


          ## Command to read the current occupancy status
          >>> occupancysensing read <attribute-name> <destination-id> <endpoint-id>
          e.g. occupancysensing read occupancy 1111 1
          ## Command to subscribe for current occupancy status
          >>> occupancysensing subscribe <attribute-name> <min-interval> <max-interval> <destination-id> <endpoint-id>
          e.g. occupancysensing subscribe occupancy 5 10 1111 1

### Controlling the device when HTTP/HTTPS App Enabled
 - HTTP/HTTPS application functionality is enabled only when compiled with ENABLE_HTTP_APP=true.
 - The HTTP/HTTPS application will start only after successful commissioning and Wi-Fi connectivity is established, i.e., the server is ready.
 - Refer to the instructions in the Example_using_HTTP_Client.pdf document found in the freertos_sdk_x.y/examples/http_client/doc directory, sections "Running the Application" and "Using the Application," for using and controlling the HTTP/HTTPS app functionality.
 

### Controlling the device when MQTT App Enabled
 - MQTT application functionality is enabled only when compiled with ENABLE_MQTT_APP=true.
 - The MQTT application will start only after successful commissioning and Wi-Fi connectivity is established, i.e., the server is ready.
 - Refer to the instructions in the Example_for_MQTT.pdf document found in the freertos_sdk_x.y/ examples/mqtt/doc directory, sections "Running the Application using Mosquitto Project’s Test Server and Evaluating the Application using Mosquitto Local Server" for using and controlling the MQTT app functionality.
 


### Controlling the device using AWS-IoT Subscribe and Publish
 - AWS-IoT subscribe and publish application functionality is enabled only when compiled with ENABLE_AWS_IOT_APP=true.
 - The AWS-IoT Subscribe and Publish application will start only after successful commissioning and Wi-Fi connectivity is established, i.e., the server is ready.
 - Refer to the instructions in the Application_for_IoT_AWS.pdf document found in the freertos_sdk_x.y/apps/iot_aws/doc directory, sections "AWS Set-up" and "MQTT Publish and Subscribe" for using and controlling the AWS-IoT subscribe and publish app functionality.
    - Note: Follow the AWS-IoT subscribe and publish message formats mentioned in the sections below, not as per the above document.
 - The lighting device can be controlled by sending commands using AWS-IoT Publish.
   [AWS server will publish the commands --> T2 will subscribe to these commands]
 - The lighting device can update its status using AWS-IoT Subscribe.
   [T2 will publish the status updates --> AWS server will subscribe to the status updates]


#### Send Lighting Commands: [AWS server will publish the commands --> T2 will subscribe to these commands]
 - Users can turn the light ON or OFF by sending commands from the AWS-IoT server.

        $ Command to Turn ON Light:

        {
        "from": "AWS IoT console"
        "to": "T2"
        "msg": "Turn On Light"
        }

        $ Command to Turn OFF Light:

        {
        "from": "AWS IoT console"
        "to": "T2"
        "msg": "Turn Off Light"
        }

 - Upon receiving the command from the AWS-IoT server, T2 will perform the light turn ON/OFF and the status will be published to the AWS-IoT server as described below.


#### Update Lighting Status [T2 will publish the status updates --> AWS server will subscribe to the status updates]
 - T2 will send lighting turn ON/OFF status updates to the AWS-IoT server as follows:

        $ Status of Turn ON Light:

        {
        "from": "Talaria T2",
        "to": "AWS",
        "msg": "Light status On",
        "msg_id": 1
        }

        $ Status of Turn OFF Light:

        {
        "from": "Talaria T2",
        "to": "AWS",
        "msg": "Light status Off",
        "msg_id": 2
        }
