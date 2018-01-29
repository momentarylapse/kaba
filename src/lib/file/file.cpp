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
| last update: 2010.06.01 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#include "file.h"



//#define StructuredShifts
//#define FILE_COMMENTS_DEBUG

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#ifdef OS_WINDOWS
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
	#include <stdarg.h>
	#include <windows.h>
	#include <winbase.h>
	#include <winnt.h>
#endif
#ifdef OS_LINUX
	#include <unistd.h>
	#include <dirent.h>
	#include <stdarg.h>
	#include <sys/timeb.h>
	#include <sys/stat.h>

	#define _open	::open
	#define _read	::read
	#define _write	::write
	#define _lseek	::lseek
	bool _eof(int handle)
	{
		int pos = lseek(handle,0,SEEK_CUR);
		struct stat stat;
		fstat(handle, &stat);
		return (pos >= stat.st_size);
	}
	#define _close	::close
	#define _rmdir	::rmdir
	#define _unlink	::unlink
#endif


#ifdef OS_WINDOWS
Date systime2date(_SYSTEMTIME t)
{
	Date d;
	d.time=-1;//(int)t.time;   :---(
	d.year=t.wYear;
	d.month=t.wMonth;
	d.day=t.wDay;
	d.hour=t.wHour;
	d.minute=t.wMinute;
	d.second=t.wSecond;
	d.milli_second=t.wMilliseconds;
	d.day_of_week=t.wDayOfWeek;
	d.day_of_year=-1;//t.wDayofyear;   :---(
	return d;
}
#endif
Date time2date(time_t t)
{
	Date d;
	d.time=(int)t;
	tm *tm=localtime(&t);
	d.year=tm->tm_year+1900;
	d.month=tm->tm_mon;
	d.day=tm->tm_mday;
	d.hour=tm->tm_hour;
	d.minute=tm->tm_min;
	d.second=tm->tm_sec;
	d.milli_second=0;
	d.day_of_week=tm->tm_wday;
	d.day_of_year=tm->tm_yday;
	return d;
}

Date get_current_date()
{
#ifdef OS_WINDOWS
	_SYSTEMTIME t;
	GetLocalTime(&t);
	return systime2date(t);
#endif
#ifdef OS_LINUX
	time_t t;
	t = time(NULL);
	Date d;
	d = time2date(t);
	timeb tb;
	ftime(&tb);
	d.milli_second = tb.millitm;
	return d;
#endif
}

string Date::format(const string &f) const
{
	char buffer [80];
	time_t rawtime = this->time;
	tm * timeinfo = localtime (&rawtime);
	strftime(buffer, sizeof(buffer), f.c_str(), timeinfo);
	return buffer;
}

string Date::str() const
{	return this->format("%c");	}


t_file_try_again_func *FileTryAgainFunc;



File::File()
{
	handle = -1;
	float_decimals = 6;
}

File::~File()
{
	if (handle>=0)
		close();
}

bool File::eof()
{
#ifdef OS_WINDOW
	return _eof(handle);
#endif
#ifdef OS_LINUX
	int pos = lseek(handle,0,SEEK_CUR);
	struct stat stat;
	fstat(handle, &stat);
	return (pos >= stat.st_size);
#endif
}

class TextFile : public File
{
public:
	TextFile();
	virtual ~TextFile();

	virtual char read_char();
	virtual unsigned char read_byte();
	virtual unsigned int read_word();
	virtual int read_int();
	virtual float read_float();
	virtual bool read_bool();
	virtual string read_str();

	virtual void write_char(char c);
	virtual void write_byte(unsigned char c);
	virtual void write_word(unsigned int);
	virtual void write_int(int);
	virtual void write_float(float f);
	virtual void write_bool(bool b);
	virtual void write_str(const string &str);

	virtual void read_comment();
	virtual void write_comment(const string &str);
};

TextFile::TextFile()
{
	float_decimals = 6;
}

TextFile::~TextFile()
{
}

File *FileOpen(const string &filename)
{
	File *f = new File();
	try{
		f->open(filename);
	}catch(FileError &e){
		delete f;
		throw e;
	}
	return f;
}

File *FileOpenText(const string &filename)
{
	File *f = new TextFile();
	try{
		f->open(filename);
	}catch(FileError &e){
		delete f;
		throw e;
	}
	return f;
}

File *FileCreate(const string &filename)
{
	File *f = new File();
	try{
		f->create(filename);
	}catch(FileError &e){
		delete f;
		throw e;
	}
	return f;
}

File *FileCreateText(const string &filename)
{
	File *f = new TextFile();
	try{
		f->create(filename);
	}catch(FileError &e){
		delete f;
		throw e;
	}
	return f;
}

File *FileAppend(const string &filename)
{
	File *f = new File();
	try{
		f->append(filename);
	}catch(FileError &e){
		delete f;
		throw e;
	}
	return f;
}

void FileClose(File *f)
{
	if (f){
		f->close();
		delete(f);
	}
}

string FileRead(const string &filename)
{
	File *f = FileOpen(filename);
	string r = f->read_complete();
	FileClose(f);
	return r;
}

