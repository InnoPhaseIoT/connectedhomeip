red='\033[0;31m'
green='\033[0;32m'
yellow='\033[0;33m'
blue='\033[0;34m'
magenta='\033[0;35m'
cyan='\033[0;36m'
clear='\033[0m'

echo -e "${yellow}"
pwd
echo -e "${yellow}    2)*** loading out/chip-example-app...${clear}"

if [ $1 -eq 0 ]; then
    ../../../third_party/talaria/repo/freertos_sdk/script/internal/load.sh /dev/ttyUSB2 out/test/lighting-app.elf matter.factory_reset=1
    sleep 5
fi

../../../third_party/talaria/repo/freertos_sdk/script/internal/load.sh /dev/ttyUSB2 out/test/lighting-app.elf \
    matter.color_log=1 matter.discriminator=1122 matter.commissioning.flow_type=0 suspend=0 matter.factory_reset=0 \
    matter.sed.active_interval=1 matter.sed.idle_interval=1

# echo "    2)*** END OF loading out/chip-example-app..."
