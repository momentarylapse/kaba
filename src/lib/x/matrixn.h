#if !defined(MATRIXN_H)
#define MARIXN_H

// nr x nc - matrix
struct matrix_n{
	int nr,nc;
	float e[16][16];
};

struct vector_n{
	int n;
	float e[16];
};

bool TestVectorNSanity(vector_n &v);
bool TestMatrixNSanity(matrix_n &m);

extern vector_n DebugC,DebugdtC;

void matnout(matrix_n &m,const char *name="");
void vecnout(vector_n &v,const char *name="");
void MatrixZeroN(matrix_n &a,int nr,int nc);
void MatrixMultiplyN(matrix_n &m,matrix_n &a,matrix_n &b);
void MatrixTransposeN(matrix_n &mo,matrix_n &mi);
void MatrixVectorMultiplyN(vector_n &b,matrix_n &a,vector_n &x);
void MatrixVectorSolveN(matrix_n &a,vector_n &b,vector_n &x);
float VecNLength(vector_n &v);
void matout(matrix m);
void MatOut3(matrix *t);
void mat3out(matrix3 m);
matrix MatrixVCP(vector v);

#endif
