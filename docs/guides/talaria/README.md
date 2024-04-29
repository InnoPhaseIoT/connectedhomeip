# Matter support in InnoPhase IoT sdk

This document describes the steps to build the matter application for Talaria platform using InnoPhase IoT.

## Clonning Repository
- Clone connectedhomeip repository from https://github.com/InnoPhaseIoT/connectedhomeip wherever you want using following command
```sh
~/$ git clone https://github.com/InnoPhaseIoT/connectedhomeip.git
```

## Environment Setup
- Installing prerequisites on Linux as mentioned in https://github.com/project-chip/connectedhomeip/blob/master/docs/guides/BUILDING.md#installing-prerequisites-on-linux
```sh
sudo apt-get install git gcc g++ pkg-config libssl-dev libdbus-1-dev \
     libglib2.0-dev libavahi-client-dev ninja-build python3-venv python3-dev \
     python3-pip unzip libgirepository1.0-dev libcairo2-dev libreadline-dev
```

- Switch to InnoPhase development branch (talaria_two_matter_v1.2) using following command
```sh
$ cd connectedhomeip ; git checkout talaria_two_matter_v1.2
```
- Setup the freertos_sdk link using following command
```sh
connectedhomeip$ cd third_party/talaria/repo/ ; ln -sf <aboslute-path-to-FreeRTOS_sdk_x.x> freertos_sdk ; cd -
```
- regenerate the components library using following command
```sh
FreeRTOS_sdk_x.x$ make -C components clean all
```
## Building the Example application and chip-tool
- Build the environment if not already built using following command from connectedhomeip root directory.
```sh
connectedhomeip$ source ./scripts/bootstrap.sh
```
- Activate the environment if not activated already using following command from connectedhomeip root directory. This step is required from each new bash session before starting to build.
```sh
connectedhomeip$ source ./scripts/activate.sh
```
- Build chip-tool: Run the following command from connecedhomeip root directory as mentioned in the examples/chip-tool/README.md.
```sh
FreeRTOS_sdk_3.0/matter/connectedhomeip$ scripts/examples/gn_build_example.sh examples/chip-tool out/debug
```
- Build lighting application: Run Following commands
```sh
connectedhomeip$ cd examples/lighting-app/talaria/
connectedhomeip/examples/lighting-app/talaria$ make clean ; make EXT_FLASH=true
```
- Build light-switch application: Run Following commands
```sh
connectedhomeip$ cd examples/light-switch-app/talaria/
connectedhomeip/examples/light-switch-app/talaria$ make clean ; make
```
- Build lock application: Run Following commands
```sh
connectedhomeip$ cd examples/lock-app/talaria/
connectedhomeip/examples/lock-app/talaria$ make clean ; make CHIP_ENABLE_OTA_STORAGE_ON_HOST=true
```
## Generating Factory Configuraion Data
The following steps provide the information on how the factory data can be generated which is required to be written to DATA partition after programming of the image on Talaria TWO EVB is done
```sh
FreeRTOS_sdk_x.x$ scripts/gen_factory_data.py --discriminator 1122 --output ./

### The console output of the command
Folder './data' created successfully.
Folder './data/chip-factory' created successfully.
Folder './data/chip-config' created successfully.
Folder './data/chip-counters' created successfully.
Integer value 1122 successfully stored in ./data/chip-factory/discriminator
Integer value 0 successfully stored in ./data/chip-factory/loc-capability
Integer value 0 successfully stored in ./data/chip-config/reg-location
```
## Generating Factory Configuraion Data using the generated certificates
Generating the Factory Configuration Data may involve multiple steps based on the requirement such as following:
1. Configure the generated certificates instead of connectedhomeip example certificates
2. Configure the setup passcode value
#### Generating the Certificates
Use following steps to generate PAA, PAI, DAC and Certificate Declaration in the required format
```sh
## Build chip-cert utility if already not using following command
connectedhomeip$ source scripts/activate.sh
connectedhomeip$ gn gen out/host
connectedhomeip$ ninja -C out/host chip-cert

## Generate PAA Certificate. [Here 'generate_certs' is used as output directory. Create the same if not created]. Vendor-Id can be the Test vendor id or the given by the CSA.
connectedhomeip$ out/host/chip-cert gen-att-cert --type a --subject-cn "Matter Development PAA 01" --subject-vid 0xFFF1 --valid-from "2024-03-05 00:00:00" --lifetime 4294967295 --out-key ./generate_certs/Chip-PAA-Key.pem --out ./generate_certs/Chip-PAA-Cert.pem

## Generate PAI Certificate
connectedhomeip$ out/host/chip-cert gen-att-cert --type i --subject-cn "Matter Development PAI 01" --subject-vid 0xFFF1 --valid-from "2024-03-05 00:00:00" --lifetime 4294967295 --ca-key ./generate_certs/Chip-PAA-Key.pem --ca-cert ./generate_certs/Chip-PAA-Cert.pem --out-key ./generate_certs/Chip-PAI-Key.pem --out ./generate_certs/Chip-PAI-Cert.pem

## Generate DAC Certificate. Here the vid should be same as PAA and PAI certificates. pid can be chosen based on the requirement
connectedhomeip$ out/host/chip-cert gen-att-cert --type d --subject-cn "Matter Development DAC 01" --subject-vid 0xFFF1 --subject-pid 0x8001 --valid-from "2024-03-05 00:00:00" --lifetime 4294967295 --ca-key ./generate_certs/Chip-PAI-Key.pem --ca-cert ./generate_certs/Chip-PAI-Cert.pem --out-key ./generate_certs/Chip-DAC-Key.pem --out ./generate_certs/Chip-DAC-Cert.pem

## Validating the generated certificates. If no error found then certificates are valid.
connectedhomeip$ out/host/chip-cert validate-att-cert --dac ./generate_certs/Chip-DAC-Cert.pem --pai ./generate_certs/Chip-PAI-Cert.pem --paa ./generate_certs/Chip-PAA-Cert.pem

## Generate Certificate Declaration. Here the vid and pid value should be same as the DAC certificate
connectedhomeip$ out/host/chip-cert gen-cd --format-version 1 --vendor-id 0xFFF1 --product-id 0x8001 --device-type-id 0x0100 --certificate-id CSA00000SWC00000-01 --security-level 0 --security-info 0 --version-number 1 --certification-type 1 --key credentials/test/certification-declaration/Chip-Test-CD-Signing-Key.pem --cert credentials/test/certification-declaration/Chip-Test-CD-Signing-Cert.pem --out ./generate_certs/Chip-CD.der

## Converting DAC and PAI certificates to .der format
connectedhomeip$ out/host/chip-cert convert-cert -d ./generate_certs/Chip-PAI-Cert.pem ./generate_certs/Chip-PAI-Cert.der
connectedhomeip$ out/host/chip-cert convert-cert -d ./generate_certs/Chip-DAC-Cert.pem ./generate_certs/Chip-DAC-Cert.der
```
#### Generating the passcode
Use following commands to generate the passcode, salt, iteration-count and verifier values
```sh
## Here -i is an interation-count value, -s is the salt value, -p is passcode value and generated ouput string is verifier value
connectedhomeip$ scripts/tools/spake2p/spake2p.py gen-verifier -i 1000 -s "U1BBS0UyUCBLZXkgU2FsdA==" -p 20202021
uWFwqugDNGiEck/po7KHwwMwwqZgN10XuyBajPGuyzUEV/iree4lOrao5GuwnlQ65CJzbeUB49s31EH+NEkg0JVI5MGCQGMMT/SRPFNRODm3wH/MBiehuFc6FJ/NH6Rmzw==
```
#### Creating the Factory data content
The following steps provide the information on how the factory data can be generated which is required to be written to DATA partition after programming of the image on Talaria TWO EVB is done
```sh
FreeRTOS_sdk_x.x$ ./gen_factory_data.py --output ./ --dac-cert <path-to-connectedhomeip>/generate_certs/Chip-DAC-Cert.der --dac-key <path-to-connectedhomeip>/generate_certs/Chip-DAC-Key.pem --pai-cert <path-to-connectedhomeip>/generate_certs/Chip-PAI-Cert.der --cert-dclrn <path-to-connectedhomeip>/generate_certs/Chip-CD.der --vendor-id 0xFFF1 --product-id 0x8001 --discriminator 3840 --pin-code 20202021 --iteration-count 1000 --salt "U1BBS0UyUCBLZXkgU2FsdA==" --verifier "uWFwqugDNGiEck/po7KHwwMwwqZgN10XuyBajPGuyzUEV/iree4lOrao5GuwnlQ65CJzbeUB49s31EH+NEkg0JVI5MGCQGMMT/SRPFNRODm3wH/MBiehuFc6FJ/NH6Rmzw=="

### The console output of the command
Folder './data' created successfully.
Folder './data/chip-factory' created successfully.
Folder './data/chip-config' created successfully.
Folder './data/chip-counters' created successfully.
Integer value 20202021 successfully stored in ./data/chip-factory/pin-code
Integer value 3840 successfully stored in ./data/chip-factory/discriminator
Integer value 1000 successfully stored in ./data/chip-factory/iteration-count
Integer value U1BBS0UyUCBLZXkgU2FsdA== successfully stored in ./data/chip-factory/salt
Integer value uWFwqugDNGiEck/po7KHwwMwwqZgN10XuyBajPGuyzUEV/iree4lOrao5GuwnlQ65CJzbeUB49s31EH+NEkg0JVI5MGCQGMMT/SRPFNRODm3wH/MBiehuFc6FJ/NH6Rmzw== successfully stored in ./data/chip-factory/verifier
Contents of ../connectedhomeip/gen_keys_to_load/Chip-DAC-Cert.der copied to ./data/chip-factory/dac-cert
Contents of ../connectedhomeip/gen_keys_to_load/Chip-PAI-Cert.der copied to ./data/chip-factory/pai-cert
Contents of ../connectedhomeip/gen_keys_to_load/Chip-CD.der copied to ./data/chip-factory/cert-dclrn
Integer value 5270 successfully stored in ./data/chip-factory/vendor-id
Integer value 32769 successfully stored in ./data/chip-factory/product-id
Integer value 0 successfully stored in ./data/chip-factory/loc-capability
Integer value 0 successfully stored in ./data/chip-config/reg-location
```
#### Updated chip-tool command to commission the DUT using the generated certificates
Use the following command to argument with the chip-tool command to make use of the generated certificates
```sh
## Command for interactive session
connectedhomeip$ out/debug/chip-tool interactive start --paa-trust-store-path ./generate_certs/
## Command to commission without using interactive session
connectedhomeip$ out/debug/chip-tool pairing ble-wifi 1111 <wifi-ssid> <wifi-passphrase> 20202021 3840 --paa-trust-store-path ./generate_certs/
```
