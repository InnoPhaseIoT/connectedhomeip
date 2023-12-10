red='\033[0;31m'
green='\033[0;32m'
yellow='\033[0;33m'
blue='\033[0;34m'
magenta='\033[0;35m'
cyan='\033[0;36m'
clear='\033[0m'

echo -e "${yellow}"
pwd
echo -e "${yellow}    4)*** linking and generating out/chip-example-app${clear}"
#set -x
arm-none-eabi-ld -Map out/talaria.map \
    -L../../../third_party/talaria/repo/freertos_sdk/libs \
    -L../../../third_party/talaria/repo/freertos_sdk/libs/arm \
    -L../../../third_party/talaria/repo/freertos_sdk/build \
    -Lout/test/lib \
    -T../../../third_party/talaria/repo/freertos_sdk/build/firmware-arm-virt.lds --entry _start \
    -zmax-page-size=256 \
    --start-group \
    -lwifi -lrfdrv -llwip2 -lsupplicant -lfreertos -lc -linnos -lfs -losal -lcomponents \
    -lmbedtls -ldragonfly \
    ./out/test/LightApplication.a \
    -lCHIP -lbt_host -lbt_controller -lbt_profiles \
    ../../../.environment/cipd/packages/arm/arm-none-eabi/lib/thumb/v7-m/nofp/libstdc++_nano.a \
    ../../../.environment/cipd/packages/arm/lib/gcc/arm-none-eabi/12.2.1/thumb/v7-m/nofp/libgcc.a \
    --end-group \
    -o out/chip-example-app.elf

# -lbt_profiles -ldragonfly -limath -lmbedtls -lbt_controller -lbt_host \

# echo "    4)*** END OF linking and generating out/chip-example-app"