string FileReadText(const string &filename)
{
	File *f = FileOpenText(filename);
	string r = f->read_complete();
	FileClose(f);
	return r;
}

void FileWrite(const string &filename, const string &str)
{
	File *f = FileCreate(filename);
	f->write_buffer(str.data, str.num);
	FileClose(f);
}

void FileWriteText(const string &filename, const string &str)
{
	File *f = FileCreateText(filename);
	f->write_buffer(str.data, str.num);
	FileClose(f);
}

void set_mode(File *f)
{
#ifdef OS_WINDOWS
	if (dynamic_cast<TextFile*>(f))
		_setmode(handle,_O_TEXT);
	else
		_setmode(handle,_O_BINARY);
#endif
}

// open a file
void File::open(const string &_filename)
{
	handle = _open(_filename.sys_filename().c_str(), O_RDONLY);
	if (handle <= 0)
		throw FileError("failed opening file '" + _filename + "'");

	/*if (FileTryAgainFunc){
		if (FileTryAgainFunc(filename)){
			handle = _open(filename.sys_filename().c_str(), O_RDONLY);
			return;
		}
	}*/

	filename = _filename;
	set_mode(this);
}

// create a new file or reset an existing one
void File::create(const string &_filename)
{
#ifdef OS_WINDOWS
	handle = _creat(_filename.sys_filename().c_str(), _S_IREAD | _S_IWRITE);
#endif
#ifdef OS_LINUX
	handle = ::creat(_filename.sys_filename().c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
	if (handle <= 0)
		throw FileError("failed creating file '" + _filename + "'");

	filename = _filename;
	set_mode(this);
}

// create a new file or append data to an existing one
void File::append(const string &_filename)
{
#ifdef OS_WINDOWS
	handle = _open(_filename.sys_filename().c_str(), O_WRONLY | O_APPEND | O_CREAT,_S_IREAD | _S_IWRITE);
#endif
#ifdef OS_LINUX
	handle = ::open(_filename.sys_filename().c_str(), O_WRONLY | O_APPEND | O_CREAT,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
	if (handle <= 0)
		throw FileError("failed appending file '" + _filename + "'");

	filename = _filename;
	set_mode(this);
}

// close the file
void File::close()
{
	//flush(handle);
	if (handle >= 0)
		_close(handle);
	handle = -1;
}

// jump to an position in the file or to a position relative to the current
void File::set_pos(int pos)
{
	_lseek(handle, pos, SEEK_SET);
}

void File::seek(int delta)
{
	_lseek(handle, delta, SEEK_CUR);
}

// retrieve the size of the opened(!) file
int File::get_size()
{
#ifdef OS_WINDOWS
	return (int)_filelength(handle);
#endif
#ifdef OS_LINUX
	struct stat _stat;
	fstat(handle, &_stat);
	return _stat.st_size;
#endif
}

// retrieve the size of the opened(!) file
long long File::get_size64()
{
#ifdef OS_WINDOWS
	return (long long)_filelength(handle);
#endif
#ifdef OS_LINUX
	struct stat _stat;
	fstat(handle, &_stat);
	return _stat.st_size;
#endif
}

Date File::GetDateCreation()
{
	struct stat _stat;
	fstat(handle, &_stat);
	return time2date(_stat.st_ctime);
}

Date File::GetDateModification()
{
	struct stat _stat;
	fstat(handle, &_stat);
	return time2date(_stat.st_mtime);
}

Date File::GetDateAccess()
{
	struct stat _stat;
	fstat(handle, &_stat);
	return time2date(_stat.st_atime);
}

// where is the current reading position in the file?
int File::get_pos()
{
	return _lseek(handle, 0, SEEK_CUR);
}

// read a single character followed by the file-format-version-number
int File::ReadFileFormatVersion()
{
	unsigned char a=0;
	if (_read(handle,&a,1)<=0){
		return -1;
	}
	if (a=='t'){
		//SetBinaryMode(false);
	}else if (a=='b'){
		//SetBinaryMode(true);
	}else{
		throw FileError("File Format Version must begin ether with 't' or 'b'!!!");
	}
	return read_word();
}

// write a single character followed by the file-format-version-number
void File::WriteFileFormatVersion(bool binary,int fvv)
{
	char a = binary ? 'b' : 't';
	int r = _write(handle, &a, 1);
	//SetBinaryMode(binary);
	write_word(fvv);
}

#define CHUNK_SIYE		2048

// read the complete file into the buffer
string File::read_complete()
{
	string buf;
	string chunk;
	chunk.resize(CHUNK_SIYE);
	int t_len = CHUNK_SIYE;
	while(t_len > 0){
		t_len = read_buffer(chunk.data, chunk.num);

		buf += string(chunk.data, t_len);
	}
	return buf;
}

// read a part of the file into the buffer
int File::read_buffer(void *buffer, int size)
{
	int r = _read(handle, buffer, size);
	if (r < 0)
		throw FileError("failed reading file '" + filename + "'");
	return r;
}

// insert the buffer into the file
int File::write_buffer(const void *buffer, int size)
{
	int r = _write(handle,buffer,size);
	if (r < 0)
		throw FileError("failed writing file '" + filename + "'");
	return r;
}


// read a single character (1 byte)
char File::read_char()
{
	char c;
	read_buffer(&c, 1);
	return c;
}
char TextFile::read_char()
{
	char c = 0x0d;
	while (c == 0x0d)
		read_buffer(&c, 1);
	return c;
}

// read a single character (1 byte)
unsigned char File::read_byte()
{
	return read_char();
}

unsigned char TextFile::read_byte()
{
	return read_str()._int();
}

// read the rest of the line (only text mode)
void File::read_comment()
{}
void TextFile::read_comment()
{
#ifdef FILE_COMMENTS_DEBUG
	msg_write("comment: " + ReadStr());
#else
	read_str();
#endif

}

// read a word (2 bytes in binary mode)
unsigned int File::read_word()
{
	unsigned int i;
	read_buffer(&i, 2);
	return i;
}
unsigned int TextFile::read_word()
{
	return read_str()._int();
}

// read a word (2 bytes in binary mode)
unsigned int File::read_word_reversed()
{
	unsigned int a = read_byte();
	unsigned int b = read_byte();
	return (a << 8) + b;
}

// read an integer (4 bytes in binary mode)
int File::read_int()
{
	int i;
	read_buffer(&i, 4);
	return i;
}
int TextFile::read_int()
{
	return read_str()._int();
}

// read a float (4 bytes in binary mode)
float File::read_float()
{
	float f;
	read_buffer(&f, 4);
	return f;
}
float TextFile::read_float()
{
	return read_str()._float();
}

// read a boolean (1 byte in binary mode)
bool File::read_bool()
{
	char bb = 0;
	read_buffer(&bb, 1);
	return (bb == '1') or (bb == 0x01); // sigh, old style booleans
}
bool TextFile::read_bool()
{
	string s = read_str();
	return (s == "1");
}

// read a string
//   text mode:   complete rest of this line
//   binary mode: length word, then string
string File::read_str()
{
	// binary: read length as a word then read so many bytes
	int l = read_word();
	string str;
	str.resize(l + 16); // prevents "uninitialized" bytes in syscall parameter... (valgrind)
	read_buffer(str.data, l);
	str.resize(l);
	return str;
}
string TextFile::read_str()
{
	// read one byte at a time until we reach a \n character
	string str;
	while(true){
		char c = read_char();

		#ifdef OS_LINUX
			// windows read-function does this on its own... (O_O)
			if (c == '\r')
				continue;
		#endif

		if (c == '\n')
			break;
		str.add(c);
	}
	return str;
}

// read a null-terminated string
string File::read_str_nt()
{
	string str;
	while(true){
		char c = read_char();
		if (c == 0)
			break;
		str.add(c);
	}
	return str;
}

// read a string having reversed byte as length in binary mode
string File::read_str_rw()
{
	int l = read_word_reversed();
	string str;
	str.resize(l);
	read_buffer(str.data, l);
	return str;
}

void File::read_vector(void *v)
{
	((float*)v)[0] = read_float();
	((float*)v)[1] = read_float();
	((float*)v)[2] = read_float();
}

// write a single character (1 byte)
void File::write_char(char c)
{
	write_buffer(&c, 1);
}
void TextFile::write_char(char c)
{
	write_str(string(&c, 1));
	//write_str(i2s(c));
}

// write a single character (1 byte)
void File::write_byte(unsigned char c)
{
	write_buffer(&c, 1);
}
void TextFile::write_byte(unsigned char c)
{
	write_str(i2s(c));
}

// write a word (2 bytes)
void File::write_word(unsigned int w)
{
	write_buffer(&w, 2);
}
void TextFile::write_word(unsigned int w)
{
	write_str(i2s(w));
}

// write an integer (4 bytes)
void File::write_int(int i)
{
	write_buffer(&i, 4);
}
void TextFile::write_int(int i)
{
	write_str(i2s(i));
}

// write a float (4 bytes)
void File::write_float(float f)
{
	write_buffer(&f, 4);
}
void TextFile::write_float(float f)
{
	write_str(f2s(f, float_decimals));
}

// write a boolean (1 byte)
void File::write_bool(bool b)
{
	write_buffer(&b, 1);
}
void TextFile::write_bool(bool b)
{
	write_str(b ? "1" : "0");
}

// write a string
//   text mode:   complete rest of this line
//   binary mode: length word, then string
void File::write_str(const string &str)
{
	int num = min(str.num, 65535);
	write_word(num);
	if (num > 0)
		write_buffer(str.data, num);
}
void TextFile::write_str(const string &str)
{
	write_buffer(str.data, str.num);
	write_buffer("\n", 1);
}

// write a comment line
void File::write_comment(const string &str)
{
}
void TextFile::write_comment(const string &str)
{
#ifdef FILE_COMMENTS_DEBUG
	msg_write("comment: " + str);
#endif
	write_str(str);
}

void File::write_vector(const void *v)
{
	write_float(((float*)v)[0]);
	write_float(((float*)v)[1]);
	write_float(((float*)v)[2]);
}

