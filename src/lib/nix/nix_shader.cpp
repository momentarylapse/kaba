/*----------------------------------------------------------------------------*\
| Nix shader                                                                   |
| -> shader files                                                              |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"

Array<sShaderFile> NixShader;

int NixglShaderCurrent;



int NixLoadShader(const string &filename)
{
	if (filename.num < 1)
		return -1;
	msg_write("loading shader file " + filename.sys_filename());
	msg_right();
	sShaderFile sf;
/*#ifndef NIX_IDE_VCS
	msg_error("ignoring shader file....(no visual studio!)");
	msg_left();
	return -1;
#endif*/
	if (!OGLShaderSupport){
		msg_left();
		return -1;
	}
//	msg_todo("NixLoadShaderFile for OpenGL");
	//msg_write("-shader");
	int s = glCreateShader(GL_FRAGMENT_SHADER);
	if (s <= 0){
		msg_error("could not create gl shader object");
		msg_left();
		return -1;
	}

	if (!file_test_existence(filename + ".glsl")){
		msg_left();
		return -1;
	}
	const char *pbuf[2];
	string shader_buf = FileRead(filename + ".glsl");

	pbuf[0] = shader_buf.c_str();
	glShaderSource(s, 1, pbuf,NULL);


	//msg_write("-compile");
	glCompileShader(s);

	int status;
	glGetShaderiv(s, GL_COMPILE_STATUS, &status);
	//msg_write(status);
	if (status != GL_TRUE){
		msg_error("while compiling shader...");
		string error_str;
		error_str.resize(16384);
		int size;
		glGetShaderInfoLog(s, error_str.num, &size, (char*)error_str.data);
		error_str.resize(size);
		msg_write(error_str);
		HuiErrorBox(NixWindow, "error in shader file", error_str);
		msg_left();
		return -1;
	}

	//msg_write("-program");
	int p=glCreateProgram();
	if (p<=0){
		msg_error("could not create gl shader program");
		msg_left();
		return -1;
	}

	//msg_write("-attach");
	glAttachShader(p,s);

	//msg_write("-link");
	glLinkProgram(p);
	glGetProgramiv(p,GL_LINK_STATUS,&status);
	//msg_write(status);
	if (status!=GL_TRUE){
		msg_error("while linking the shader program...");
		string error_str;
		error_str.resize(16384);
		int size;
		glGetProgramInfoLog(s, error_str.num, &size, (char*)error_str.data);
		error_str.resize(size);
		msg_write(error_str);
		HuiErrorBox(NixWindow, "error linking shader file", error_str);
		msg_left();
		return -1;
	}
	sf.glShader = p;
	msg_write("ok?");

	NixShader.add(sf);
	msg_left();
	return NixShader.num - 1;
}

void NixDeleteShader(int index)
{
	msg_todo("NixDeleteShader");
}

void NixSetShader(int index)
{
	if (!OGLShaderSupport)
		return;
	if (index >= 0)
		NixglShaderCurrent = NixShader[index].glShader;
	else
		NixglShaderCurrent = 0;
	glUseProgram(NixglShaderCurrent);
}

void NixSetShaderData(int index, const string &var_name, const void *data, int size)
{
	if (index<0)
		return;
	sShaderFile *s = &NixShader[index];
	if (!OGLShaderSupport)
		return;
	//msg_todo("NixSetShaderData");
	NixSetShader(index);

	int loc = glGetUniformLocation(s->glShader, var_name.c_str());
	glUniform1f(loc, *(float*)data);

	NixSetShader(-1);
		
	/*int loc = glGetUniformLocationARB(my_program, “my_color_texture”);

glActiveTexture(GL_TEXTURE0 + i);
glBindTexture(GL_TEXTURE_2D, my_texture_object);

glUniform1iARB(my_sampler_uniform_location, i);*/
}

void NixGetShaderData(int index, const string &var_name, void *data, int size)
{
	if (index<0)
		return;
	if (!OGLShaderSupport)
		return;
	msg_todo("NixGetShaderData for OpenGL");
}
