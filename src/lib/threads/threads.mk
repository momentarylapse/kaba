# module: threads

THREAD_DIR = $(LIB_DIR)/threads
THREAD_BIN  = $(THREAD_DIR)/threads.a
THREAD_OBJ  = $(THREAD_DIR)/threads.o $(THREAD_DIR)/mutex.o $(THREAD_DIR)/work.o
THREAD_CXXFLAGS = `pkg-config --cflags gtk+-2.0` $(GLOBALFLAGS)
THREAD_DEP =  $(THREAD_DIR)/threads.h

$(THREAD_BIN) : $(THREAD_OBJ) $(THREAD_DEP)
	rm -f $@
	ar cq $@ $(THREAD_OBJ)

$(THREAD_DIR)/threads.o : $(THREAD_DIR)/threads.cpp $(THREAD_DEP)
	$(CPP) -c $(THREAD_DIR)/threads.cpp -o $@ $(THREAD_CXXFLAGS)

$(THREAD_DIR)/work.o : $(THREAD_DIR)/work.cpp $(THREAD_DEP)
	$(CPP) -c $(THREAD_DIR)/work.cpp -o $@ $(THREAD_CXXFLAGS)

$(THREAD_DIR)/mutex.o : $(THREAD_DIR)/mutex.cpp $(THREAD_DEP)
	$(CPP) -c $(THREAD_DIR)/mutex.cpp -o $@ $(THREAD_CXXFLAGS)


