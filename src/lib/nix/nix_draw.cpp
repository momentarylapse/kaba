/*----------------------------------------------------------------------------*\
| Nix draw                                                                     |
| -> drawing functions                                                         |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#if HAS_LIB_GL

#include "nix.h"
#include "nix_common.h"

namespace nix{

extern Shader *current_shader;





void Draw3D(OldVertexBuffer *vb) {
	if (vb->dirty)
		vb->update();

	current_shader->set_default_data();

	TestGLError("a");
	glEnableVertexAttribArray(0);
	TestGLError("b1");
	glBindBuffer(GL_ARRAY_BUFFER, vb->buf_v);
	TestGLError("c1");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	TestGLError("d1");

	glEnableVertexAttribArray(1);
	TestGLError("b2");
	glBindBuffer(GL_ARRAY_BUFFER, vb->buf_n);
	TestGLError("c2");
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, (void*)0);
	TestGLError("d2");

	for (int i=0; i<vb->num_textures; i++) {
		glEnableVertexAttribArray(2 + i);
		TestGLError("b3");
		glBindBuffer(GL_ARRAY_BUFFER, vb->buf_t[i]);
		TestGLError("c3");
		glVertexAttribPointer(2+i, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		TestGLError("d3");
	}

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, 3*vb->num_triangles); // Starting from vertex 0; 3 vertices total -> 1 triangle
	TestGLError("e");

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	for (int i=0; i<vb->num_textures; i++)
		glDisableVertexAttribArray(2 + i);
	TestGLError("f");


	TestGLError("Draw3D");
}

void DrawTriangles(VertexBuffer *vb) {
	if (vb->count() == 0)
		return;
	current_shader->set_default_data();

	SetVertexBuffer(vb);

	glDrawArrays(GL_TRIANGLES, 0, vb->count()); // Starting from vertex 0; 3 vertices total -> 1 triangle

	TestGLError("DrawTriangles");
}


void DrawLines(VertexBuffer *vb, bool contiguous) {
	if (vb->count() == 0)
		return;
	current_shader->set_default_data();

	SetVertexBuffer(vb);

	if (contiguous)
		glDrawArrays(GL_LINE_STRIP, 0, vb->count());
	else
		glDrawArrays(GL_LINES, 0, vb->count());
	TestGLError("DrawLines");
}


void ResetToColor(const color &c) {
	glClearColor(c.r, c.g, c.b, c.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	TestGLError("ResetToColor");
}

void ResetZ() {
	glClear(GL_DEPTH_BUFFER_BIT);
	TestGLError("ResetZ");
}

};
#endif
