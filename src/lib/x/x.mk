# module: x

X_DIR = $(LIB_DIR)/x
X_BIN  = $(X_DIR)/x.a
X_OBJ  = $(X_DIR)/meta.o $(X_DIR)/fx.o $(X_DIR)/camera.o $(X_DIR)/model.o $(X_DIR)/object.o $(X_DIR)/terrain.o\
$(X_DIR)/god.o $(X_DIR)/links.o $(X_DIR)/matrixn.o $(X_DIR)/tree.o $(X_DIR)/collision.o $(X_DIR)/physics.o \
$(X_DIR)/gui.o $(X_DIR)/light.o
X_CXXFLAGS = `pkg-config --cflags gtk+-3.0` $(GLOBALFLAGS)

$(X_BIN) : $(X_OBJ)
	rm -f $@
	ar cq $@ $(X_OBJ)

$(X_DIR)/camera.o : $(X_DIR)/camera.cpp
	$(CPP) -c $(X_DIR)/camera.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/collision.o : $(X_DIR)/collision.cpp
	$(CPP) -c $(X_DIR)/collision.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/fx.o : $(X_DIR)/fx.cpp
	$(CPP) -c $(X_DIR)/fx.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/god.o : $(X_DIR)/god.cpp
	$(CPP) -c $(X_DIR)/god.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/links.o : $(X_DIR)/links.cpp
	$(CPP) -c $(X_DIR)/links.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/light.o : $(X_DIR)/light.cpp
	$(CPP) -c $(X_DIR)/light.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/matrixn.o : $(X_DIR)/matrixn.cpp
	$(CPP) -c $(X_DIR)/matrixn.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/meta.o : $(X_DIR)/meta.cpp
	$(CPP) -c $(X_DIR)/meta.cpp -o $@ $(X_CXXFLAGS) -O1

$(X_DIR)/model.o : $(X_DIR)/model.cpp
	$(CPP) -c $(X_DIR)/model.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/object.o : $(X_DIR)/object.cpp
	$(CPP) -c $(X_DIR)/object.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/physics.o : $(X_DIR)/physics.cpp
	$(CPP) -c $(X_DIR)/physics.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/terrain.o : $(X_DIR)/terrain.cpp
	$(CPP) -c $(X_DIR)/terrain.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/tree.o : $(X_DIR)/tree.cpp
	$(CPP) -c $(X_DIR)/tree.cpp -o $@ $(X_CXXFLAGS)

$(X_DIR)/gui.o : $(X_DIR)/gui.cpp
	$(CPP) -c $(X_DIR)/gui.cpp -o $@ $(X_CXXFLAGS)


$(X_DIR)/camera.cpp : $(X_DIR)/x.h
$(X_DIR)/collision.cpp : $(X_DIR)/x.h
$(X_DIR)/fx.cpp : $(X_DIR)/x.h
$(X_DIR)/god.cpp : $(X_DIR)/x.h
$(X_DIR)/links.cpp : $(X_DIR)/x.h
$(X_DIR)/light.cpp : $(X_DIR)/x.h
$(X_DIR)/matrixn.cpp : $(X_DIR)/x.h
$(X_DIR)/meta.cpp : $(X_DIR)/x.h
$(X_DIR)/model.cpp : $(X_DIR)/x.h
$(X_DIR)/object.cpp : $(X_DIR)/x.h
$(X_DIR)/physics.cpp :
$(X_DIR)/terrain.cpp : $(X_DIR)/x.h
$(X_DIR)/tree.cpp : $(X_DIR)/x.h
$(X_DIR)/x.h : $(LIB_DIR)/nix/nix.h $(LIB_DIR)/file/file.h $(X_DIR)/camera.h $(X_DIR)/collision.h $(X_DIR)/model.h $(X_DIR)/god.h $(X_DIR)/links.h $(X_DIR)/physics.h $(X_DIR)/terrain.h $(X_DIR)/object.h $(X_DIR)/tree.h $(X_DIR)/matrixn.h $(X_DIR)/fx.h
