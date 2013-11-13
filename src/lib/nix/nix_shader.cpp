/*----------------------------------------------------------------------------*\
| Nix shader                                                                   |
| -> shader files                                                              |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"

string NixShaderDir;

static Array<NixShader*> NixShaders;

int NixGLCurrentProgram = 0;

string NixShaderError;

int create_empty_shader_program()
{
	if (!OGLShaderSupport)
		return -1;
	int gl_p = glCreateProgram();
	TestGLError("CreateProgram");
	if (gl_p <= 0){
		msg_error("could not create gl shader program");
		return -1;
	}
	return gl_p;
}

string get_inside_of_tag(const string &source, const string &tag)
{
	string r;
	int pos0 = source.find("<" + tag + ">");
	if (pos0 < 0)
		return "";
	pos0 += tag.num + 2;
	int pos1 = source.find("</" + tag + ">", pos0);
	if (pos1 < 0)
		return "";
	return source.substr(pos0, pos1 - pos0);
}

int create_gl_shader(const string &source, int type)
{
	if (source.num == 0)
		return -1;
	int gl_shader = glCreateShader(type);
	TestGLError("CreateShader create");
	if (gl_shader <= 0){
		NixShaderError = "could not create gl shader object";
		msg_error(NixShaderError);
		return -1;
	}
	
	const char *pbuf[2];

	pbuf[0] = source.c_str();
	glShaderSource(gl_shader, 1, pbuf, NULL);
	TestGLError("CreateShader source");

	glCompileShader(gl_shader);
	TestGLError("CreateShader compile");

	int status;
	glGetShaderiv(gl_shader, GL_COMPILE_STATUS, &status);
	TestGLError("CreateShader status");
	//msg_write(status);
	if (status != GL_TRUE){
		NixShaderError.resize(16384);
		int size;
		glGetShaderInfoLog(gl_shader, NixShaderError.num, &size, (char*)NixShaderError.data);
		NixShaderError.resize(size);
		msg_error("while compiling shader: " + NixShaderError);
		return -1;
	}
	return gl_shader;
}

NixShader *NixCreateShader(const string &source)
{
	string source_vertex = get_inside_of_tag(source, "VertexShader");
	if (source_vertex.num == 0)
		source_vertex = get_inside_of_tag(source, "Vertex");
	string source_fragment = get_inside_of_tag(source, "FragmentShader");
	if (source_fragment.num == 0)
		source_fragment = get_inside_of_tag(source, "Fragment");

	if (source_vertex.num + source_fragment.num == 0){
		NixShaderError = "no shader tags found (<VertexShader>...</VertexShader> or <FragmentShader>...</FragmentShader>)";
		msg_error(NixShaderError);
		return NULL;
	}

	int gl_shader_v = create_gl_shader(source_vertex, GL_VERTEX_SHADER);
	if ((gl_shader_v < 0) && (source_vertex.num > 0))
		return NULL;
	int gl_shader_f = create_gl_shader(source_fragment, GL_FRAGMENT_SHADER);
	if ((gl_shader_f < 0) && (source_fragment.num > 0))
		return NULL;

	int gl_prog = create_empty_shader_program();
	if (gl_prog < 0)
		return NULL;


	if (gl_shader_v >= 0){
		glAttachShader(gl_prog, gl_shader_v);
		glDeleteShader(gl_shader_v);
	}
	if (gl_shader_f >= 0){
		glAttachShader(gl_prog, gl_shader_f);
		glDeleteShader(gl_shader_f);
	}
	TestGLError("AddShader attach");

	int status;
	glLinkProgram(gl_prog);
	TestGLError("AddShader link");
	glGetProgramiv(gl_prog, GL_LINK_STATUS, &status);
	TestGLError("AddShader status");
	if (status != GL_TRUE){
		NixShaderError.resize(16384);
		int size;
		glGetProgramInfoLog(gl_prog, NixShaderError.num, &size, (char*)NixShaderError.data);
		NixShaderError.resize(size);
		msg_error("while linking the shader program: " + NixShaderError);
		msg_left();
		return NULL;
	}

	NixShader *s = new NixShader;
	s->glProgram = gl_prog;
	NixShaderError = "";

	TestGLError("CreateShader");
	return s;
}

NixShader *NixLoadShader(const string &filename)
{
	if (filename.num == 0)
		return NULL;
	string fn = NixShaderDir + filename;
	foreachi(NixShader *s, NixShaders, i)
		if ((s->filename == fn) && (s->glProgram >= 0)){
			s->reference_count ++;
			return s;
		}

	msg_write("loading shader: " + fn);
	msg_right();

	string source = FileRead(fn);
	NixShader *shader = NixCreateShader(source);
	if (shader)
		shader->filename = fn;

	msg_left();
	return shader;
}

NixShader::NixShader()
{
	NixShaders.add(this);
	reference_count = 1;
	filename = "-no file-";
	glProgram = -1;
}

NixShader::~NixShader()
{
	msg_write("delete shader: " + filename);
	glDeleteProgram(glProgram);
	TestGLError("NixUnrefShader");
	glProgram = -1;
	filename = "";
}

void NixShader::unref()
{
	reference_count --;
	if ((reference_count <= 0) && (glProgram >= 0)){
		msg_write("delete shader: " + filename);
		glDeleteProgram(glProgram);
		TestGLError("NixUnrefShader");
		glProgram = -1;
		filename = "";
	}
}

void NixDeleteAllShaders()
{
	foreachib(NixShader *s, NixShaders, i)
		delete(s);
	NixShaders.clear();
}

void NixSetShader(NixShader *s)
{
	if (!OGLShaderSupport)
		return;
	NixGLCurrentProgram = 0;
	if (s)
		NixGLCurrentProgram = s->glProgram;
	glUseProgram(NixGLCurrentProgram);
	TestGLError("SetProgram");
}

void NixShader::set_data(const string &var_name, const void *data, int size)
{
	if (!OGLShaderSupport)
		return;
	NixSetShader(this);

	int loc = glGetUniformLocation(glProgram, var_name.c_str());
	glUniform1f(loc, *(float*)data);

	NixSetShader(NULL);
		
	/*int loc = glGetUniformLocationARB(my_program, “my_color_texture”);

glActiveTexture(GL_TEXTURE0 + i);
glBindTexture(GL_TEXTURE_2D, my_texture_object);

glUniform1iARB(my_sampler_uniform_location, i);*/
	TestGLError("SetShaderData");
}

