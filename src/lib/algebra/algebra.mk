# module: algebra

ALGEBRA_DIR = $(LIB_DIR)/algebra
ALGEBRA_BIN  = $(ALGEBRA_DIR)/algebra.a
ALGEBRA_OBJ  = $(ALGEBRA_DIR)/vli.o $(ALGEBRA_DIR)/crypto.o
ALGEBRA_CXXFLAGS = $(GLOBALFLAGS)

$(ALGEBRA_BIN) : $(ALGEBRA_OBJ)
	rm -f $@
	ar cq $@ $(ALGEBRA_OBJ)

$(ALGEBRA_DIR)/vli.o : $(ALGEBRA_DIR)/vli.cpp
	$(CPP) -c $(ALGEBRA_DIR)/vli.cpp -o $@ #$(ALGEBRA_CXXFLAGS)

$(ALGEBRA_DIR)/crypto.o : $(ALGEBRA_DIR)/crypto.cpp
	$(CPP) -c $(ALGEBRA_DIR)/crypto.cpp -o $@ #$(ALGEBRA_CXXFLAGS)
