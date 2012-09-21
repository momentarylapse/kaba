#include "x.h"

//#define _debug_matrices_

bool TestVectorNSanity(vector_n &v)
{
	bool e=false;
	for (int i=0;i<v.n;i++)
		if (inf_f(v.e[i])){
			e=true;
			v.e[i]=0;
		}
	return e;
}

bool TestMatrixNSanity(matrix_n &m)
{
	bool e=false;
	for (int i=0;i<m.nr;i++)
		for (int j=0;j<m.nc;j++)
			if (inf_f(m.e[i][j])){
				e=true;
				m.e[i][j]=0;
			}
	return e;
}

vector_n DebugC,DebugdtC;

void matnout(matrix_n &m,const char *name)
{
	if (TestMatrixNSanity(m))
		msg_error(format("N-Matrix %s unendlich!!!!!!",name));
#ifndef _debug_matrices_
	return;
#endif
	msg_write(format("Matrix %s (%d x %d)",name,m.nr,m.nc));
	for (int i=0;i<m.nr;i++){
		string str;
		for (int j=0;j<m.nc;j++){
			str += "\t" + f2s(m.e[i][j],4);
		}
		msg_write(str);
	}
}

void vecnout(vector_n &v,const char *name)
{
	if (TestVectorNSanity(v))
		msg_error(format("N-Vektor %s unendlich!!!!!!",name));
#ifndef _debug_matrices_
	return;
#endif
	msg_write(format("Vektor %s (%d)",name,v.n));
	string str;
	for (int i=0;i<v.n;i++){
		str += "\t" + f2s(v.e[i],2);
	}
	msg_write(str);
}

// a = 0
void MatrixZeroN(matrix_n &a,int nr,int nc)
{
	a.nr=nr;
	a.nc=nc;
	for (int i=0;i<nr;i++)
		for (int j=0;j<nc;j++)
			a.e[i][j]=0;
}

// m = a * b
void MatrixMultiplyN(matrix_n &m,matrix_n &a,matrix_n &b)
{
	m.nr=a.nr;
	m.nc=b.nc;
	for (int i=0;i<m.nr;i++)
		for (int j=0;j<m.nc;j++){
			m.e[i][j]=0;
			for (int k=0;k<a.nc;k++)
				m.e[i][j]+=a.e[i][k]*b.e[k][j];
		}
}

// mo = mi^t
void MatrixTransposeN(matrix_n &mo,matrix_n &mi)
{
	mo.nr=mi.nc;
	mo.nc=mi.nr;
	for (int i=0;i<mo.nr;i++)
		for (int j=0;j<mo.nc;j++)
			mo.e[i][j]=mi.e[j][i];
}

// b = a * x
void MatrixVectorMultiplyN(vector_n &b,matrix_n &a,vector_n &x)
{
	b.n=a.nr;
	for (int i=0;i<a.nr;i++){
		b.e[i]=0;
		for (int j=0;j<a.nc;j++)
			b.e[i]+=a.e[i][j]*x.e[j];
	}
}

// solve( b = a * x, x );
//     a must be square!!!
void MatrixVectorSolveN(matrix_n &a,vector_n &b,vector_n &x)
{
	int i,j,k,n=b.n;
	x.n=n;
	vector_n b1,b2;
	matrix_n a1,a2;

	x.n=a.nc;

// diagonalize

	for (i=0;i<n;i++)
		for (j=0;j<n;j++)
			a2.e[i][j]=a.e[i][j];
	for (i=0;i<n;i++)
		b2.e[i]=b.e[i];

	for (k=0;k<n;k++){

		// erste Zeile (erster Koeffizient => 1)
		float p=a2.e[k][k];
		for (i=k+1;i<n;i++)
			a1.e[k][i]=a2.e[k][i]/p;
		b1.e[k]=b2.e[k]/p;
		// restliche Zeilen (erster Koeffizient => 0)
		for (j=k+1;j<n;j++){
			float t=a2.e[j][k];
			for (i=k+1;i<n;i++)
				a1.e[j][i]=a2.e[j][i]-a1.e[k][i]*t;
			b1.e[j]=b2.e[j]-b1.e[k]*t;
		}
		for (i=k+1;i<n;i++)
			for (j=k+1;j<n;j++)
				a2.e[i][j]=a1.e[i][j];
		for (i=k+1;i<n;i++)
			b2.e[i]=b1.e[i];


		/*a1.e[k][k]=1;
		for (i=k+1;i<n;i++)
			a1.e[i][k]=0;
		a1.n=n;
		matnout(a1);
		b1.n=n;
		vecnout(b1);*/
	}

// get solution
	for (k=n-1;k>=0;k--){
		x.e[k]=b1.e[k];
		for (i=k+1;i<n;i++)
			x.e[k]-=a1.e[k][i]*x.e[i];
	}

}

float VecNLength(vector_n &v)
{
	float l=0;
	for (int i=0;i<v.n;i++)
		l+=v.e[i]*v.e[i];
	return (float)sqrt(l);
}

void matout(matrix m)
{
	msg_write("Matrix:");
	msg_write(format("%f\t%f\t%f\t%f",m.e[ 0],m.e[ 4],m.e[ 8],m.e[12]));
	msg_write(format("%f\t%f\t%f\t%f",m.e[ 1],m.e[ 5],m.e[ 9],m.e[13]));
	msg_write(format("%f\t%f\t%f\t%f",m.e[ 2],m.e[ 6],m.e[10],m.e[14]));
	msg_write(format("%f\t%f\t%f\t%f",m.e[ 3],m.e[ 7],m.e[11],m.e[15]));
}

void MatOut3(matrix *t)
{
	msg_write(format("%f		%f		%f",t->_00,t->_01,t->_02));
	msg_write(format("%f		%f		%f",t->_10,t->_11,t->_12));
	msg_write(format("%f		%f		%f",t->_20,t->_21,t->_22));
}

void mat3out(matrix3 m)
{
	msg_write("Matrix3:");
	msg_write(format("%f\t%f\t%f",m._00,m._01,m._02));
	msg_write(format("%f\t%f\t%f",m._10,m._11,m._12));
	msg_write(format("%f\t%f\t%f",m._20,m._21,m._22));
}

matrix MatrixVCP(vector v)
{
	matrix m;
	m._00=0;	m._01=v.z;	m._02=-v.y;	m._03=0;
	m._10=-v.z;	m._11=0;	m._12=v.x;	m._13=0;
	m._20=v.y;	m._21=-v.x;	m._22=0;	m._23=0;
	m._30=0;	m._31=0;	m._32=0;	m._33=1;
	return m;
}
