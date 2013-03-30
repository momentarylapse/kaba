# module: file

FILE_DIR = $(LIB_DIR)/file
FILE_MODULE  = $(FILE_DIR)/file.a
FILE_OBJ  = $(FILE_DIR)/file.o $(FILE_DIR)/msg.o $(FILE_DIR)/file_op.o
FILE_CXXFLAGS = $(GLOBALFLAGS)


$(FILE_MODULE) : $(FILE_OBJ)
	rm -f $@
	ar cq $@ $(FILE_OBJ)

$(FILE_DIR)/file.o : $(FILE_DIR)/file.cpp
	$(CPP) -c $(FILE_DIR)/file.cpp -o $@ $(FILE_CXXFLAGS)

$(FILE_DIR)/msg.o : $(FILE_DIR)/msg.cpp
	$(CPP) -c $(FILE_DIR)/msg.cpp -o $@ $(FILE_CXXFLAGS)

$(FILE_DIR)/file_op.o : $(FILE_DIR)/file_op.cpp
	$(CPP) -c $(FILE_DIR)/file_op.cpp -o $@ $(FILE_CXXFLAGS)

#file/file.cpp : file/file.h file/msg.h
#file/msg.cpp : file/msg.h
#file/msg.h : file/file.h

