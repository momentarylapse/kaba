# Project: Kaba

BIN  = ../kaba
MAINSOURCE = main.cpp
all : $(BIN)
MODULES = \
 lib/kaba/kaba.a \
 lib/file/file.a \
 lib/math/math.a \
 lib/nix/nix.a \
 lib/image/image.a \
 lib/net/net.a \
 lib/hui/hui.a \
 lib/any/any.a \
 lib/algebra/algebra.a \
 lib/threads/threads.a \
 lib/base/base.a \
 lib/sound/sound.a
OBJ = lib/base/strings.o lib/file/msg.o lib/kaba/kaba.o
CPP  = g++ -std=c++11
#GLOBALFLAGS = -O3
#GLOBALFLAGS = -g -O3
GLOBALFLAGS = -g
LIB_DIR = lib
CXXFLAGS = `pkg-config --cflags gtk+-3.0` $(GLOBALFLAGS)
CXXFLAGSLINKING =  `pkg-config --cflags --libs gtk+-3.0` -lGL -lGLU -lX11 -I/usr/include/AL -lopenal -lvorbis -lvorbisfile -lz $(GLOBALFLAGS)


$(BIN) : $(MAINSOURCE) $(OBJ) $(MODULES)
	$(CPP) $(OBJ) $(MODULES) $(MAINSOURCE) -o $(BIN) $(CXXFLAGSLINKING)

install : $(BIN)
	mkdir -p /usr/share/kaba
	mkdir -p /usr/share/kaba/Data
	cp -f ../Data/icon.ico /usr/share/kaba/Data
	xdg-mime install ../Data/michisoft-kaba.xml
	xdg-desktop-menu install ../Data/michisoft-kaba.desktop
	xdg-icon-resource install --context mimetypes --size 64 ../Data/kaba.png text-michisoft-kaba
	cp $(BIN) /usr/bin


include lib/base/base.mk
include lib/file/file.mk
include lib/hui/hui.mk
include lib/nix/nix.mk
include lib/image/image.mk
include lib/sound/sound.mk
include lib/kaba/kaba.mk
include lib/net/net.mk
include lib/math/math.mk
include lib/any/any.mk
include lib/algebra/algebra.mk
include lib/threads/threads.mk

test : all
	cd ../Tests/Regression; kaba regression.kaba

clean :
	#rm -f $(OBJ) $(BIN)
	#rm -f temp/* $(BIN)
	rm -f $(BIN)
	find . -name "*.o" -exec rm -f {} \;
	find . -name "*.a" -exec rm -f {} \;
