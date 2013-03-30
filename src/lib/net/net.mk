# module: net

NET_DIR = $(LIB_DIR)/net
NET_BIN  = $(NET_DIR)/net.a
NET_OBJ  = $(NET_DIR)/net.o
NET_CXXFLAGS =  $(GLOBALFLAGS)


$(NET_BIN) : $(NET_OBJ)
	rm -f $@
	ar cq $@ $(NET_OBJ)

$(NET_DIR)/net.o : $(NET_DIR)/net.cpp
	$(CPP) -c $(NET_DIR)/net.cpp -o $@ $(NET_CXXFLAGS)
