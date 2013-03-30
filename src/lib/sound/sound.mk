# module: sound

SOUND_DIR = $(LIB_DIR)/sound
SOUND_BIN  = $(SOUND_DIR)/sound.a
SOUND_OBJ  = $(SOUND_DIR)/sound.o $(SOUND_DIR)/sound_al.o
SOUND_CXXFLAGS = -I/usr/include/AL -lopenal -lvorbis -lvorbisfile $(GLOBALFLAGS)

$(SOUND_BIN) : $(SOUND_OBJ)
	rm -f $@
	ar cq $@ $(SOUND_OBJ)

$(SOUND_DIR)/sound.o : $(SOUND_DIR)/sound.cpp
	$(CPP) -c $(SOUND_DIR)/sound.cpp -o $@ $(SOUND_CXXFLAGS)

$(SOUND_DIR)/sound_al.o : $(SOUND_DIR)/sound_al.cpp
	$(CPP) -c $(SOUND_DIR)/sound_al.cpp -o $@ $(SOUND_CXXFLAGS)

$(SOUND_DIR)/sound.cpp : $(SOUND_DIR)/sound.h
$(SOUND_DIR)/sound.h : $(LIB_DIR)/config.h

