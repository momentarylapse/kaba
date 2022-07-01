/*----------------------------------------------------------------------------*\
| CFile                                                                        |
| -> acces to files (high & low level)                                         |
| -> text mode / binary mode                                                   |
|    -> textmode: numbers as decimal numbers, 1 line per value saved,          |
|                 carriage-return/linefeed 2 characters (windows),...          |
|    -> binary mode: numbers as 4 byte binary coded, carriage-return 1         |
|                    character,...                                             |
| -> opening a missing file can call a callback function (x: used for          |
|    automatically downloading the file)                                       |
| -> files can be stored in an archive file                                    |
|                                                                              |
| vital properties:                                                            |
|  - a single instance per file                                                |
|                                                                              |
| last update: 2010.07.01 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(FILE_H)
#define FILE_H



// ANSI:
//#include <stdarg.h>
// UNIX:
//#include <varargs.h>


#include "../base/base.h"


#include <string.h>
#include <stdlib.h>
	
#include "formatter.h"
#include "msg.h"
#include "file_op.h"
#include "path.h"



//--------------------------------------------------------------
// time/date

class Date {
public:
	int64 time;
	int milli_second;
	int dummy[7];
	string _cdecl format(const string &f) const;
	string _cdecl str() const;
	void __assign__(const Date &d);

	static Date _cdecl now();
};



class FileError : public Exception {
public:
	explicit FileError(const string &msg) : Exception(msg) {}
};


//--------------------------------------------------------------
// file operation class

typedef bool t_file_try_again_func(const string &filename);

extern t_file_try_again_func *FileTryAgainFunc;

class Stream : public Sharable<Empty> {
public:
	virtual ~Stream() {}

	// meta
	virtual void set_pos(int pos) = 0;
	virtual void seek(int delta) = 0;
	virtual int get_size32() = 0;
	virtual int64 get_size() = 0;
	virtual int get_pos() = 0;

	virtual int read(void *buffer, int size) = 0;
	virtual int write(const void *buffer, int size) = 0;
	int read(bytes&);
	int write(const bytes&);

	virtual bool is_end() = 0;

	bytes read_complete();
};

class FileStream : public Stream {
public:
	FileStream(int handle);
	~FileStream();

	// meta
	void set_pos(int pos) override;
	void seek(int delta) override;
	int get_size32() override;
	int64 get_size() override;
	int get_pos() override;
	Date ctime();
	Date mtime();
	Date atime();

	bool is_end() override;
	void close();

	int read(void *buffer, int size) override;
	int write(const void *buffer, int size) override;
	int read(bytes&); // why???
	int write(const bytes&);

//private:
	Path filename;
	int handle = -1;
};


extern FileStream *file_open(const Path &filename, const string &mode);

extern bytes file_read_binary(const Path &filename);
extern string file_read_text(const Path &filename);
extern void file_write_binary(const Path &filename, const bytes &data);
extern void file_write_text(const Path &filename, const string &str);




#endif


