MODULES=~/.kaba/modules
DEST="$MODULES/glfw"

if [ ! -d "$MODULES" ]
then
	mkdir "$MODULES"
fi


if [ ! -d "$DEST" ]
then
	mkdir "$DEST"
fi

cp -v *.kaba "$DEST"
cp -v build/libmodule* "$DEST"

