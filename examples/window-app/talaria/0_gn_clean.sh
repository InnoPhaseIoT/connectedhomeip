red='\033[0;31m'
green='\033[0;32m'
yellow='\033[0;33m'
blue='\033[0;34m'
magenta='\033[0;35m'
cyan='\033[0;36m'
clear='\033[0m'

echo -e "${yellow}"
pwd
echo -e "${yellow}    0)*** cleaning out/test/${clear}"

rm -rf out/test

gn gen out/test --args="is_debug=false"

# echo "    0)*** END OF cleaning out/test/"
