# Project: Kaba

BIN  = ../kaba
MAINSOURCE = main.cpp
all : $(BIN)
MODULES = \
 lib/script/script.a \
 lib/x/x.a \
 lib/file/file.a \
 lib/types/types.a \
 lib/nix/nix.a \
 lib/image/image.a \
 lib/net/net.a \
 lib/hui/hui.a \
 lib/any/any.a \
 lib/algebra/algebra.a \
 lib/threads/threads.a \
 lib/base/base.a \
 lib/sound/sound.a
OBJ = lib/base/strings.o lib/file/msg.o lib/script/script.o
CPP  = g++
#GLOBALFLAGS = -O3
GLOBALFLAGS = -g -O3
#GLOBALFLAGS = -g
LIB_DIR = lib
CXXFLAGS = `pkg-config --cflags gtk+-3.0` $(GLOBALFLAGS)
CXXFLAGSLINKING =  `pkg-config --cflags --libs gtk+-3.0` -lGL -lGLU -I/usr/include/AL -lopenal -lvorbis -lvorbisfile $(GLOBALFLAGS)


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
include lib/x/x.mk
include lib/script/script.mk
include lib/net/net.mk
include lib/types/types.mk
include lib/any/any.mk
include lib/algebra/algebra.mk
include lib/threads/threads.mk

clean :
	#rm -f $(OBJ) $(BIN)
	#rm -f temp/* $(BIN)
	rm -f $(BIN)
	find . -name "*.o" -exec rm -f {} \;
	find . -name "*.a" -exec rm -f {} \;
