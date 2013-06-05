# module: math

MATH_DIR = $(LIB_DIR)/math
MATH_BIN  = $(MATH_DIR)/math.a
MATH_OBJ  = $(MATH_DIR)/math.o $(MATH_DIR)/complex.o $(MATH_DIR)/vector.o \
   $(MATH_DIR)/matrix.o $(MATH_DIR)/matrix3.o $(MATH_DIR)/quaternion.o $(MATH_DIR)/plane.o \
   $(MATH_DIR)/rect.o $(MATH_DIR)/interpolation.o $(MATH_DIR)/random.o $(MATH_DIR)/ray.o
MATH_CXXFLAGS = $(GLOBALFLAGS)

$(MATH_BIN) : $(MATH_OBJ)
	rm -f $@
	ar cq $@ $(MATH_OBJ)

$(MATH_DIR)/math.o : $(MATH_DIR)/math.cpp
	$(CPP) -c $(MATH_DIR)/math.cpp -o $@ $(MATH_CXXFLAGS)

$(MATH_DIR)/complex.o : $(MATH_DIR)/complex.cpp
	$(CPP) -c $(MATH_DIR)/complex.cpp -o $@ $(MATH_CXXFLAGS)

$(MATH_DIR)/vector.o : $(MATH_DIR)/vector.cpp
	$(CPP) -c $(MATH_DIR)/vector.cpp -o $@ $(MATH_CXXFLAGS)

$(MATH_DIR)/matrix.o : $(MATH_DIR)/matrix.cpp
	$(CPP) -c $(MATH_DIR)/matrix.cpp -o $@ $(MATH_CXXFLAGS)

$(MATH_DIR)/matrix3.o : $(MATH_DIR)/matrix3.cpp
	$(CPP) -c $(MATH_DIR)/matrix3.cpp -o $@ $(MATH_CXXFLAGS)

$(MATH_DIR)/quaternion.o : $(MATH_DIR)/quaternion.cpp
	$(CPP) -c $(MATH_DIR)/quaternion.cpp -o $@ $(MATH_CXXFLAGS)

$(MATH_DIR)/plane.o : $(MATH_DIR)/plane.cpp
	$(CPP) -c $(MATH_DIR)/plane.cpp -o $@ $(MATH_CXXFLAGS)

$(MATH_DIR)/rect.o : $(MATH_DIR)/rect.cpp
	$(CPP) -c $(MATH_DIR)/rect.cpp -o $@ $(MATH_CXXFLAGS)

$(MATH_DIR)/interpolation.o : $(MATH_DIR)/interpolation.cpp
	$(CPP) -c $(MATH_DIR)/interpolation.cpp -o $@ $(MATH_CXXFLAGS)

$(MATH_DIR)/random.o : $(MATH_DIR)/random.cpp
	$(CPP) -c $(MATH_DIR)/random.cpp -o $@ $(MATH_CXXFLAGS)

$(MATH_DIR)/ray.o : $(MATH_DIR)/ray.cpp
	$(CPP) -c $(MATH_DIR)/ray.cpp -o $@ $(MATH_CXXFLAGS)
