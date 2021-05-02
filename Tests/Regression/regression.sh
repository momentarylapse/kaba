kaba="kaba"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

if [ -n "$1" ]
then
	kaba="$1"
fi


ERRORS=0

for f in *.reg
do
	k=${f%reg}kaba
	printf "%-36s" "$k"
	"$kaba" "$k" > out
	x=`diff --strip-trailing-cr "$f" out`
	if [ -n "$x" ]
	then
		echo -e "${RED}ERROR${NC}"
		echo -e "${RED}$x${NC}"
		ERRORS=`expr $ERRORS + 1`
	else
		echo -e "${GREEN}ok${NC}"
	fi
done

rm -f out

if [ $ERRORS = 0 ]
then
	echo -e "${GREEN}all tests passed${NC}"
else
	echo -e "${RED}$ERRORS tests failed${NC}"
fi
