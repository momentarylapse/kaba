# module: image

IMAGE_DIR = $(LIB_DIR)/image
IMAGE_BIN  = $(IMAGE_DIR)/image.a
IMAGE_OBJ  = $(IMAGE_DIR)/image.o $(IMAGE_DIR)/image_bmp.o $(IMAGE_DIR)/image_tga.o $(IMAGE_DIR)/image_jpg.o $(IMAGE_DIR)/color.o
IMAGE_CXXFLAGS = $(GLOBALFLAGS)

$(IMAGE_BIN) : $(IMAGE_OBJ)
	rm -f $@
	ar cq $@ $(IMAGE_OBJ)

$(IMAGE_DIR)/image.o : $(IMAGE_DIR)/image.cpp
	$(CPP) -c $(IMAGE_DIR)/image.cpp -o $@ $(IMAGE_CXXFLAGS)

$(IMAGE_DIR)/image_bmp.o : $(IMAGE_DIR)/image_bmp.cpp
	$(CPP) -c $(IMAGE_DIR)/image_bmp.cpp -o $@ $(IMAGE_CXXFLAGS)

$(IMAGE_DIR)/image_tga.o : $(IMAGE_DIR)/image_tga.cpp
	$(CPP) -c $(IMAGE_DIR)/image_tga.cpp -o $@ $(IMAGE_CXXFLAGS)

$(IMAGE_DIR)/image_jpg.o : $(IMAGE_DIR)/image_jpg.cpp
	$(CPP) -c $(IMAGE_DIR)/image_jpg.cpp -o $@ $(IMAGE_CXXFLAGS)

$(IMAGE_DIR)/color.o : $(IMAGE_DIR)/color.cpp
	$(CPP) -c $(IMAGE_DIR)/color.cpp -o $@ $(IMAGE_CXXFLAGS)
