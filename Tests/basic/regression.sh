kaba="kaba"

if [ -n "$1" ]
then
	kaba="$1"
fi




for f in *.reg
do
	k=${f%reg}kaba
	printf "%-32s" "$k"
	"$kaba" "$k" > out
	x=`diff "$f" out`
	if [ -n "$x" ]
	then
		echo "Fehler"
		echo "$x"
	else
		echo "ok"
	fi
done

rm -f out
