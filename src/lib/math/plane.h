
#ifndef _MATH_PLANE_INCLUDED_
#define _MATH_PLANE_INCLUDED_

class vector;
class matrix;

class plane
{
public:
	vector n;
	float d;

	plane(){}
	plane(const vector &p, const vector &n);
	plane(const vector &a, const vector &b, const vector &c);
	string _cdecl str() const;

	bool _cdecl intersect_line(const vector &l1, const vector &l2, vector &i) const;
	float _cdecl distance(const vector &p) const;
	void _cdecl inverse();
};

// planes
void _cdecl PlaneFromPoints(plane &pl,const vector &a,const vector &b,const vector &c);
void _cdecl PlaneFromPointNormal(plane &pl,const vector &p,const vector &n);
void _cdecl PlaneTransform(plane &plo,const matrix &m,const plane &pli);
void _cdecl GetBaryCentric(const vector &P,const vector &A,const vector &B,const vector &C,float &f,float &g);
extern float LineIntersectsTriangleF,LineIntersectsTriangleG;
bool _cdecl LineIntersectsTriangle(const vector &t1,const vector &t2,const vector &t3,const vector &l1,const vector &l2,vector &col,bool vm);
bool _cdecl LineIntersectsTriangle2(const plane &pl, const vector &t1,const vector &t2,const vector &t3,const vector &l1,const vector &l2,vector &col,bool vm);


inline void _plane_from_point_normal_(plane &pl,const vector &p,const vector &n)
{
	pl.n=n;
	pl.d=-(n*p);
}

inline bool _plane_intersect_line_(vector &cp,const plane &pl,const vector &l1,const vector &l2)
{
	float e=pl.n*l1;
	float f=pl.n*l2;
	if (e==f) // parallel?
		return false;
	float t=-(pl.d+f)/(e-f);
	//if ((t>=0)&&(t<=1)){
		//cp = l1 + t*(l2-l1);
		cp = l2 + t*(l1-l2);
	return true;
}

inline float _plane_distance_(const plane &pl,const vector &p)
{	return pl.n*p + pl.d;	}

inline void _get_bary_centric_(const vector &p,const plane &pl,const vector &a,const vector &b,const vector &c,float &f,float &g)
{
	vector ba=b-a,ca=c-a;
	vector pvec=pl.n^ca;
	float det=ba*pvec;
	vector pa;
	if (det>0)
		pa=p-a;
	else{
		pa=a-p;
		det=-det;
	}
	f=pa*pvec;
	vector qvec=pa^ba;
	g=pl.n*qvec;
	float inv_det=1.0f/det;
	f*=inv_det;
	g*=inv_det;
}


#endif
