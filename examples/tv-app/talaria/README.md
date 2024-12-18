# CHIP Talaria Basic Video Player Example

An example showing the use of CHIP on the Talaria Two platform. The document will describe how
to build and run CHIP Basic Video Player Example on Talaria Two platform.

This doc is tested on **Ubuntu 22.04.3 LTS (x86_64)**

## Building

-   Setup the environment by running the boostrap.sh script as menionend in the FreeRTOS_sdk_3.0_master_matter/matter/README.md **Building the Example application** secion

-   Build the example application:

          $ cd connectedhomeip/examples/tv-app/talaria
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make

-   To delete generated executable, libraries and object files use:

          $ cd connectedhomeip/examples/tv-app/talaria
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

-   `hio.transport=spi disp_pkt_info=1 hio.maxsize=512 hio.baudrate=2560000`

    These Boot arguments are mandatory for basic-video-player-app to work on the Nuvoton setup for communication with host



## Programming the Example on T2 of Nuvoton Setup
The Host package for matter is required for the following steps.

#### Programming T2 of LPS setup
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
3. Generate the app.img and app.img.vm from the generated ELF using following command and program the same

         $ cd connectedhomeip/examples/tv-app/talaria/
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/boot.py --output ./out/app.img out/test/basic-video-player-app.elf hio.transport=spi disp_pkt_info=1 hio.maxsize=512 hio.baudrate=2560000 matter.commissioning.flow_type=1 matter.factory_reset=0
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/boot.py --device /dev/ttyACM0 <path to FreeRTOS_sdk_3.0_master_matter>/apps/gordon.elf
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 from_json <path to FreeRTOS_sdk_3.0_master_matter>/tools/partition_files/matter_demo_partition.json
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 write 0x1000 out/app.img
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 write 0x40000 out/app.img.vm
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 write 0x140000 <path to FreeRTOS_sdk_3.0_master_matter>/root_fs/root.img
4. For information on programming the Host with matter_app.bin, refer section: document: Application_for_Matter.pdf (host\doc\application_notes\Application_for_Matter.pdf)

## Commissioning and Controlling the device
- Run generated chip-tool binary using following commands. Steps to build chip-tool is provided in FreeRTOS_sdk_3.0_master_matter/matter/README.md

          $ out/debug/chip-tool interactive start
          >>> pairing ble-wifi <node-id> <wifi-ssid> <wifi-passwd> 20202021 <discriminator>
          e.g. pairing ble-wifi 1111 tplinkc6_iop InnoQA2023$ 20202021 1122
- Once the commissioning is completed successfully you can use following commands to perform basic video player operations on Talaria Two EVB. Here the 1111 is the node-id given at the time of commissioning to the device
          
          ## OnOff command to control the media-playback device
          >>> onoff <command> <node-id>  <endpoint-id>
          e.g. onoff on 1111 1
          e.g. onoff off 1111 1
          ## OnOff command to read the Media-Playback device On/Off status
          e.g. onoff read on-off 1111 1
          ## Command to subcribe for Media-Playback On/Off attribute status
          >>> onoff subscribe <min-interval> <max-interval> <attribute-name> <node-id> <endpoint-id>
          e.g. onoff subscribe on-off 5 10 1111 1
          ## Command to control the Media-Playback
          >>> mediaplayback <command> <node-id> <endpoint-id>
          e.g. mediaplayback play 1111 1
          e.g. mediaplayback pause 1111 1
          e.g. mediaplayback stop 1111 1
          e.g. mediaplayback start-over 1111 1
          e.g. mediaplayback previous 1111 1
          e.g. mediaplayback next 1111 1
          e.g. mediaplayback rewind 1111 1
          e.g. mediaplayback fast-forward 1111 1
          e.g. mediaplayback skip-forward 1000 1111 1
          e.g. mediaplayback skip-backward 1000 1111 1
          e.g. mediaplayback seek 1000 1111 1
          ## Command to read the Media-Playback attribute values
          >>> mediaplayback read <attribute-name> <node-id> <endpoint-id>
          e.g. mediaplayback read current-state 1111 1
          e.g. mediaplayback read start-time 1111 1
          e.g. mediaplayback read duration 1111 1
          e.g. mediaplayback read sampled-position 1111 1
          e.g. mediaplayback read playback-speed 1111 1
          e.g. mediaplayback read seek-range-end 1111 1
          e.g. mediaplayback read seek-range-start 1111 1
          ## Command to subcribe for Media-Playback attribute values
          >>> mediaplayback subscribe <min-interval> <max-interval> <attribute-name> <node-id> <endpoint-id>
          e.g. mediaplayback subscribe current-state 5 10 1111 1
          e.g. mediaplayback subscribe start-time 5 10 1111 1
          e.g. mediaplayback subscribe duration 5 10 1111 1
          e.g. mediaplayback subscribe sampled-position 5 10 1111 1
          e.g. mediaplayback subscribe playback-speed 5 10 1111 1
          e.g. mediaplayback subscribe seek-range-end 5 10 1111 1
          e.g. mediaplayback subscribe seek-range-start 5 10 1111 1
          ## Command to send input-key
          >>> keypadinput <command> <Key-Code> <node-id> <endpoint-id>
          e.g. keypadinput send-key 0x5 1111 1

## Factory Reset Flow
1. To trigger factory reset 1 flow, press and hold the user button of host (M2354) for more than 10 seconds.
2. To trigger factory reset 2 flow, press and hold the user button, press the reset button of host (M2354).