void NixShader::get_data(const string &var_name, void *data, int size)
{
	if (!OGLShaderSupport)
		return;
	msg_todo("NixGetShaderData for OpenGL");
}


void NixSetDefaultShaderData(int num_textures, const vector &cam_pos)
{
	if (NixGLCurrentProgram == 0)
		return;
	int loc;
	loc = glGetUniformLocation(NixGLCurrentProgram, "tex0");
	if (loc > -1)
		glUniform1i(loc, 0);
	// glUniform1i(loc, n) sends the n'th bound texture 
	loc = glGetUniformLocation(NixGLCurrentProgram, "tex1");
	if (loc > -1)
		glUniform1i(loc, 1);
	loc = glGetUniformLocation(NixGLCurrentProgram, "tex2");
	if (loc > -1)
		glUniform1i(loc, 2);
	loc = glGetUniformLocation(NixGLCurrentProgram, "tex3");
	if (loc > -1)
		glUniform1i(loc, 3);
	loc = glGetUniformLocation(NixGLCurrentProgram, "_CamPos");
	if (loc > -1){
		matrix m = NixWorldMatrix * NixViewMatrix, mi;
		MatrixInverse(m, mi);
		vector cp = mi * v_0;
		glUniform3f(loc, cp.x, cp.y, cp.z);
	}
	TestGLError("SetDefaultShaderData");
}
