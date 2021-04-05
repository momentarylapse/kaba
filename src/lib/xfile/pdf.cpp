/*
 * pdf.cpp
 *
 *  Created on: 22.04.2018
 *      Author: michi
 */

#include "pdf.h"
#include "../image/image.h"
#include "../image/Painter.h"
#include "../math/math.h"
#include "../file/file.h"

namespace pdf {

Array<Path> font_paths;


class TTF {
public:


	struct BEUShort {
		unsigned char c[2];
		int _int() {
			unsigned int a = c[0];
			unsigned int b = c[1];
			return (a << 8) + b;
		}
	};
	struct BESShort {
		unsigned char c[2];
		int _int() {
			unsigned int a = c[0];
			unsigned int b = c[1];
			unsigned int r = (a << 8) + b;
			if ((r & 0x8000) != 0)
				r -= 1<<16;
			return r;
		}
	};
	struct BELong {
		unsigned char x[4];
		int _int() {
			unsigned int a = x[0];
			unsigned int b = x[1];
			unsigned int c = x[2];
			unsigned int d = x[3];
			return (a << 24) + (b << 16) + (c << 8) + d;
		}
	};

	struct Fixed {
		BEUShort a, b;
	};


	static int readUS(File *f) {
		BEUShort t;
		f->read_buffer(&t, 2);
		return t._int();
	}

	static int readSS(File *f) {
		BESShort t;
		f->read_buffer(&t, 2);
		return t._int();
	}

	struct TableDirectory {
		Fixed version;
		BEUShort num_tables;
		BEUShort seach_range;
		BEUShort entry_selector;
		BEUShort range_shift;
	};

	struct TableDirectoryEntry {
		BELong tag, chksum, offset, length;
		string name() {
			string s;
			s.add(tag.x[0]);
			s.add(tag.x[1]);
			s.add(tag.x[2]);
			s.add(tag.x[3]);
			return s;
		}
	};

	struct Header {
		Fixed version;
		Fixed revision;
		BELong chksm;
		BELong magic;
		BEUShort flags;
		BEUShort units_per_em;
		// more
	};

	TableDirectory td;
	Array<TableDirectoryEntry> tdentries;
	Header head;

	Array<int> lsb, advance;


	TableDirectoryEntry* get_table(const string &tag) {
		for (auto &ee: tdentries)
			if (tag == ee.name())
				return &ee;
		return nullptr;
	}

	void read_table_directory(File *f) {
		f->read_buffer(&td, 12);
		int n = td.num_tables._int();
		if (n > 1000)
			throw Exception("argh");
		for (int i=0; i<n; i++) {
			TableDirectoryEntry e;
			f->read_buffer(&e, 16);
			tdentries.add(e);
		}

		//for (auto &ee: tdentries)
		//	msg_write(format("%s  %d", ee.name(), ee.offset._int()));
	}



	bool try_read_hhead(File *f) {
		auto te = get_table("hhea");
		if (!te)
			return false;
		f->set_pos(te->offset._int());
		//msg_write("hhead-----------------------------");

		f->seek(34);
		int n = readUS(f);
		//msg_write(n);
		lsb.resize(n);
		advance.resize(n);
		return true;
	}

	bool try_read_hmetrix(File *f) {
		auto te = get_table("hmtx");
		if (!te)
			return false;
		f->set_pos(te->offset._int());
		//msg_write("hmetrix-----------------------------");
		for (int i=0; i<lsb.num; i++) {
			advance[i] = readUS(f);
			lsb[i] = readSS(f);
		}
		//msg_write(ia2s(advance));
		return true;
	}

