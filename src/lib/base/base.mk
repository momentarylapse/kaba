# module: base

BASE_DIR = $(LIB_DIR)/base
BASE_MODULE  = $(BASE_DIR)/base.a
BASE_OBJ  = $(BASE_DIR)/array.o $(BASE_DIR)/strings.o
BASE_CXXFLAGS = $(GLOBALFLAGS)


$(BASE_MODULE) : $(BASE_OBJ)
	rm -f $@
	ar cq $@ $(BASE_OBJ)

$(BASE_DIR)/array.o : $(BASE_DIR)/array.cpp
	$(CPP) -c $(BASE_DIR)/array.cpp -o $@ $(BASE_CXXFLAGS)

$(BASE_DIR)/strings.o : $(BASE_DIR)/strings.cpp
	$(CPP) -c $(BASE_DIR)/strings.cpp -o $@ $(BASE_CXXFLAGS)

