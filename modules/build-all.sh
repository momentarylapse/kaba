

for M in *
do
	if [ ! -f "$M/xmake.conf" ]
	then
		continue
	fi

	echo $M

	if [ ! -d $M/build ]
	then
		mkdir $M/build
	fi
	
	cd $M/build
	cmake .. -GNinja && ninja
	cd ../..
done

