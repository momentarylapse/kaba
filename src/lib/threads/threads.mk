# module: threads

THREAD_DIR = $(LIB_DIR)/threads
THREAD_BIN  = $(THREAD_DIR)/threads.a
THREAD_OBJ  = $(THREAD_DIR)/Thread.o $(THREAD_DIR)/Mutex.o $(THREAD_DIR)/ThreadedWork.o
THREAD_CXXFLAGS = `pkg-config --cflags gtk+-3.0` $(GLOBALFLAGS)
THREAD_DEP =  $(THREAD_DIR)/Thread.h

$(THREAD_BIN) : $(THREAD_OBJ) $(THREAD_DEP)
	rm -f $@
	ar cq $@ $(THREAD_OBJ)

$(THREAD_DIR)/Thread.o : $(THREAD_DIR)/Thread.cpp $(THREAD_DEP)
	$(CPP) -c $(THREAD_DIR)/Thread.cpp -o $@ $(THREAD_CXXFLAGS)

$(THREAD_DIR)/ThreadedWork.o : $(THREAD_DIR)/ThreadedWork.cpp $(THREAD_DEP)
	$(CPP) -c $(THREAD_DIR)/ThreadedWork.cpp -o $@ $(THREAD_CXXFLAGS)

$(THREAD_DIR)/Mutex.o : $(THREAD_DIR)/Mutex.cpp $(THREAD_DEP)
	$(CPP) -c $(THREAD_DIR)/Mutex.cpp -o $@ $(THREAD_CXXFLAGS)


