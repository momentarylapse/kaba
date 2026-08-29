URL_LIB="https://github.com/momentarylapse/common-kaba-packages.git"
URL_KABA="https://github.com/momentarylapse/kaba.git"
RED='\033[0;31m'
ORANGE='\033[0;33m'
GREEN='\033[0;32m'
NC='\033[0m'
BASE=~/.kaba
LOCAL_BIN=~/.local/bin

#-------------------------------------------------
# checks


if [ -d "$BASE" ]
then
	echo -e "${RED}ERROR${NC}: '$BASE' already exists"
	exit 1
fi


MISSING_TOOLS=0
function check_tool() {
	X=`which $1`
	if [ $? -ne 0 ]
	then
		echo -e "${RED}ERROR${NC}: '$1' not found, please install"
		MISSING_TOOLS=1
	fi
}

check_tool git
check_tool ninja
check_tool cmake

if [ $MISSING_TOOLS -ne 0 ]
then
	exit 1
fi


#-------------------------------------------------
# start

echo -e "${ORANGE}creating kaba environment in '$BASE'${NC}"


mkdir "$BASE"
mkdir "$BASE/repos"
mkdir "$BASE/packages"
mkdir -p "$LOCAL_BIN"


#-------------------------------------------------
# git clone

echo -e "${ORANGE}cloning git repo for kaba compiler${NC}"
git clone "$URL_KABA" "$BASE/repos/kaba"

echo -e "${ORANGE}cloning git repo for common package${NC}"
git clone "$URL_LIB" "$BASE/repos/michi.common"


#-------------------------------------------------
# build kaba

echo -e "${ORANGE}building kaba compiler${NC}"
TEMP="$BASE/temp"
mkdir "$TEMP"
cd "$TEMP"
cmake "$BASE/repos/kaba" -GNinja
ninja
cp kaba "$LOCAL_BIN"
cd -
rm -rf "$TEMP"


#-------------------------------------------------
# install package manager

echo -e "${ORANGE}installing package manager${NC}"
cp -r "$BASE/repos/michi.common/package" "$BASE/packages"

echo "repo kaba $URL_KABA" >> "$BASE/sources"
echo "repo michi.common $URL_LIB" >> "$BASE/sources"

echo $PATH | grep -q ~/.local/bin
if [ $? != 0 ]
then
	echo -e "${RED}WARNING ${LOCAL_BIN} is not in PATH"
	echo -e "please append the following line to ~/.bashrc"
	echo -e "  PATH=${LOCAL_BIN}:\$PATH${NC}"
fi


echo -e "${GREEN}success${NC}"
echo ""
echo "useful commands:"
echo "   kaba package list              # list all available packages"
echo "   kaba package install <PACKAGE> # install a package"
echo "   kaba package upgrade           # pull repos and install updates"
