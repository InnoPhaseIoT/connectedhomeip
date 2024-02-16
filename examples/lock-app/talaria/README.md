# CHIP Talaria Lock Example

An example showing the use of CHIP on the Talaria Two Nuvoton LPS platform. The document will describe how
to build and run CHIP Lock Example on Talaria Two Nuvoton LPS platform. This doc is tested
on **Ubuntu 22.04.3 LTS (x86_64)**

## Building

-   Setup the environment by running the boostrap.sh script as menionend in the FreeRTOS_sdk_3.0_master_matter/matter/README.md **Building the Example application** secion

-   Build the example application:

          $ cd connectedhomeip/examples/lock-app/talaria/
          $ source third_party/connectedhomeip/scripts/activate.sh ## If not activated already
          $ make

-   To delete generated executable, libraries and object files use:

          $ cd connectedhomeip/examples/lock-app/talaria/
          $ make clean

## Boot arguments

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

-   `hio.transport=spi disp_pkt_info=1 hio.maxsize=512 hio.baudrate=2560000`

    These Boot arguments are mandatory for lock-app to work on the Nuvoton LPS setup for communication with host
    
## Programming the Example on T2 of Nuvoton LPS Setup
The LPS package for matter is required for the following steps.

#### Programming T2 of LPS setup
1. Connect the LPS board to PC using USB C-type cable.
2. Switch the board to DFU mode by power cycling the board with DFU button pressed.
Note: The OLED screen will be blank in this mode.
3. Flash Talaria TWO using the LPS flashing tool from lps_adk package
(lps_adk/tools/flashing_tools/lps_gui/LPS_Flashing_Tool_Linux).
Note: Run the flashing tool with sudo access
4. Under Program Nuvoton Host tab, select recov_app.bin in the Select App Binary file
field, click on Program.
5. Once successfully programmed, press the Reset button on the board. This will display “Recovery
App V 1.0” and “T2 Program Mode” on the OLED display.
6. Generate the factory data image using following commands. To create the factory data follow the steps from FreeRTOS_sdk_3.x_master_matter/matter/README.md #'Generating Factory Configuraion Data' section.
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

         $ cd connectedhomeip/examples/lock-app/talaria/
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/boot.py --output ./out/app.img out/test/lock-app.elf hio.transport=spi disp_pkt_info=1 hio.maxsize=512 hio.baudrate=2560000 matter.commissioning.flow_type=1 matter.factory_reset=0
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/boot.py --device /dev/ttyACM0 <path to FreeRTOS_sdk_3.0_master_matter>/apps/gordon.elf
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 from_json <path to FreeRTOS_sdk_3.0_master_matter>/tools/partition_files/matter_demo_partition.json
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 write 0x1000 out/app.img
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 write 0x40000 out/app.img.vm
         $ python3 <path to FreeRTOS_sdk_3.0_master_matter>/script/flash.py --device /dev/ttyACM0 write 0x140000 <path to FreeRTOS_sdk_3.0_master_matter>/root_fs/root.img
7. For information on programming the Host with matter_app.bin, refer section: Using the LPS GUI Flashing
Tool, of the document: Application_for_Matter.pdf (matter_alpha_v1.0\hosted_matter\doc\application_notes\Application_for_Matter)


## Commissioning and Controlling the device
- Put the device into commissioning mode by pressing the reworked switch on the LPS setup for at least 3 seconds and release
- Run generated chip-tool binary using following commands. Steps to build chip-tool is provided in FreeRTOS_sdk_3.0_master_matter/matter/README.md

          $ out/debug/chip-tool interactive start
          >>> pairing ble-wifi <node-id> <wifi-ssid> <wifi-passwd> 20202021 <discriminator>
          e.g. pairing ble-wifi 1111 tplinkc6_iop InnoQA2023$ 20202021 1122
- Once the commissioning is completed successfully you can use following commands to control the lock application. Here this commands considers that the device is commissioned with node-id 1111.

          ## Command to Unlock the door
          >>> doorlock unlock-door 1111 1 --timedInteractionTimeoutMs 999
          ## Command to Lock the door
          >>> doorlock lock-door 1111 1 --timedInteractionTimeoutMs 1000
          ## Command to Unlock the door with timeout
          >>> doorlock unlock-with-timeout 5 1111 1 --timedInteractionTimeoutMs 1000
          ## Command to Set the User
          >>> doorlock set-user 0 1 mike 5 1 0 0 1111 1 --timedInteractionTimeoutMs 1000
          ## Command to Get the User
          >>> doorlock get-user 1 1111 1
          ## Command to Set the Credential
          >>> doorlock set-credential 0 '{ "credentialType": 1, "credentialIndex": 1 }' 123456 1 null null 1111 1 --timedInteractionTimeoutMs 1000
          ## Command to get the credential
          >>> doorlock get-credential-status '{ "credentialType": 1, "credentialIndex": 1}' 1111 1
          ## Command to Unlock the Door using PIN
          >>> doorlock unlock-door 1111 1 --timedInteractionTimeoutMs 1000 --PINCode 123456
          ## Command to Clear the Credential. This clears the user info which is bound to this credential
          >>> doorlock clear-credential '{ "credentialType": 1, "credentialIndex": 1}' 1111 1 --timedInteractionTimeoutMs 1000
          ## Command to Clear the User
          >>> doorlock clear-user 1 1111 1 --timedInteractionTimeoutMs 1000
