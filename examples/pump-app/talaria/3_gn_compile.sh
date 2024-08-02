red='\033[0;31m'
green='\033[0;32m'
yellow='\033[0;33m'
blue='\033[0;34m'
magenta='\033[0;35m'
cyan='\033[0;36m'
clear='\033[0m'

echo -e "${yellow}"
pwd
echo -e "${yellow}    3)*** compiling to out/test/${clear}"

ninja -C out/test -j 4

# echo "    3)*** END OF compiling to out/test/"