	void load(const Path &filename) {
		auto f = FileOpen(filename);

		read_table_directory(f);
		//read_head(f);
		//read_mapping(f);
		try_read_hhead(f);
		try_read_hmetrix(f);
		//read_glyphs();

		//update_codes()
		//update_size()

		delete f;
	}
};

Path find_ttf(const string &name) {
	string fname = name + ".ttf";
	for (auto &dir: font_paths)
		if (file_exists(dir << fname)) {
			return dir << fname;
		}
	return "";
}

void load_ttf(const string &name) {
	auto filename = find_ttf(name);
	if (!filename.is_empty()) {
		//msg_write(filename.str());
		TTF ttf;
		ttf.load(filename);
	} else {
		msg_error(format("ttf font not found: %s", name));
	}
}



PagePainter::PagePainter(Parser* _parser, Page *_page) {
	parser = _parser;
	page = _page;
	width = page->width;
	height = page->height;
	col = new color(1,0,0,0);
	line_width = 1;
	font_size = 12;
	font_name = "Helvetica";
	filling = true;
	text_x = text_y = 0;
	set_color(Black);
}

PagePainter::~PagePainter() {
	delete col;
}

void PagePainter::set_color(const color& c) {
	*col = c;
	page->content += format("     %.2f %.2f %.2f RG\n", c.r, c.g, c.b);
	page->content += format("     %.2f %.2f %.2f rg\n", c.r, c.g, c.b);
	//page->content += format("     %.2f CA\n", c.a);
	//page->content += format("     %.2f ca\n", c.a);
}

void PagePainter::set_font(const string& font, float size, bool bold, bool italic) {
	font_name = font;
	font_size = size;
}

void PagePainter::set_font_size(float size) {
	font_size = size;
}

void PagePainter::set_antialiasing(bool enabled) {
}

void PagePainter::set_line_width(float w) {
	line_width = w;
	page->content += format("     %.1f w\n", w);
}

void PagePainter::set_line_dash(const Array<float>& dash, float offset) {
	page->content += "     [";
	for (float d: dash)
		page->content += format("%.0f ", d);
	page->content += format("] %.0f d\n", offset);
}

void PagePainter::set_fill(bool fill) {
	filling = fill;
}

void PagePainter::set_clip(const rect& r) {
}

void PagePainter::draw_point(float x, float y) {
}

void PagePainter::draw_line(float x1, float y1, float x2, float y2) {
	page->content += format("     %.1f %.1f m\n", x1, height-y1);
	page->content += format("     %.1f %.1f l\n", x2, height-y2);
	page->content += "     S\n";
}

void PagePainter::draw_lines(const Array<complex>& p) {
	if (p.num < 2)
		return;
	page->content += format("     %.1f %.1f m\n", p[0].x, height-p[0].y);
	for (int i=1; i<p.num; i++)
		page->content += format("     %.1f %.1f l\n", p[i].x, height-p[i].y);
	page->content += "     S\n";
}

void PagePainter::draw_polygon(const Array<complex>& p) {
	if (p.num < 2)
		return;
	page->content += format("     %.1f %.1f m\n", p[0].x, height-p[0].y);
	for (int i=1; i<p.num; i++)
		page->content += format("     %.1f %.1f l\n", p[i].x, height-p[i].y);
	if (filling)
		page->content += "     f\n";
	else
		page->content += "     s\n";
}

void PagePainter::draw_rect(float x1, float y1, float w, float h) {
	draw_rect(rect(x1, x1+w, y1, y1+h));
	//page->content += format("     %.1f %.1f %.1f %.1f re\n", x1, height-y1-h, w, h);
}

void PagePainter::draw_rect(const rect& r) {
	page->content += format("     %.1f %.1f %.1f %.1f re\n", r.x1, height-r.y2, r.width(), r.height());
	if (filling)
		page->content += "     f\n";
	else
		page->content += "     S\n";
}


// TODO optimize
void PagePainter::draw_circle(float x, float y, float radius) {
	complex p[12];
	float rr = radius * 0.6f;
	p[0] = complex(x,        height-y-radius);
	p[1] = complex(x+rr,     height-y-radius);
	p[2] = complex(x+radius, height-y-rr);
	p[3] = complex(x+radius, height-y);
	p[4] = complex(x+radius, height-y+rr);
	p[5] = complex(x+rr,     height-y+radius);
	p[6] = complex(x,        height-y+radius);
	p[7] = complex(x-rr,     height-y+radius);
	p[8] = complex(x-radius, height-y+rr);
	p[9] = complex(x-radius, height-y);
	p[10]= complex(x-radius, height-y-rr);
	p[11]= complex(x-rr,     height-y-radius);
	page->content += format("     %.1f %.1f m\n", p[0].x, p[0].y);
	for (int i=0; i<12; i+=3)
		page->content += format("     %.1f %.1f %.1f %.1f %.1f %.1f c\n", p[i+1].x, p[i+1].y, p[i+2].x, p[i+2].y, p[(i+3)%12].x, p[(i+3)%12].y);
	if (filling)
		page->content += "     f\n";
	else
		page->content += "     S\n";
}

static string _pdf_str_filter(const string &str) {
	string x = str.replace(u8"\u266f", "#").replace(u8"\u266d", "b");
	x = x.replace(u8"ä", "ae").replace(u8"ö", "oe").replace(u8"ü", "ue").replace(u8"ß", "ss");
	x = x.replace(u8"Ä", "Ae").replace(u8"Ö", "Oe").replace(u8"Ü", "Ue");
	return x;
}

void PagePainter::draw_str(float x, float y, const string& str) {
	y = height - y - font_size*0.8f;
	float dx = x - text_x;
	float dy = y - text_y;
	auto f = parser->font_get(font_name);
	page->content += format("     %s %.1f Tf\n     %.2f %.2f Td\n     (%s) Tj\n", f->internal_name, font_size, dx, dy, _pdf_str_filter(str));

	text_x = x;
	text_y = y;
}

float PagePainter::get_str_width(const string& str) {
	return font_size * str.num * 0.5f;
}

void PagePainter::draw_image(float x, float y, const Image *image) {
}

void PagePainter::draw_mask_image(float x, float y, const Image *image) {
}

rect PagePainter::area() const {
	return rect(0, width, 0, height);
}

rect PagePainter::clip() const {
	return area();
}

Parser::Parser() {
	current_painter = nullptr;
	page_width = 595.276f;
	page_height = 841.89f;
}

Parser::~Parser() {
	if (current_painter)
		delete current_painter;
}

void Parser::__init__() {
	new(this) Parser;
}

void Parser::__delete__() {
	this->Parser::~Parser();
}


void Parser::set_page_size(float w, float h) {
	page_width = w;
	page_height = h;
}

Painter* Parser::add_page() {
	auto page = new Page;
	page->width = page_width;
	page->height = page_height;
	pages.add(page);
	if (current_painter)
		delete current_painter;
	return new PagePainter(this, page);
}


const Array<string> PDF_DEFAULT_FONTS = {"Times", "Courier", "Helvetica"};

Parser::FontData *Parser::font_get(const string &name) {
	for (auto &f: font_data)
		if (name == f.name)
			return &f;
	FontData fd;
	fd.name = name;
	fd.internal_name = format("/F%d", font_data.num+1);
	fd.true_type = false;
	fd.id = -1;
	fd.id_widths = -1;
	fd.id_descr = -1;
	fd.id_file = -1;

	if (!sa_contains(PDF_DEFAULT_FONTS, name)) {
		auto ff = find_ttf(fd.name);
		if (!ff.is_empty()) {
			fd.true_type = true;
			fd.file_contents = FileRead(ff);
			load_ttf(name);

			for (int k=0; k<256; k++)
				fd.widths.add(800);
		} else {
			msg_error("font not found: " + name);
		}
	}
	font_data.add(fd);
	return &font_data.back();
}

void Parser::save(const Path &filename) {
	auto f = FileCreate(filename);
	Array<int> pos;
	Array<int> page_id;
	for (int i=0; i<pages.num; i++)
		page_id.add(4 + i);
	Array<int> stream_id;
	for (int i=0; i<pages.num; i++)
		stream_id.add(4 + pages.num + i);

	int proc_id = 4 + pages.num * 2;
	int next_id = proc_id + 1;

	// preparing fonts
	foreachi(auto &fd, font_data, i) {
		if (fd.true_type) {
			auto ff = find_ttf(fd.name);
			if (!ff.is_empty()) {
				fd.id_file = next_id ++;
				fd.file_contents = FileRead(ff);
			}
			fd.id_descr = next_id ++;
			fd.id_widths = next_id ++;
		}
		fd.id = next_id ++;
	}
	int xref_id = next_id;


	f->write_buffer("%PDF-1.4\n");
	pos.add(f->get_pos());
	f->write_buffer("1 0 obj\n");
	f->write_buffer("<< /Type /Catalog\n");
	f->write_buffer("   /Outlines 2 0 R\n");
	f->write_buffer("   /Pages 3 0 R\n");
	f->write_buffer(">>\n");
	f->write_buffer("endobj\n");
	pos.add(f->get_pos());
	f->write_buffer("2 0 obj\n");
	f->write_buffer("<< /Type Outlines\n");
	f->write_buffer("   /Count 0\n");
	f->write_buffer(">>\n");
	f->write_buffer("endobj\n");
	pos.add(f->get_pos());
	f->write_buffer("3 0 obj\n");
	f->write_buffer("<< /Type /Pages\n");
	f->write_buffer("   /Kids [");
	for (int i=0; i<pages.num; i++) {
		if (i > 0)
			f->write_buffer(" ");
		f->write_buffer(format("%d 0 R", page_id[i]));
	}
	f->write_buffer("]\n");
	f->write_buffer(format("   /Count %d\n", pages.num));
	f->write_buffer(">>\n");
	f->write_buffer("endobj\n");

	// pages
	foreachi(auto p, pages, i) {
		pos.add(f->get_pos());
		f->write_buffer(format("%d 0 obj\n", page_id[i]));
		f->write_buffer("<< /Type /Page\n");
		f->write_buffer("   /Parent 3 0 R\n");
		f->write_buffer(format("   /MediaBox [0 0 %.3f %.3f]\n", p->width, p->height));
		f->write_buffer(format("   /Contents %d 0 R\n", stream_id[i]));
		f->write_buffer(format("   /Resources << /ProcSet %d 0 R\n", proc_id));
		string fff;
		for (auto &fd: font_data)
			fff += format("%s %d 0 R ", fd.internal_name, fd.id);
		f->write_buffer("                 /Font << " + fff + ">>\n");
		f->write_buffer("              >>\n");
		f->write_buffer(">>\n");
	}

	// streams
	foreachi(auto p, pages, i) {
		pos.add(f->get_pos());
		f->write_buffer(format("%d 0 obj\n", stream_id[i]));
		f->write_buffer(format("<< /Length %d >>\n", p->content.num));
		f->write_buffer("stream\n");
		f->write_buffer("  BT\n");
		f->write_buffer(p->content);
		f->write_buffer("  ET\n");
		f->write_buffer("endstream\n");
		f->write_buffer("endobj\n");
	}

	// proc
	pos.add(f->get_pos());
	f->write_buffer(format("%d 0 obj\n", proc_id));
	f->write_buffer("  [/PDF]\n");
	f->write_buffer("endobj\n");

	// fonts
	for (auto &fd: font_data) {

		if (fd.id_file > 0) {
			pos.add(f->get_pos());
			f->write_buffer(format("%d 0 obj\n", fd.id_file));
			f->write_buffer(format("<< /Filter /ASCIIHexDecode /Length %d >>\n", fd.file_contents.num*3));
			f->write_buffer("stream\n");
			f->write_buffer(fd.file_contents.hex().replace("."," ") + " >\n");
			f->write_buffer("endstream\n");
			f->write_buffer("endobj\n");
		}

		if (fd.id_descr > 0) {
			pos.add(f->get_pos());
			f->write_buffer(format("%d 0 obj\n", fd.id_descr));
			f->write_buffer("<< /Type /FontDescriptor\n");
			f->write_buffer(format("   /FontName %s\n", fd.internal_name));
			f->write_buffer("   /Flags 4\n");
			f->write_buffer("   /FontBBox [ -500 -300 1300 900 ]\n");
			f->write_buffer("   /ItalicAngle 0\n");
			f->write_buffer("   /Ascent 900\n");
			f->write_buffer("   /Descent -200\n");
			f->write_buffer("   /CapHeight 900\n");
			f->write_buffer("   /StemV 80\n");
			if (fd.id_file > 0)
				f->write_buffer(format("   /FontFile2 %d 0 R\n", fd.id_file));
			f->write_buffer("  >>\n");
			f->write_buffer("endobj\n");
		}
		if (fd.id_widths > 0) {
			pos.add(f->get_pos());
			f->write_buffer(format("%d 0 obj\n", fd.id_widths));
			f->write_buffer(ia2s(fd.widths).replace(",", "") + "\n");
			f->write_buffer("endobj\n");
		}

		pos.add(f->get_pos());
		f->write_buffer(format("%d 0 obj\n", fd.id));
		f->write_buffer("<< /Type /Font\n");
		if (fd.true_type) {
			f->write_buffer("   /Subtype /TrueType\n");
			f->write_buffer("   /FirstChar 0\n");
			f->write_buffer("   /LastChar 255\n");
		} else {
			f->write_buffer("   /Subtype /Type1\n");
		}
		f->write_buffer(format("   /Name %s\n", fd.internal_name));
		f->write_buffer(format("   /BaseFont /%s\n", fd.name));
		f->write_buffer("   /Encoding /MacRomanEncoding\n");
		if (fd.id_widths > 0)
			f->write_buffer(format("   /Widths %d 0 R\n", fd.id_widths));
		if (fd.id_descr > 0)
			f->write_buffer(format("   /FontDescriptor %d 0 R\n", fd.id_descr));
		f->write_buffer(">>\n");
		f->write_buffer("endobj\n");
	}

	int xref_pos = f->get_pos();
	f->write_buffer("xref\n");
	f->write_buffer(format("0 %d\n", xref_id));
	f->write_buffer("0000000000 65535 f\n");
	for (int p: pos)
		f->write_buffer(format("%010d 00000 n\n", p));
	f->write_buffer("trailer\n");
	f->write_buffer(format("<< /Size %d\n", xref_id));
	f->write_buffer("   /Root 1 0 R\n");
	f->write_buffer(">>\n");
	f->write_buffer("startxref\n");
	f->write_buffer(format("%d\n", xref_pos));

	f->write_buffer("%%EOF\n");
	delete f;
}


void add_font_directory(const Path &dir) {
	font_paths.add(dir);
}

}
