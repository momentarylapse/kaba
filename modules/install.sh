DEST_BASE_DIR=~/.kaba/modules


if [ ! -d "$DEST_BASE_DIR" ]
then
	mkdir "$DEST_BASE_DIR"
fi





for M in *
do
	if [ ! -d $M ]
	then
		continue
	fi

	echo $M

	if [ ! -d $M/build ]
	then
		echo "-- NO BUILD FOLDER! --"
		continue
	fi

	DEST="$DEST_BASE_DIR/$M"


	if [ ! -d "$DEST" ]
	then
		mkdir "$DEST"
	fi

	cp -v $M/*.kaba "$DEST"
	cp -v $M/build/libmodule* "$DEST"


done

