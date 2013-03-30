# module: types

TYPES_DIR = $(LIB_DIR)/types
TYPES_BIN  = $(TYPES_DIR)/types.a
TYPES_OBJ  = $(TYPES_DIR)/types.o $(TYPES_DIR)/types_color.o $(TYPES_DIR)/types_color.o $(TYPES_DIR)/types_complex.o $(TYPES_DIR)/types_vector.o \
   $(TYPES_DIR)/types_matrix.o $(TYPES_DIR)/types_matrix3.o $(TYPES_DIR)/types_quaternion.o $(TYPES_DIR)/types_plane.o \
   $(TYPES_DIR)/types_rect.o $(TYPES_DIR)/types_interpolation.o
TYPES_CXXFLAGS = $(GLOBALFLAGS)

$(TYPES_BIN) : $(TYPES_OBJ)
	rm -f $@
	ar cq $@ $(TYPES_OBJ)

$(TYPES_DIR)/types.o : $(TYPES_DIR)/types.cpp
	$(CPP) -c $(TYPES_DIR)/types.cpp -o $@ $(TYPES_CXXFLAGS)

$(TYPES_DIR)/types_color.o : $(TYPES_DIR)/color.cpp
	$(CPP) -c $(TYPES_DIR)/color.cpp -o $@ $(TYPES_CXXFLAGS)

$(TYPES_DIR)/types_complex.o : $(TYPES_DIR)/complex.cpp
	$(CPP) -c $(TYPES_DIR)/complex.cpp -o $@ $(TYPES_CXXFLAGS)

$(TYPES_DIR)/types_vector.o : $(TYPES_DIR)/vector.cpp
	$(CPP) -c $(TYPES_DIR)/vector.cpp -o $@ $(TYPES_CXXFLAGS)

$(TYPES_DIR)/types_matrix.o : $(TYPES_DIR)/matrix.cpp
	$(CPP) -c $(TYPES_DIR)/matrix.cpp -o $@ $(TYPES_CXXFLAGS)

$(TYPES_DIR)/types_matrix3.o : $(TYPES_DIR)/matrix3.cpp
	$(CPP) -c $(TYPES_DIR)/matrix3.cpp -o $@ $(TYPES_CXXFLAGS)

$(TYPES_DIR)/types_quaternion.o : $(TYPES_DIR)/quaternion.cpp
	$(CPP) -c $(TYPES_DIR)/quaternion.cpp -o $@ $(TYPES_CXXFLAGS)

$(TYPES_DIR)/types_plane.o : $(TYPES_DIR)/plane.cpp
	$(CPP) -c $(TYPES_DIR)/plane.cpp -o $@ $(TYPES_CXXFLAGS)

$(TYPES_DIR)/types_rect.o : $(TYPES_DIR)/rect.cpp
	$(CPP) -c $(TYPES_DIR)/rect.cpp -o $@ $(TYPES_CXXFLAGS)

$(TYPES_DIR)/types_interpolation.o : $(TYPES_DIR)/interpolation.cpp
	$(CPP) -c $(TYPES_DIR)/interpolation.cpp -o $@ $(TYPES_CXXFLAGS)
