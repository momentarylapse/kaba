KABA="kaba"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

VERBOSE=0
SUITES_FILTER=""

while [[ $# -gt 0 ]]; do
	case $1 in
		-v|--verbose)
			VERBOSE=1
			shift
			;;
		-s|--suites)
			SUITES_FILTER=$2
			shift
			shift
			;;
		-k|--kaba)
			KABA="$2"
			shift
			shift
			;;
		*)
			echo "unknown option: $1"
			shift
			;;
	esac
done

TOTAL_FAILED=0
TOTAL_TESTS=0

for D in *
do
	if [ ! -d $D ]
	then
		continue
	fi
	if [[ -n "$SUITES_FILTER" && $D != "$SUITES_FILTER" ]]
	then
		continue
	fi
	
#	echo $D
	printf "%-20s" "$D"

	FAILED=0
	TESTS=0
	PASSED=0

	for f in $D/*.reg
	do
		k=${f%reg}kaba
		"$KABA" "$k" > out
		x=`diff --strip-trailing-cr "$f" out`
		if [ -n "$x" ]
		then
			if [ $VERBOSE -gt 0 ]
			then
				echo $f
				echo -e "${RED}ERROR${NC}"
				echo -e "${RED}$x${NC}"
			fi
			FAILED=`expr $FAILED + 1`
		else
#			echo -e "${GREEN}ok${NC}"
			PASSED=`expr $PASSED + 1`
		fi
		
		TESTS=`expr $TESTS + 1`
	done
	
	if [ $FAILED = 0 ]
	then
		echo -e "    ${GREEN}passed: $PASSED / $TESTS${NC}"
	else
		echo -e "    ${RED}passed: $PASSED / $TESTS  ($FAILED failed)${NC}"
	fi
	TOTAL_TESTS=`expr $TOTAL_TESTS + $TESTS`
	TOTAL_FAILED=`expr $TOTAL_FAILED + $FAILED`
done

rm -f out

if [ $TOTAL_FAILED = 0 ]
then
	echo -e "${GREEN}all tests passed${NC}"
else
	echo -e "${RED}$TOTAL_FAILED tests failed${NC}"
fi
