# Project: Kaba

BIN  = ../kaba
MAINSOURCE = main.cpp
all : $(BIN)

install : all
	mkdir -p /usr/share/kaba
	mkdir -p /usr/share/kaba/Data
	cp -f ../Data/icon.ico /usr/share/kaba/Data
	xdg-mime install ../Data/michisoft-kaba.xml
	xdg-desktop-menu install ../Data/michisoft-kaba.desktop
	xdg-icon-resource install --context mimetypes --size 64 ../Data/kaba.png text-michisoft-kaba
	cp $(BIN) /usr/bin


