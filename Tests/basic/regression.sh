KABA="kaba"
OPT=""

if [ -n "$1" ]
then
	KABA="$1"
fi

if [ -n "$2" ]
then
	OPT="$2"
fi



for f in *.reg
do
	k=${f%reg}kaba
	printf "%-32s" "$k"
	"$KABA" $OPT "$k" > out
	x=`diff --strip-trailing-cr "$f" out`
	if [ -n "$x" ]
	then
		echo "Fehler"
		echo "$x"
	else
		echo "ok"
	fi
done

rm -f out
