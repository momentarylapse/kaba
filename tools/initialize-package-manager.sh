BASE=~/.kaba
PACKAGES=$BASE/packages
REPOS=$BASE/repos
URL="https://github.com/momentarylapse/common-kaba-packages.git"
SOURCE_NAME="michi.common"

if [ ! -d $BASE ]
then
	mkdir $BASE
fi

echo "creating $BASE/sources"
if [ -e $BASE/sources ]
then
	echo "WARNING  already exists"
else
	echo "repo $SOURCE_NAME $URL" > $BASE/sources
fi

if [ ! -d $PACKAGES ]
then
	mkdir $PACKAGES
fi
if [ ! -d $REPOS ]
then
	mkdir $REPOS
fi

echo "cloning $SOURCE_NAME repo into $REPOS/$SOURCE_NAME"
if [ -d $REPOS/$SOURCE_NAME ]
then
	echo "ERROR  already exists"
	exit 1
else
	mkdir $REPOS/$SOURCE_NAME
	git clone $URL $REPOS/$SOURCE_NAME
fi


echo "installing package manager package"
DIR=$PACKAGES/package
if [ -d $DIR ]
then
	echo "ERROR already exists"
	exit 1
else
	cp -r $REPOS/$SOURCE_NAME/package $DIR
fi

