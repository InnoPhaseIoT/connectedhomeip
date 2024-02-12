red='\033[0;31m'
green='\033[0;32m'
yellow='\033[0;33m'
blue='\033[0;34m'
magenta='\033[0;35m'
cyan='\033[0;36m'
clear='\033[0m'

echo -e "${yellow}"
pwd
echo -e "${yellow}    1)*** compile and link out/chip-example-app...${clear}"

# ./3_gn_compile.sh && ./4_gn_link.sh
./3_gn_compile.sh

# echo "    1)*** END OF build and load out/chip-example-app..."
