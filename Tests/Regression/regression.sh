for f in *.reg
do
	k=${f%reg}kaba
	echo $k
	if grep -q vli "$k"
	then
		echo "---vli"
	else
		../../Release/Kaba.exe "$k" > out
		diff "$f" out
	fi

done
