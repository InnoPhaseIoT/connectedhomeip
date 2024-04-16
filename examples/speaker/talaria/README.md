# CHIP Talaria Speaker Example

An example showing the use of CHIP on the Talaria Two platform. The document will describe how
to build and run CHIP Speaker Example on Talaria Two platform.

This doc is tested on **Ubuntu 22.04.3 LTS (x86_64)**

## Building

-   Setup the environment by running the boostrap.sh script as menionend in the FreeRTOS_sdk_3.0_master_matter/matter/README.md **Building the Example application** secion

-   Build the example application:

          $ cd connectedhomeip/examples/speaker/talaria
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make

-   To delete generated executable, libraries and object files use:

          $ cd connectedhomeip/examples/speaker/talaria
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

-   `hio.transport=spi disp_pkt_info=1 hio.maxsize=512 hio.baudrate=2560000`

    These Boot arguments are mandatory for speaker-app to work on the Nuvoton setup for communication with host


## Programming the Example on T2 of Nuvoton Setup
The host package for matter is required for the following steps.

#### Programming T2 of Nuvoton setup
1. Connect the Nuvoton board to PC using USB C-type cable.
2. Generate the factory data image using following commands. To create the factory data follow the steps from FreeRTOS_sdk_3.x_master_matter/matter/README.md #'Generating Factory Configuraion Data' section.
NOTE: Here it's considered that factory data is created inside 'FreeRTOS_sdk_3.x_master_matter/data' directory.

        $ cd ../
        FreeRTOS_sdk_3.x_master_matter$ cp -r ./data/* ./root_fs/root/
        FreeRTOS_sdk_3.x_master_matter$ python3 ./script/build_rootfs_generic.py

        ### Expected output
        root folder to generate root img is from  root_fs
        Creating checksum files...
        creating root img
        /part.checksum
        /boot.checksum
        /boot.json
        /part.json
        /chip-factory/discriminator
        /chip-factory/loc-capability
        /chip-config/reg-location
        /fota_config.checksum
        /dirty
        /chip-counters/total-hours
        /chip-counters/up-time
        /chip-counters/boot-reason
        /chip-counters/boot-count
        /fota_config.json
        copied root.img to root_fs
7. Generate the app.img and app.img.vm from the generated ELF using following command and program the same

         $ cd connectedhomeip/examples/speaker/talaria/
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/boot.py --output ./out/app.img out/test/speaker-app.elf hio.transport=spi disp_pkt_info=1 hio.maxsize=512 hio.baudrate=2560000 matter.commissioning.flow_type=1 matter.factory_reset=0
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/boot.py --device /dev/ttyACM0 <path to FreeRTOS_sdk_3.0_master_matter>/apps/gordon.elf
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 from_json <path to FreeRTOS_sdk_3.0_master_matter>/tools/partition_files/matter_demo_partition.json
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 write 0x1000 out/app.img
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 write 0x40000 out/app.img.vm
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 write 0x140000 <path to FreeRTOS_sdk_3.0_master_matter>/root_fs/root.img
7. For information on programming the Host with matter_app.bin, refer section: Application_for_Matter.pdf (host\doc\application_notes\Application_for_Matter.pdf)

## Commissioning and Controlling the device
- Run generated chip-tool binary using following commands. Steps to build chip-tool is provided in FreeRTOS_sdk_3.0_master_matter/matter/README.md

          $ out/debug/chip-tool interactive start
          >>> pairing ble-wifi <node-id> <wifi-ssid> <wifi-passwd> 20202021 <discriminator>
          e.g. pairing ble-wifi 1111 tplinkc6_iop InnoQA2023$ 20202021 1122
- Once the commissioning is completed successfully you can use following commands to perform speaker operations on Talaria Two EVB. Here the 1111 is the node-id given at the time of commissioning to the device


          ## OnOff commands to control the speaker
          >>> onoff <command> <node-id> <endpoint-id>
          e.g. onoff on 1111 1
          e.g. onoff off 1111 1
          ## Commands to read current status of OnOff cluster attributes
          >>> onoff read <attribute-name> <node-id> <endpoint-id>
          e.g. onoff read on-off 1111 1
          ## Commands to subscribe for OnOff cluster attributes status
          >>> onoff subscribe <attribute-name> <min-interval> <max-interval> <destination-id> <endpoint-id>
          e.g. onoff subscribe on-off 5 10 1111 1
          ## Level control commands to control the volume of speaker
          >>> levelcontrol <command> <Level> <TransitionTime> <OptionsMask> <OptionsOverride> <node-id> <endpoint-id>
          e.g. levelcontrol move-to-level 120 0 1 1 1111 1
          ## Commands to read current status of level control cluster attributes
          >>> levelcontrol read <attribute-name> <node-id> <endpoint-id>
          e.g. levelcontrol read current-level 1111 1
          ## Commands to subscribe for level control cluster attributes status
          >>> levelcontrol subscribe <attribute-name> <min-interval> <max-interval> <destination-id> <endpoint-id>
          e.g. levelcontrol subscribe current-level 5 10 1111 1

