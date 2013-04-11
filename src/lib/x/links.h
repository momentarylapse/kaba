#if !defined(LINKS_H)
#define LINKS_H



#ifdef _X_ALLOW_ODE_
#define dSINGLE
#include <ode/ode.h>
#endif


struct sLink{
	int type;
	Object *o1, *o2;
	vector p, rho1, rho2, d1, d2, d3, d4;
	float param_f1, param_f2, friction;
	float k_s, k_d, c_fdf, c_fdt;
	vector_n lambda;

	// contact
	vector f1, f2, t1, t2, n, f_fric, dv;
	vector sf1, sf2, st1, st2;

#ifdef _X_ALLOW_ODE_
	dJointID joint_id, motor_id;
#endif
};

// all:
//   rho1			- link point in o1's frame
//   rho2			- link point in o2's frame
// LinkTypeHinge:
//   d1				- link axis in o1's frame
//   d2				- link axis in o2's frame
//   d3,d4			- 2 vectors orthogonal to the link axis in o1's frame


enum{
	LinkTypeNone,
	LinkTypeBall,
	LinkTypeHinge,
	LinkTypeStatic,
	LinkTypeContact,
	LinkTypeSpring,
	LinkTypeHinge2,
	LinkTypeSlider,
	LinkTypeUniversal,
	// debug...
	LinkTypeFix,
	LinkTypeFixPoint1Rot,
	LinkTypeFixPointSphere,
	LinkTypexxxxx
};


extern Array<sLink> Link;

void LinksReset();
int _cdecl AddLinkSpring(Object *o1, Object *o2, const vector &p1, const vector &p2, float dx0, float k);
int _cdecl AddLinkBall(Object *o1, Object *o2, const vector &p);
int _cdecl AddLinkHinge(Object *o1, Object *o2, const vector &p, const vector &ax);
int _cdecl AddLinkHinge2(Object *o1, Object *o2, const vector &p, const vector &ax1, const vector &ax2);
int _cdecl AddLinkSlider(Object *o1, Object *o2, const vector &ax);
int _cdecl AddLinkUniversal(Object *o1, Object *o2, const vector &p, const vector &ax1, const vector &ax2);

void _cdecl AddLinkContact(Object *o1, Object *o2, const vector &cp, const vector &n, float depth, const vector &dv, float c_static, float c_dynamic);

/*int _cdecl AddLinkSpring(Object *o1, Object *o2, const vector &rho1, const vector &rho2, float dx, float k);
int _cdecl AddLinkHinge(Object *o1, Object *o2, const vector &rho1, const vector &rho2, const vector &ax1, const vector &ax2);
int _cdecl AddLinkHingeAbs(Object *o1, Object *o2, const vector &p, const vector &ax);
int _cdecl AddLinkBall(Object *o1, Object *o2, const vector &rho1, const vector &rho2);
int _cdecl AddLinkBallAbs(Object *o1, Object *o2, const vector &p);*/
void DoLinks(int steps);
void GodGetLinkedList(Object *o, Array<Object*> &list);
//void _cdecl LinkHingeSetTorque(int l, float t);
//void _cdecl LinkHingeSetAxis(int l, const vector &ax1, const vector &ax2);

void _cdecl LinkSetTorque(int l, float t);
void _cdecl LinkSetTorqueAxis(int l, int axis, float t);
void _cdecl LinkSetRange(int l, float min, float max);
void _cdecl LinkSetRangeAxis(int l, int axis, float min, float max);
void _cdecl LinkSetFriction(int l, float f);
float _cdecl LinkGetPosition(int l);
float _cdecl LinkGetPositionAxis(int l, int axis);

void LinkCalcContactFriction();
void LinkRemoveContacts();

inline bool ObjectsLinked(Object *o1, Object *o2)
{
	//return false;
	for (int l=0;l<Link.num;l++){
		if ((Link[l].o1==o1)&&(Link[l].o2==o2))
			return true;
		if ((Link[l].o1==o2)&&(Link[l].o2==o1))
			return true;
	}
	return false;
}

#endif
