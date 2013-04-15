#include "x.h"

#define APPLY_FORCES_LATER


Array<sLink> Link;

#ifdef _X_ALLOW_PHYSICS_DEBUG_
#define MaxObjects 256

vector CForce[MaxObjects],CTorque[MaxObjects];
#endif

char *v2s(vector v);

#ifdef object
#undef object
#endif

#ifdef _X_ALLOW_ODE_
#define USE_ODE
#endif



//------------------------------------------------------------------------------
// clusters

struct sCluster
{
	Array<Object> object;
	//Array<sLink> link;
};
Array<sCluster> Cluster;


//------------------------------------------------------------------------------
// links

void LinksReset()
{
	Link.clear();
	for (int i=0;i<Cluster.num;i++)
		Cluster[i].object.clear();
	Cluster.clear();
}

inline void ob_add_force_torque(Object *o, const vector &force, const vector &torque)
{
	o->force_int += force;
	o->torque_int += torque;
#ifdef USE_ODE
	dBodyAddForce(o->body_id, force.x, force.y, force.z);
	dBodyAddTorque(o->body_id, torque.x, torque.y, torque.z);
#endif
}

inline void ob_add_force(Object *o, const vector &force, const vector &rho)
{
	ob_add_force_torque(o, force, rho ^ force);
}

sLink *AddLink(Object *o1, Object *o2, int type)
{
	sLink l;
	memset(&l, 0, sizeof(l));
	l.o1 = o1;
	l.o2 = o2;
	l.type = type;
	l.k_s = 2.9f;
	l.k_d = 2.9f;
	l.c_fdt = 1000;
	Link.add(l);
	return &Link.back();
}


int AddLinkSpring(Object *o1, Object *o2, const vector &p1, const vector &p2, float dx0, float k)
{
	msg_db_r("AddLinkSpring", 2);
	sLink *l = AddLink(o1, o2, LinkTypeSpring);
	matrix im1, im2;
	MatrixInverse(im1, o1->_matrix);
	MatrixInverse(im2, o2->_matrix);
	l->rho1 = im1 * p1;
	l->rho2 = im2 * p2;
	l->param_f1 = (p1 - p2).length() + dx0;
	l->param_f2 = k;
	msg_db_l(2);
	return Link.num - 1;
}

int AddLinkBall(Object *o1, Object *o2, const vector &p)
{
	msg_db_r("AddLinkBall", 2);
	sLink *l = AddLink(o1, o2, LinkTypeBall);
	l->k_s = 2000;
	l->k_d = 200;
	matrix im1, im2;
	MatrixInverse(im1, o1->_matrix);
	MatrixInverse(im2, o2->_matrix);
	l->rho1 = im1 * p;
	l->rho2 = im2 * p;
#ifdef USE_ODE
	l->d4 = v_0;
	l->joint_id = dJointCreateBall(world_id, 0);
	dJointAttach(l->joint_id, o1->body_id, o2->body_id);
	dJointSetBallAnchor(l->joint_id, p.x, p.y, p.z);
	l->motor_id = dJointCreateAMotor(world_id, 0);
	dJointAttach(l->motor_id, o1->body_id, o2->body_id);
	dJointSetAMotorMode(l->motor_id, dAMotorEuler);
	dJointSetAMotorNumAxes(l->motor_id, 3);
	dJointSetAMotorAxis(l->motor_id, 0, 1, 1,0,0);
	dJointSetAMotorAxis(l->motor_id, 1, 1, 0,1,0);
	dJointSetAMotorAxis(l->motor_id, 2, 1, 0,0,1);
#endif
	msg_db_l(2);
	return Link.num - 1;
}

int AddLinkHinge(Object *o1, Object *o2, const vector &p, const vector &ax)
{
	msg_db_r("AddLinkHinge", 2);
	sLink *l = AddLink(o1, o2, LinkTypeHinge);
	l->k_s = 2000;
	l->k_d = 200;
	matrix im1, im2;
	MatrixInverse(im1, o1->_matrix);
	MatrixInverse(im2, o2->_matrix);
	l->rho1 = im1 * p;
	l->rho2 = im2 * p;
	l->d1 = ax.transform_normal(im1);
	l->d2 = ax.transform_normal(im2);
	l->d1.normalize();
	l->d2.normalize();
	// create some vectors building an ONB with ax1
	l->d3 = l->d1.ortho();
	l->d4 = l->d1 ^ l->d3;
	l->d4.normalize();
#ifdef USE_ODE
	l->joint_id = dJointCreateHinge(world_id, 0);
	dJointAttach(l->joint_id, o1->body_id, o2->body_id);
	dJointSetHingeAnchor(l->joint_id, p.x, p.y, p.z);
	dJointSetHingeAxis(l->joint_id, ax.x, ax.y, ax.z);
#endif
	msg_db_l(2);
	return Link.num - 1;
}

int AddLinkHinge2(Object *o1, Object *o2, const vector &p, const vector &ax1, const vector &ax2)
{
	msg_db_r("AddLinkHinge2", 2);
#ifdef USE_ODE
	sLink *l = AddLink(o1, o2, LinkTypeHinge2);
	l->joint_id = dJointCreateHinge2(world_id, 0);
	dJointAttach(l->joint_id, o1->body_id, o2->body_id);
	dJointSetHinge2Anchor(l->joint_id, p.x, p.y, p.z);
	dJointSetHinge2Axis1(l->joint_id, ax1.x, ax1.y, ax1.z);
	dJointSetHinge2Axis2(l->joint_id, ax2.x, ax2.y, ax2.z);
	msg_db_l(2);
	return Link.num - 1;
#else
	msg_todo("AddLinkHinge2");
	msg_db_l(2);
	return - 1;
#endif
}

int AddLinkSlider(Object *o1, Object *o2, const vector &ax)
{
	msg_db_r("AddLinkSlider", 2);
#ifdef USE_ODE
	sLink *l = AddLink(o1, o2, LinkTypeSlider);
	l->joint_id = dJointCreateSlider(world_id, 0);
	dJointAttach(l->joint_id, o1->body_id, o2->body_id);
	dJointSetSliderAxis(l->joint_id, ax.x, ax.y, ax.z);
	msg_db_l(2);
	return Link.num - 1;
#else
	msg_todo("AddLinkSlider");
	msg_db_l(2);
	return - 1;
#endif
}

int AddLinkUniversal(Object *o1, Object *o2, const vector &p, const vector &ax1, const vector &ax2)
{
	msg_db_r("AddLinkUniversal", 2);
#ifdef USE_ODE
	sLink *l = AddLink(o1, o2, LinkTypeUniversal);
	l->joint_id = dJointCreateUniversal(world_id, 0);
	dJointAttach(l->joint_id, o1->body_id, o2->body_id);
	dJointSetUniversalAnchor(l->joint_id, p.x, p.y, p.z);
	dJointSetUniversalAxis1(l->joint_id, ax1.x, ax1.y, ax1.z);
	dJointSetUniversalAxis2(l->joint_id, ax2.x, ax2.y, ax2.z);
	msg_db_l(2);
	return Link.num - 1;
#else
	msg_todo("AddLinkUniversal");
	msg_db_l(2);
	return - 1;
#endif
}

float LinkGetPosition(int l)
{
#ifdef USE_ODE	
	if (l < 0)
		return 0;
	if (Link[l].type == LinkTypeHinge)
		return dJointGetHingeAngle(Link[l].joint_id);
	else if (Link[l].type == LinkTypeHinge2)
		return dJointGetHinge2Angle1(Link[l].joint_id);
	else if (Link[l].type == LinkTypeSlider)
		return dJointGetSliderPosition(Link[l].joint_id);
	else if (Link[l].type == LinkTypeUniversal)
		return dJointGetUniversalAngle1(Link[l].joint_id);
#endif
	return 0;
}

float LinkGetPositionAxis(int l, int axis)
{
#ifdef USE_ODE	
	if (l < 0)
		return 0;
	if (Link[l].type == LinkTypeHinge)
		return dJointGetHingeAngle(Link[l].joint_id);
	else if (Link[l].type == LinkTypeHinge2){
		return dJointGetHinge2Angle1(Link[l].joint_id);
	}else if (Link[l].type == LinkTypeSlider)
		return dJointGetSliderPosition(Link[l].joint_id);
	else if (Link[l].type == LinkTypeUniversal){
		if (axis == 0)
			return dJointGetUniversalAngle1(Link[l].joint_id);
		if (axis == 1)
			return dJointGetUniversalAngle2(Link[l].joint_id);
	}else if (Link[l].type == LinkTypeBall)
		return dJointGetAMotorAngle(Link[l].joint_id, axis);
#endif
	return 0;
}

void LinkSetTorque(int l, float t)
{
	if (l < 0)
		return;
	if (Link[l].type == LinkTypeHinge){
		Link[l].param_f1 = t;
	}else if (Link[l].type == LinkTypeHinge2){
		Link[l].param_f1 = t;
	}else if (Link[l].type == LinkTypeBall){
		Link[l].param_f1 = t;
		Link[l].param_f2 = t;
	}
}

void LinkSetTorqueAxis(int l, int axis, float t)
{
	if (l < 0)
		return;
	msg_todo("LinkSetTorqueAxis");
	if (Link[l].type == LinkTypeHinge){
		//dJointAddHingeTorque(Link[l].joint_id, t);
		Link[l].param_f1 = t;
	}else if (Link[l].type == LinkTypeHinge2){
		//dJointAddHinge2Torques(Link[l].joint_id, t, 0);
		Link[l].param_f1 = t;
	}
}

void LinkSetFriction(int l, float f)
{
	if (l < 0)
		return;
	Link[l].friction = f;
}

void LinkSetRange(int l, float min, float max)
{
#ifdef USE_ODE	
	if (l < 0)
		return;
	if (Link[l].type == LinkTypeHinge){
		dJointSetHingeParam(Link[l].joint_id, dParamLoStop, min);
		dJointSetHingeParam(Link[l].joint_id, dParamHiStop, max);
	}else if (Link[l].type == LinkTypeHinge2){
		dJointSetHinge2Param(Link[l].joint_id, dParamLoStop, min);
		dJointSetHinge2Param(Link[l].joint_id, dParamHiStop, max);
	}else if (Link[l].type == LinkTypeBall){
		dJointSetAMotorParam(Link[l].motor_id, dParamLoStop, min);
		dJointSetAMotorParam(Link[l].motor_id, dParamHiStop, max);
		dJointSetAMotorParam(Link[l].motor_id, dParamLoStop2, min);
		dJointSetAMotorParam(Link[l].motor_id, dParamHiStop2, max);
		dJointSetAMotorParam(Link[l].motor_id, dParamLoStop3, min);
		dJointSetAMotorParam(Link[l].motor_id, dParamHiStop3, max);
	}else if (Link[l].type == LinkTypeSlider){
		dJointSetSliderParam(Link[l].joint_id, dParamLoStop, min);
		dJointSetSliderParam(Link[l].joint_id, dParamHiStop, max);
	}else if (Link[l].type == LinkTypeUniversal){
		dJointSetUniversalParam(Link[l].joint_id, dParamLoStop, min);
		dJointSetUniversalParam(Link[l].joint_id, dParamHiStop, max);
		dJointSetUniversalParam(Link[l].joint_id, dParamLoStop2, min);
		dJointSetUniversalParam(Link[l].joint_id, dParamHiStop2, max);
	}
#endif
}

void LinkSetRangeAxis(int l, int axis, float min, float max)
{
#ifdef USE_ODE	
	if (l < 0)
		return;
	if (Link[l].type == LinkTypeHinge){
		dJointSetHingeParam(Link[l].joint_id, dParamLoStop, min);
		dJointSetHingeParam(Link[l].joint_id, dParamHiStop, max);
	}else if (Link[l].type == LinkTypeHinge2){
		if (axis == 0){
			dJointSetHinge2Param(Link[l].joint_id, dParamLoStop, min);
			dJointSetHinge2Param(Link[l].joint_id, dParamHiStop, max);
		}else if (axis == 1){
			dJointSetHinge2Param(Link[l].joint_id, dParamLoStop2, min);
			dJointSetHinge2Param(Link[l].joint_id, dParamHiStop2, max);
		}
	}else if (Link[l].type == LinkTypeBall){
		if (axis == 0){
			dJointSetAMotorParam(Link[l].motor_id, dParamLoStop, min);
			dJointSetAMotorParam(Link[l].motor_id, dParamHiStop, max);
		}else if (axis == 1){
			dJointSetAMotorParam(Link[l].motor_id, dParamLoStop2, min);
			dJointSetAMotorParam(Link[l].motor_id, dParamHiStop2, max);
		}else if (axis == 2){
			dJointSetAMotorParam(Link[l].motor_id, dParamLoStop3, min);
			dJointSetAMotorParam(Link[l].motor_id, dParamHiStop3, max);
		}
	}else if (Link[l].type == LinkTypeSlider){
		dJointSetSliderParam(Link[l].joint_id, dParamLoStop, min);
		dJointSetSliderParam(Link[l].joint_id, dParamHiStop, max);
	}else if (Link[l].type == LinkTypeUniversal){
		if (axis == 0){
			dJointSetUniversalParam(Link[l].joint_id, dParamLoStop, min);
			dJointSetUniversalParam(Link[l].joint_id, dParamHiStop, max);
		}else if (axis == 1){
			dJointSetUniversalParam(Link[l].joint_id, dParamLoStop2, min);
			dJointSetUniversalParam(Link[l].joint_id, dParamHiStop2, max);
		}
	}
#endif
}

#if 0

int AddLinkSpring(Object *o1, Object *o2, const vector &rho1, const vector &rho2, float dx, float k)
{
	msg_db_r("AddLinkSpring", 2);
	sLink *l=AddLink(o1,o2,LinkTypeSpring);
	l->rho1=rho1;
	l->rho2=rho2;
	l->param_f1=dx;
	l->param_f2=k;
	msg_db_l(2);
	return Link.num-1;
}

// e12 is some vector orthogonal to ax1
int AddLinkHinge(Object *o1, Object *o2, const vector &rho1, const vector &rho2, const vector &ax1, const vector &ax2)
{
	msg_db_r("AddLinkHinge", 2);
	sLink *l = AddLink(o1, o2, LinkTypeHinge);
	l->k_s = 2000;
	l->k_d = 200;
	l->rho1 = rho1;
	l->rho2 = rho2;
	VecNormalize(l->d1, ax1);
	VecNormalize(l->d2, ax2);
	// create some vectors building an ONB with ax1
	l->d3 = get_orthogonal(ax1);
	l->d4 = ax1 ^ l->d3;
	VecNormalize(l->d4, l->d4);
#ifdef USE_ODE
	l->joint_id = dJointCreateHinge(world_id, 0);
	dJointAttach(l->joint_id, body_id[o1->ID], body_id[o2->ID]);
	vector p, a;
	VecTransform(p, *o1->Matrix, rho1);
	dJointSetHingeAnchor(l->joint_id, p.x, p.y, p.z);
	a = VecRotate(ax1, o1->Ang);
	dJointSetHingeAxis(l->joint_id, a.x, a.y, a.z);
#endif
	msg_db_l(2);
	return Link.num-1;
}


int AddLinkHingeAbs(Object *o1, Object *o2, const vector &p, const vector &ax)
{
	msg_db_r("AddLinkHingeAbs", 2);
	matrix im1,im2;
	MatrixInverse(im1,*o1->Matrix);
	MatrixInverse(im2,*o2->Matrix);
	vector rho1,rho2;
	VecTransform(rho1,im1,p);
	VecTransform(rho2,im2,p);
	vector ax1,ax2;
	VecNormalTransform(ax1,im1,ax);
	VecNormalTransform(ax2,im2,ax);
	msg_db_l(2);
	return AddLinkHinge(o1,o2,rho1,rho2,ax1,ax2);
}

int AddLinkBall(Object *o1, Object *o2, const vector &rho1, const vector &rho2)
{
	msg_db_r("AddLinkBall", 2);
	sLink *l=AddLink(o1,o2,LinkTypeBall);
	l->k_s = 2000;
	l->k_d = 200;
	l->rho1=rho1;
	l->rho2=rho2;
#ifdef USE_ODE
	l->joint_id = dJointCreateBall(world_id, 0);
	dJointAttach(l->joint_id, body_id[o1->ID], body_id[o2->ID]);
	vector p;
	VecTransform(p, *o1->Matrix, rho1);
	dJointSetBallAnchor(l->joint_id, p.x, p.y, p.z);
#endif
	msg_db_l(2);
	return Link.num-1;
}

int AddLinkBallAbs(Object *o1, Object *o2, const vector &p)
{
	msg_db_r("AddLinkBallAbs", 2);
	matrix im1,im2;
	MatrixInverse(im1,*o1->Matrix);
	MatrixInverse(im2,*o2->Matrix);
	vector rho1,rho2;
	VecTransform(rho1,im1,p);
	VecTransform(rho2,im2,p);
	msg_db_l(2);
	return AddLinkBall(o1,o2,rho1,rho2);
}

/*
 static
	k_s = 2000;
	k_d = 200;
 */

void LinkHingeSetTorque(int l, float t)
{
	if (Link[l].type==LinkTypeHinge){
		dJointAddHingeTorque(Link[l].joint_id, t);
		Link[l].param_f1=t;
	}
}

void LinkSetFriction(int l, float f)
{
	Link[l].friction=f;
}

void LinkHingeSetAxis(int l, const vector &ax1, const vector &ax2)
{
	if (Link[l].type==LinkTypeHinge){
		msg_db_r("LinkHingeSetAxis", 2);
		vector d3 = get_orthogonal(ax1);
		VecNormalize(Link[l].d1, ax1);
		VecNormalize(Link[l].d2, ax2);
		VecNormalize(Link[l].d3, d3);
		VecNormalize(Link[l].d4, ax1 ^ d3);
#ifdef USE_ODE
		vector a = VecRotate(ax1, Link[l].o1->Ang);
		dJointSetHingeAxis(Link[l].joint_id, a.x, a.y, a.z);
#endif		
		msg_db_l(2);
	}
}

#endif

// Ebene mit n durch cp relativ zu o1
void AddLinkContact(Object *o1, Object *o2, const vector &cp, const vector &n, float depth, const vector &dv, float c_static, float c_dynamic)
{
	msg_db_r("AddLinkContact", 2);
	sLink *l = AddLink(o1, o2, LinkTypeContact);
	l->c_fdt = 0;
	l->c_fdf = 0;
	l->k_s = 1000;
	l->k_d = 100;
	l->p = cp;
	matrix m1, m2, mi1, mi2;
	MatrixRotation(m1, o1->ang);
	MatrixTranspose(mi1, m1);
	l->rho1 = mi1 * (cp - o1->pos);
	MatrixRotation(m2, o2->ang);
	MatrixTranspose(mi2, m2);
	vector cp2 = cp - n * depth;
	l->rho2 = mi2 * (cp2 - o2->pos);
	l->d1 = n.transform_normal(mi1);

	l->dv = dv;
	l->param_f1 = c_static;
	l->param_f2 = c_dynamic;
	l->d4 = v_0;

	l->f_fric = v_0;
	msg_db_l(2);
}

static bool TestVectorSanity(vector &v, const char *name)
{
	if (inf_v(v)){
		v=v_0;
		msg_error(format("Vektor %s unendlich!!!!!!!",name));
		return true;
	}
	return false;
}

static bool TestMatrix3Sanity(matrix3 &m, const char *name)
{
	bool e=false;
	for (int k=0;k<9;k++)
		if (inf_f(m.e[k]))
			e=true;
	if (e){
		//m=v_0;
		msg_error(format("Matrix3 %s unendlich!!!!!!!",name));
		return true;
	}
	return false;
}


inline void DoLinkSpring(sLink *&l,Object *&o1,Object *&o2)
{
	// current coordinate frame
	vector rho1,rho2,p1,p2;
	rho1 = l->rho1.transform_normal(o1->_matrix);
	if (o2)
		rho2 = l->rho2.transform_normal(o2->_matrix);
	else
		rho2 = l->rho2;
	p1 = o1->pos + rho1;
	if (o2)
		p2 = o2->pos + rho2;
	else
		p2 = l->p;

	// evaluate forces		( = ( dist - dist_0 ) * k )
	float x=(p2-p1).length();
	vector dir;
	if (x==0)
		dir=e_x;
	else
		dir=(p2-p1)/x;
	float f=(x-l->param_f1)*l->param_f2;

	// apply forces
	ob_add_force(o1, dir*f, rho1);
	if (o2)
		ob_add_force(o2, - dir*f, rho1);
}

static matrix_n J,JT,W,JW,JWJ;
static vector_n lambda, Q, JWQ, rhs, Qc, C, dtC;
static vector L1,L2,wxL1,wxL2;

inline void DoLinkBall(sLink *&l,Object *&o1,Object *&o2,int nc)
{
	// some values
	vector r1,r2,dtr1,dtr2;
	r1 = l->rho1.transform_normal(o1->_matrix);
	dtr1 = VecCrossProduct( o1->rot, r1 );
	if (o2){
		r2 = l->rho2.transform_normal(o2->_matrix);
		dtr2 = VecCrossProduct( o2->rot, r2 );
	}

	// J
	J.nr=3;	J.nc=nc;
	J.e[0][0]= 1;	J.e[0][1]= 0;	J.e[0][2]= 0;	J.e[0][3]= 0;		J.e[0][4]= r1.z;	J.e[0][5]=-r1.y;
	J.e[1][0]= 0;	J.e[1][1]= 1;	J.e[1][2]= 0;	J.e[1][3]=-r1.z;	J.e[1][4]= 0;		J.e[1][5]= r1.x;
	J.e[2][0]= 0;	J.e[2][1]= 0;	J.e[2][2]= 1;	J.e[2][3]= r1.y;	J.e[2][4]=-r1.x;	J.e[2][5]= 0;
	if (o2){
		J.e[0][6]=-1;	J.e[0][7]= 0;	J.e[0][8]= 0;	J.e[0][9]= 0;		J.e[0][10]=-r2.z;	J.e[0][11]= r2.y;
		J.e[1][6]= 0;	J.e[1][7]=-1;	J.e[1][8]= 0;	J.e[1][9]= r2.z;	J.e[1][10]= 0;		J.e[1][11]=-r2.x;
		J.e[2][6]= 0;	J.e[2][7]= 0;	J.e[2][8]=-1;	J.e[2][9]=-r2.y;	J.e[2][10]= r2.x;	J.e[2][11]= 0;
	}

	// C		(constraint)
	C.n=3;
	if (o2){
		C.e[0]= o1->pos.x + r1.x - o2->pos.x - r2.x;
		C.e[1]= o1->pos.y + r1.y - o2->pos.y - r2.y;
		C.e[2]= o1->pos.z + r1.z - o2->pos.z - r2.z;
	}else{
		C.e[0]= o1->pos.x + r1.x - l->p.x;
		C.e[1]= o1->pos.y + r1.y - l->p.y;
		C.e[2]= o1->pos.z + r1.z - l->p.z;
	}

	// dtC
	dtC.n=3;
	dtC.e[0]= o1->vel.x + dtr1.x;
	dtC.e[1]= o1->vel.y + dtr1.y;
	dtC.e[2]= o1->vel.z + dtr1.z;
	if (o2){
		dtC.e[0]-= o2->vel.x + dtr2.x;
		dtC.e[1]-= o2->vel.y + dtr2.y;
		dtC.e[2]-= o2->vel.z + dtr2.z;
	}

	// friction
	if (l->friction != 0){
		// bad: should also use rho's to create forces!

		// relative rotation
		vector drot = o1->rot;
		if (o1)
			drot -= o2->rot;
		// friction
		vector t_fric = - drot * l->friction;
		// apply
		o1->torque_int += t_fric;
		if (o2)
			o2->torque_int -= t_fric;
	}
}

inline void DoLinkHinge(sLink *&l,Object *&o1,Object *&o2,int nc)
{
	// some values
	vector r1,r2,dtr1,dtr2,d1,d2,d3,d4,dtd1,dtd2,dtd3,dtd4;
	r1 = l->rho1.transform_normal(o1->_matrix);
	dtr1 = VecCrossProduct( o1->rot, r1 );
	d1 = l->d1.transform_normal(o1->_matrix);
	dtd1 = VecCrossProduct( o1->rot, d1 );
	if (o2){
		r2 = o2->_matrix.transform_normal(l->rho2);
		dtr2 = VecCrossProduct( o2->rot, r2 );
		d2 = o2->_matrix.transform_normal(l->d2);
		dtd2 = VecCrossProduct( o2->rot, d2 );
	}else{
		d2=l->d2;
		dtd2=v_0;
	}
	d3 = o1->_matrix.transform_normal(l->d3);
	dtd3 = VecCrossProduct( o1->rot, d3 );
	d4 = o1->_matrix.transform_normal(l->d4);
	dtd4 = VecCrossProduct( o1->rot, d4 );
	float d32=VecDotProduct(d3,d2);
	float d42=VecDotProduct(d4,d2);
	vector d3x2=VecCrossProduct(d3,d2);
	vector d4x2=VecCrossProduct(d4,d2);

	// J
	J.nr=5;	J.nc=nc;
	J.e[0][0]= 1;	J.e[0][1]= 0;	J.e[0][2]= 0;	J.e[0][3]= 0;		J.e[0][4]= r1.z;	J.e[0][5]=-r1.y;
	J.e[1][0]= 0;	J.e[1][1]= 1;	J.e[1][2]= 0;	J.e[1][3]=-r1.z;	J.e[1][4]= 0;		J.e[1][5]= r1.x;
	J.e[2][0]= 0;	J.e[2][1]= 0;	J.e[2][2]= 1;	J.e[2][3]= r1.y;	J.e[2][4]=-r1.x;	J.e[2][5]= 0;
	J.e[3][0]= 0;	J.e[3][1]= 0;	J.e[3][2]= 0;	J.e[3][3]= d3x2.x;	J.e[3][4]= d3x2.y;	J.e[3][5]= d3x2.z;
	J.e[4][0]= 0;	J.e[4][1]= 0;	J.e[4][2]= 0;	J.e[4][3]= d4x2.x;	J.e[4][4]= d4x2.y;	J.e[4][5]= d4x2.z;
	if (o2){
		J.e[0][6]=-1;	J.e[0][7]= 0;	J.e[0][8]= 0;	J.e[0][9]= 0;		J.e[0][10]=-r2.z;	J.e[0][11]= r2.y;
		J.e[1][6]= 0;	J.e[1][7]=-1;	J.e[1][8]= 0;	J.e[1][9]= r2.z;	J.e[1][10]= 0;		J.e[1][11]=-r2.x;
		J.e[2][6]= 0;	J.e[2][7]= 0;	J.e[2][8]=-1;	J.e[2][9]=-r2.y;	J.e[2][10]= r2.x;	J.e[2][11]= 0;
		J.e[3][6]= 0;	J.e[3][7]= 0;	J.e[3][8]= 0;	J.e[3][9]=-d3x2.x;	J.e[3][10]=-d3x2.y;	J.e[3][11]=-d3x2.z;
		J.e[4][6]= 0;	J.e[4][7]= 0;	J.e[4][8]= 0;	J.e[4][9]=-d4x2.x;	J.e[4][10]=-d4x2.y;	J.e[4][11]=-d4x2.z;
	}

	// C		(constraint)
	C.n=5;
	if (o2){
		C.e[0]= o1->pos.x + r1.x - o2->pos.x - r2.x;
		C.e[1]= o1->pos.y + r1.y - o2->pos.y - r2.y;
		C.e[2]= o1->pos.z + r1.z - o2->pos.z - r2.z;
		C.e[3]= d32;
		C.e[4]= d42;
	}else{
		C.e[0]= o1->pos.x + r1.x - l->p.x;
		C.e[1]= o1->pos.y + r1.y - l->p.y;
		C.e[2]= o1->pos.z + r1.z - l->p.z;
		C.e[3]= d32;
		C.e[4]= d42;
	}

	// dtC
	dtC.n=5;
	dtC.e[0]= o1->vel.x + dtr1.x;
	dtC.e[1]= o1->vel.y + dtr1.y;
	dtC.e[2]= o1->vel.z + dtr1.z;
	dtC.e[3]= VecDotProduct(dtd3,d2)+VecDotProduct(d3,dtd2);
	dtC.e[4]= VecDotProduct(dtd4,d2)+VecDotProduct(d4,dtd2);
	if (o2){
		dtC.e[0]-= o2->vel.x + dtr2.x;
		dtC.e[1]-= o2->vel.y + dtr2.y;
		dtC.e[2]-= o2->vel.z + dtr2.z;
	}

	// motor
	o1->torque_int += d1*l->param_f1;
	if (o2)
		o2->torque_int -= d1*l->param_f1;
	//msg_write(string2("          Hinge - Torque = %f",l->param_f1));

	// friction
	if (l->friction!=0){
		// bad: should also use rho's to create forces!

		// relative rotation
		vector drot=o1->rot;
		if (o1)
			drot-=o2->rot;
		// project onto axis
		vector t_fric=-VecDotProduct(drot,d1)*d1*l->friction;
		// apply
		o1->torque_int += t_fric;
		if (o2)
			o2->torque_int -= t_fric;
	}
}

inline void DoLinkStatic(sLink *&l,Object *&o1,Object *&o2,int nc)
{
	// some values
	vector r1,r2,dtr1,dtr2,d1,d2,d3,d4,dtd1,dtd2,dtd3,dtd4;
	r1 = o1->_matrix.transform_normal(l->rho1);
	dtr1 = VecCrossProduct( o1->rot, r1 );
	d1 = o1->_matrix.transform_normal(l->d1);
	dtd1 = VecCrossProduct( o1->rot, d1 );
	d3 = o1->_matrix.transform_normal(l->d3);
	dtd3 = VecCrossProduct( o1->rot, d3 );
	if (o2){
		r2 = o2->_matrix.transform_normal(l->rho2);
		dtr2 = VecCrossProduct( o2->rot, r2 );
		d2 = o2->_matrix.transform_normal(l->d2);
		dtd2 = VecCrossProduct( o2->rot, d2 );
		d4 = o2->_matrix.transform_normal(l->d4);
		dtd4 = VecCrossProduct( o2->rot, d4 );
	}else{
		d2=l->d2;
		dtd2=v_0;
		d4=l->d4;
		dtd4=v_0;
	}
	float d14=VecDotProduct(d1,d4);
	float d34=VecDotProduct(d3,d4);
	float d32=VecDotProduct(d3,d2);
	vector d1x4=VecCrossProduct(d1,d4);
	vector d3x4=VecCrossProduct(d3,d4);
	vector d3x2=VecCrossProduct(d3,d2);

	// J
	J.nr=6;	J.nc=nc;
	J.e[0][0]= 1;	J.e[0][1]= 0;	J.e[0][2]= 0;	J.e[0][3]= 0;		J.e[0][4]= r1.z;	J.e[0][5]=-r1.y;
	J.e[1][0]= 0;	J.e[1][1]= 1;	J.e[1][2]= 0;	J.e[1][3]=-r1.z;	J.e[1][4]= 0;		J.e[1][5]= r1.x;
	J.e[2][0]= 0;	J.e[2][1]= 0;	J.e[2][2]= 1;	J.e[2][3]= r1.y;	J.e[2][4]=-r1.x;	J.e[2][5]= 0;
	J.e[3][0]= 0;	J.e[3][1]= 0;	J.e[3][2]= 0;	J.e[3][3]= d1x4.x;	J.e[3][4]= d1x4.y;	J.e[3][5]= d1x4.z;
	J.e[4][0]= 0;	J.e[4][1]= 0;	J.e[4][2]= 0;	J.e[4][3]= d3x4.x;	J.e[4][4]= d3x4.y;	J.e[4][5]= d3x4.z;
	J.e[5][0]= 0;	J.e[5][1]= 0;	J.e[5][2]= 0;	J.e[5][3]= d3x2.x;	J.e[5][4]= d3x2.y;	J.e[5][5]= d3x2.z;
	if (o2){
		J.e[0][6]=-1;	J.e[0][7]= 0;	J.e[0][8]= 0;	J.e[0][9]= 0;		J.e[0][10]=-r2.z;	J.e[0][11]= r2.y;
		J.e[1][6]= 0;	J.e[1][7]=-1;	J.e[1][8]= 0;	J.e[1][9]= r2.z;	J.e[1][10]= 0;		J.e[1][11]=-r2.x;
		J.e[2][6]= 0;	J.e[2][7]= 0;	J.e[2][8]=-1;	J.e[2][9]=-r2.y;	J.e[2][10]= r2.x;	J.e[2][11]= 0;
		J.e[3][6]= 0;	J.e[3][7]= 0;	J.e[3][8]= 0;	J.e[3][9]=-d1x4.x;	J.e[3][10]=-d1x4.y;	J.e[3][11]=-d1x4.z;
		J.e[4][6]= 0;	J.e[4][7]= 0;	J.e[4][8]= 0;	J.e[4][9]=-d3x4.x;	J.e[4][10]=-d3x4.y;	J.e[4][11]=-d3x4.z;
		J.e[5][6]= 0;	J.e[5][7]= 0;	J.e[5][8]= 0;	J.e[5][9]=-d3x2.x;	J.e[5][10]=-d3x2.y;	J.e[5][11]=-d3x2.z;
	}

	// C		(constraint)
	C.n=6;
	if (o2){
		C.e[0]= o1->pos.x + r1.x - o2->pos.x - r2.x;
		C.e[1]= o1->pos.y + r1.y - o2->pos.y - r2.y;
		C.e[2]= o1->pos.z + r1.z - o2->pos.z - r2.z;
		C.e[3]= d14;
		C.e[4]= d34;
		C.e[5]= d32;
	}else{
		C.e[0]= o1->pos.x + r1.x - l->p.x;
		C.e[1]= o1->pos.y + r1.y - l->p.y;
		C.e[2]= o1->pos.z + r1.z - l->p.z;
		C.e[3]= d14;
		C.e[4]= d34;
		C.e[5]= d32;
	}

	// dtC
	dtC.n=6;
	dtC.e[0]= o1->vel.x + dtr1.x;
	dtC.e[1]= o1->vel.y + dtr1.y;
	dtC.e[2]= o1->vel.z + dtr1.z;
	dtC.e[3]= VecDotProduct(dtd1,d4)+VecDotProduct(d1,dtd4);
	dtC.e[4]= VecDotProduct(dtd3,d4)+VecDotProduct(d3,dtd4);
	dtC.e[5]= VecDotProduct(dtd3,d2)+VecDotProduct(d3,dtd2);
	if (o2){
		dtC.e[0]-= o2->vel.x + dtr2.x;
		dtC.e[1]-= o2->vel.y + dtr2.y;
		dtC.e[2]-= o2->vel.z + dtr2.z;
	}
}

inline void DoLinkContact(sLink *&l,Object *&o1,Object *&o2,int &nc, matrix_n &dtJ)
{
	vector n,dtn,r1,r2,dtr1,dtr2;
	/*msg_write(v2s(l->d1));
	msg_write(v2s(n));*/
	n = o1->_matrix.transform_normal(l->d1);
	l->n = n;
	dtn = VecCrossProduct( o1->rot, n );
	r1 = o1->_matrix.transform_normal(l->rho1);
	dtr1 = VecCrossProduct( o1->rot, r1 );
	if (o2){
		r2 = o2->_matrix.transform_normal(l->rho2);
		dtr2 = VecCrossProduct( o2->rot, r2 );
	}else{
		r2 = l->rho2;
		dtr2 = v_0;
	}
	vector nxr1=VecCrossProduct(n,r1);
	vector nxr2=VecCrossProduct(n,r2);

	J.nr=1;	J.nc=nc;
	MatrixZeroN(J,1,nc);
	J.e[0][0]= n.x;
	J.e[0][1]= n.y;
	J.e[0][2]= n.z;
	J.e[0][3]=-nxr1.x;
	J.e[0][4]=-nxr1.y;
	J.e[0][5]=-nxr1.z;
	if (o2){
		J.e[0][ 6]=-n.x;
		J.e[0][ 7]=-n.y;
		J.e[0][ 8]=-n.z;
		J.e[0][ 9]= nxr2.x;
		J.e[0][10]= nxr2.y;
		J.e[0][11]= nxr2.z;
	}

	C.n=1;
	if (o2)
		C.e[0]= VecDotProduct( n, o1->pos + r1 - o2->pos - r2 );
	else
		C.e[0]= VecDotProduct( n, o1->pos + r1 );
	//msg_write(f2s(VecDotProduct( n, o1->pos),2));
	//msg_write(f2s(VecDotProduct( n, o1->pos + r1 - o2->pos - r2 ),2));
	//msg_write(f2s(C.e[0],2));

	dtC.n=1;
	if (o2)
		dtC.e[0]= VecDotProduct( n, o1->vel - o2->vel - dtr2 ) + VecDotProduct( dtn, o1->pos - o2->pos - r2 );
	else
		dtC.e[0]= VecDotProduct( n, o1->vel ) + VecDotProduct( dtn, o1->pos );

	vector va=-VecCrossProduct(dtn,r2)-VecCrossProduct(n,dtr2);
	MatrixZeroN( dtJ, 1, nc );
	dtJ.e[0][ 3]=-va.x;	dtJ.e[0][ 4]=-va.y;	dtJ.e[0][ 5]=-va.z;
	dtJ.e[0][ 9]= va.x;	dtJ.e[0][10]= va.y;	dtJ.e[0][11]= va.z;
}


		/*if ( l->type == LinkTypeFixPoint1Rot ){

			vector rho,dtrho;
			VecNormalTransform( rho, o1->Matrix, l->rho1 );
			//rho=vector(0,0,0);
			dtrho = VecCrossProduct( o1->Rot, rho );
			//dtrho=vector(0,0,0);

			C.n=5;
			C.e[0]= l->p.x - o1->pos.x;
			C.e[1]= l->p.y - o1->pos.y;
			C.e[2]= l->p.z - o1->pos.z;
			C.e[3]= rho.y;
			C.e[4]= rho.z;

			dtC.n=5;
			dtC.e[0]= - o1->vel.x;
			dtC.e[1]= - o1->vel.y;
			dtC.e[2]= - o1->vel.z;
			dtC.e[3]= + dtrho.y;
			dtC.e[4]= + dtrho.z;

			J.nr=5;	J.nc=6;
			J.e[0][0]=-1;	J.e[0][1]=0;	J.e[0][2]=0;	J.e[0][3]=0;		J.e[0][4]=0;		J.e[0][5]=0;
			J.e[1][0]=0;	J.e[1][1]=-1;	J.e[1][2]=0;	J.e[1][3]=0;		J.e[1][4]=0;		J.e[1][5]=0;
			J.e[2][0]=0;	J.e[2][1]=0;	J.e[2][2]=-1;	J.e[2][3]=0;		J.e[2][4]=0;		J.e[2][5]=0;
			J.e[3][0]=0;	J.e[3][1]=0;	J.e[3][2]=0;	J.e[3][3]=-rho.z;	J.e[3][4]=0;		J.e[3][5]= rho.x;
			J.e[4][0]=0;	J.e[4][1]=0;	J.e[4][2]=0;	J.e[4][3]= rho.y;	J.e[4][4]=-rho.x;	J.e[4][5]=0;
		}

		if (l->type==LinkTypeFixPointSphere){

			vector dp = l->p - o1->pos;

			C.n=1;
			C.e[0]=VecDotProduct( dp, dp ) - l->param_f1;

			dtC.n=1;
			dtC.e[0]= -2 * VecDotProduct( dp, o1->vel );

			J.nr=1;	J.nc=6;
			J.e[0][0]=-2*dp.x;	J.e[0][1]=-2*dp.y;	J.e[0][2]=-2*dp.z;	J.e[0][3]=0;		J.e[0][4]=0;		J.e[0][5]=0;
		}*/

inline void HandleDtJdtq(Object *o1, Object *o2, int nc, matrix_n &dtJ, vector_n &rhs)
{
	vector_n dtq, dtJdtq;
	dtq.n = nc;
	matnout(dtJ,"dJ/dt");
	dtq.e[0]=o1->vel.x;
	dtq.e[1]=o1->vel.y;
	dtq.e[2]=o1->vel.z;
	dtq.e[3]=o1->rot.x;
	dtq.e[4]=o1->rot.y;
	dtq.e[5]=o1->rot.z;
	if (o2){
		dtq.e[6]=o2->vel.x;
		dtq.e[7]=o2->vel.y;
		dtq.e[8]=o2->vel.z;
		dtq.e[9]=o2->rot.x;
		dtq.e[10]=o2->rot.y;
		dtq.e[11]=o2->rot.z;
	}
	vecnout(dtq,"dq/dt");

	MatrixVectorMultiplyN( dtJdtq, dtJ, dtq );
	vecnout( dtJdtq, "dJ/dt dq/dt" );
	for (int k=0;k<J.nr;k++)
		rhs.e[k] -= dtJdtq.e[k];
}

inline bool all_frozen(Object *o1, Object *o2)
{		
	if (o1->frozen){
		if (o2){
			return o2->frozen;
		}else
			return true;
	}
	return false;
}

void DoAllSpings()
{
	for (int i=0;i<Link.num;i++){
		sLink *l = &Link[i];
		if (l->type == LinkTypeSpring){
			Object *o1 = l->o1;
			Object *o2 = l->o2;
		
			//if (all_frozen(o1, o2))
			//	continue;

			DoLinkSpring(l, o1, o2);
		}

	}
}

void DoLinksStep()
{
	for (int i=0;i<Link.num;i++){
//		msg_write(string2("-------------------------------------- Link[%d] ---------------",i));
		sLink *l = &Link[i];
		//l->o2=-1;
		Object *o1 = l->o1;
		Object *o2 = l->o2;

		if (all_frozen(o1, o2))
			continue;

	//	if ( l->type == LinkTypeNone )
	//		continue;
		
		// springs are a bit different to handle
		if ( l->type == LinkTypeSpring )
			continue;

		
	// prelude...
		int nc = o2 ? 12 : 6;
		matrix_n dtJ;
		bool use_dtJdtq=false;
		// 1/m
		float im1 = o1->active_physics ? ( 1.0f / o1->mass ) : 0;
		float im2 = o2 ? ( o2->active_physics ? ( 1.0f / o2->mass ) : 0 ) : 0;

		// w x L
		vector L1,L2,wxL1,wxL2;
		TestVectorSanity(o1->rot,"o1->rot");
		TestMatrix3Sanity(o1->theta, "o1->theta");
		L1 = o1->theta * o1->rot;
		TestVectorSanity(L1,"L1");
		wxL1 = VecCrossProduct( o1->rot, L1 );
		TestVectorSanity(wxL1,"wxL1  1");
		if (o2){
			TestVectorSanity(o2->rot,"o2->rot");
			// debug!!!
			TestMatrix3Sanity(o2->theta, "o2->theta");
			/*msg_write(string2("o2.rot   %f  %f  %f",o2->rot.x, o2->rot.y, o2->rot.z));
			msg_write(string2("theta  %f  %f  %f",o2->theta.e[0], o2->theta.e[1], o2->theta.e[2]));
			msg_write(string2("theta  %f  %f  %f",o2->theta.e[3], o2->theta.e[4], o2->theta.e[5]));
			msg_write(string2("theta  %f  %f  %f",o2->theta.e[6], o2->theta.e[7], o2->theta.e[8]));*/
			L2 = o2->theta * o2->rot;
			//msg_write(string2("L2   %f  %f  %f",L2.x, L2.y, L2.z));
			TestVectorSanity(L2,"L2");
			wxL2 = VecCrossProduct( o2->rot, L2 );
			//msg_write(string2("wxL2   %f  %f  %f",wxL2.x, wxL2.y, wxL2.z));
			TestVectorSanity(wxL2,"wxL2  1");
		}

		// W		(1/inertia)
		MatrixZeroN( W, nc, nc );
		W.e[ 0][ 0]=im1;	W.e[ 1][ 1]=im1;	W.e[ 2][ 2]=im1;
		W.e[ 3][ 3]=o1->theta_inv._00;	W.e[ 3][ 4]=o1->theta_inv._01;	W.e[ 3][ 5]=o1->theta_inv._02;
		W.e[ 4][ 3]=o1->theta_inv._10;	W.e[ 4][ 4]=o1->theta_inv._11;	W.e[ 4][ 5]=o1->theta_inv._12;
		W.e[ 5][ 3]=o1->theta_inv._20;	W.e[ 5][ 4]=o1->theta_inv._21;	W.e[ 5][ 5]=o1->theta_inv._22;
		if (o2){
			W.e[ 6][ 6]=im2;	W.e[ 7][ 7]=im2;	W.e[ 8][ 8]=im2;
			W.e[ 9][ 9]=o2->theta_inv._00;	W.e[ 9][10]=o2->theta_inv._01;	W.e[ 9][11]=o2->theta_inv._02;
			W.e[10][ 9]=o2->theta_inv._10;	W.e[10][10]=o2->theta_inv._11;	W.e[10][11]=o2->theta_inv._12;
			W.e[11][ 9]=o2->theta_inv._20;	W.e[11][10]=o2->theta_inv._21;	W.e[11][11]=o2->theta_inv._22;
		}
		matnout(W,"W");

		// Q		(external forces  ...and link forces from the last step...)
		Q.n = nc;
		Q.e[ 0] = o1->force_int.x;
		Q.e[ 1] = o1->force_int.y;
		Q.e[ 2] = o1->force_int.z;
		Q.e[ 3] = o1->torque_int.x - wxL1.x;
		Q.e[ 4] = o1->torque_int.y - wxL1.y;
		Q.e[ 5] = o1->torque_int.z - wxL1.z;
		//printf("f:  (%f %f %f)       (%f %f %f)\n", o1->force_int.x, o1->force_int.y, o1->force_int.z, o2->force_int.x, o2->force_int.y, o2->force_int.z);
		//printf("fx: (%f %f %f)       (%f %f %f)\n", o1->ForceExt.x, o1->ForceExt.y, o1->ForceExt.z, o2->ForceExt.x, o2->ForceExt.y, o2->ForceExt.z);
		if (o2){
			Q.e[ 6] = o2->force_int.x;
			Q.e[ 7] = o2->force_int.y;
			Q.e[ 8] = o2->force_int.z;
			Q.e[ 9] = o2->torque_int.x - wxL2.x;
			Q.e[10] = o2->torque_int.y - wxL2.y;
			Q.e[11] = o2->torque_int.z - wxL2.z;
		}
		vecnout(Q,"Q");


	// link specific matrices
		if ( l->type == LinkTypeBall )
			DoLinkBall(l,o1,o2,nc);

		else if ( l->type == LinkTypeHinge )
			DoLinkHinge(l,o1,o2,nc);

		else if ( l->type == LinkTypeStatic )
			DoLinkStatic(l,o1,o2,nc);

		else if ( l->type == LinkTypeContact ){
			//printf("Kontakt....\n");
			DoLinkContact(l, o1, o2, nc, dtJ);
			use_dtJdtq=true;
		}else
			continue;

		
	// the actual calculations
		matnout(J,"J");
		vecnout(C,"C");
		vecnout(dtC,"dC/dt");
		
		// some more matrices needed
		MatrixTransposeN(JT,J);
		matnout(JT,"J^T");
		MatrixMultiplyN(JW,J,W);
		matnout(JW,"JW");
		MatrixMultiplyN(JWJ,JW,JT);
		matnout(JWJ,"JWJ");
		MatrixVectorMultiplyN(JWQ,JW,Q);
		vecnout(JWQ,"JWQ");

		// - J,t * q,t - J * W * Q - k_x * C - k_d * C,t		(",t" meaning d/dt)
		rhs.n=J.nr;
		for (int k=0;k<J.nr;k++){
			rhs.e[k] = - JWQ.e[k] - l->k_s * C.e[k] - l->k_d * dtC.e[k];
			//rhs.e[k] = - JWQ.e[k];
			//rhs.e[k] = - JWQ.e[k] - k_d * dtC.e[k];
			//rhs.e[k] = - JWQ.e[k] - k_s * C.e[k];
		}
		//DebugC=C;
		//DebugdtC=dtC;

		if (use_dtJdtq)
			HandleDtJdtq(o1, o2, nc, dtJ, rhs);

#ifdef _X_ALLOW_PHYSICS_DEBUG_
		DebugC=C;
		DebugdtC=dtC;
#endif

		//for (int k=0;k<JWJ.nc;k++)
		//	JWJ.e[k][k]+=0.1f; // cfm...

		// solve!!!
		// J * W * (J^T) * lambda =  - J,t * q,t - J * W * Q - k_x * C - k_d * C,t		(comma meaning d/dt)
		// (solving for lambda)
		vecnout( rhs, "rhs" );
		matnout( JWJ, "JWJ" );
		MatrixVectorSolveN( JWJ, rhs, lambda );
		vecnout( lambda, "lambda" );

		vector force1,torque1,force2,torque2;

/*		// TODO ....test
		float fff = 0;//.2f;
		for (int i=0;i<l->lambda.n;i++)
			lambda.e[i] = lambda.e[i]*(1 - fff) + l->lambda.e[i]*fff;
		l->lambda = lambda;*/

		// evaluate the forces
		MatrixVectorMultiplyN( Qc, JT, lambda );
		vecnout(Qc,"Q_c");



		
		l->f1 = vector( Qc.e[0], Qc.e[1], Qc.e[2] );
		l->t1 = vector( Qc.e[3], Qc.e[4], Qc.e[5] );
		l->sf1 += l->f1;
		l->st1 += l->t1;
		if (o2){
			l->f2 = vector( Qc.e[6], Qc.e[7], Qc.e[8] );
			l->t2 = vector( Qc.e[9], Qc.e[10], Qc.e[11] );
			l->sf2 += l->f2;
			l->st2 += l->t2;
		}

		

/*		// apply forces
		vector fdt=o1->rot; // friction
		vector fdf=o1->vel;
		if (o2){
			fdt-=o2->rot;
			fdf-=o2->vel;
		}
		fdt*=l->c_fdt;
		fdf*=l->c_fdf;
		force1 = vector( Qc.e[0], Qc.e[1], Qc.e[2] ) - fdf;
		torque1 = vector( Qc.e[3], Qc.e[4], Qc.e[5] ) - fdt;
		if (l->type==LinkTypeContact){
			if (force1 * l->n >= 0){
				//printf("Kontakt:   nicht noetig!\n");
//				continue;
			}
		}
#ifdef APPLY_FORCES_LATER
		l->f1 = force1;
		l->t1 = torque1;
#else
		o1->force_int += force1;
		o1->torque_int += torque1;
#endif
		if (o2){
			force2 = vector( Qc.e[6], Qc.e[7], Qc.e[8] ) + fdf;
			torque2 = vector( Qc.e[9], Qc.e[10], Qc.e[11] ) + fdt;
#ifdef APPLY_FORCES_LATER
			l->f2 = force2;
			l->t2 = torque2;
#else
			o2->force_int += force2;
			o2->torque_int += torque2;
#endif
		}
		if (l->type==LinkTypeContact)
			l->d4 += force1;*/
		//printf("erg: (%f %f %f)  (%f %f %f)\n(%f %f %f)  (%f %f %f)\n", force1.x, force1.y, force1.z, torque1.x, torque1.y, torque1.z, force2.x, force2.y, force2.z, torque2.x, torque2.y, torque2.z);
	}

	
	
#ifdef APPLY_FORCES_LATER
	for (int i=0;i<Link.num;i++){
		sLink *l = &Link[i];
		Object *o1 = l->o1;
		Object *o2 = l->o2;
		o1->force_int += l->f1;
		o1->torque_int += l->t1;
		if (o2){
			o2->force_int += l->f2;
			o2->torque_int += l->t2;
		}
	}
#endif

}

// simulate constraints (such as hinges etc)
//   for detailed information visit:
//   http://wiki.michi.is-a-geek.org/index.php?cmd=show&article=Spielephysik_mit_Zwangsbedingungen
void DoLinks(int steps)
{
	if (Engine.Elapsed <= 0)
		return;
#ifdef USE_ODE
	for (int i=0;i<Link.num;i++){
		if (Link[i].type == LinkTypeHinge){
			float drot = dJointGetHingeAngleRate(Link[i].joint_id);
			dJointAddHingeTorque(Link[i].joint_id, Link[i].param_f1 - drot * Link[i].friction);
		}else if (Link[i].type == LinkTypeHinge2){
			float drot = dJointGetHinge2Angle2Rate(Link[i].joint_id);
			dJointAddHinge2Torques(Link[i].joint_id, 0, Link[i].param_f1 - drot * Link[i].friction);
		}else if (Link[i].type == LinkTypeBall){
			// ...ODE INTERNAL ERROR: not yet implemented
			/*float x = dJointGetAMotorAngleRate(Link[i].motor_id, 0);
			float y = dJointGetAMotorAngleRate(Link[i].motor_id, 1);
			float z = dJointGetAMotorAngleRate(Link[i].motor_id, 2);*/
			vector a, v;
			a.x = dJointGetAMotorAngle(Link[i].motor_id, 0);
			a.y = dJointGetAMotorAngle(Link[i].motor_id, 1);
			a.z = dJointGetAMotorAngle(Link[i].motor_id, 2);
			v = (a - Link[i].d4) / Engine.Elapsed;
			dJointAddAMotorTorques(Link[i].motor_id,	Link[i].param_f1 - v.x * Link[i].friction,
			    										Link[i].param_f2 - v.y * Link[i].friction,
														                 - v.z * Link[i].friction);
			Link[i].d4 = a;
		}
	}
	DoAllSpings();
	
#ifdef _X_ALLOW_PHYSICS_DEBUG_
	PhysicsTimeLinks += HuiGetTime(PhysicsTimer);
#endif
	
	return;
#endif
	//msg_write("Links");
	// pseudo friction for numerical stability
	msg_db_r("DoLinks", 1);


	// reset link forces
	for (int i=0;i<Link.num;i++)
		Link[i].sf1 = Link[i].st1 = Link[i].sf2 = Link[i].st2 = v_0;

	printf("%d links\n", Link.num);
	
	// springs are a bit different to handle
	DoAllSpings();


	// solve iteratively
	for (int uuu=0;uuu<steps;uuu++)
		DoLinksStep();


	// force allowed?
	for (int i=0;i<Link.num;i++){
		sLink *l = &Link[i];
		if (l->type==LinkTypeContact){
			if (l->sf1 * l->n >= 0){
				//printf("   -> ignore\n");
				//msg_error("ignore");
				l->o1->force_int -= l->sf1;
				l->o1->torque_int -= l->st1;
				if (l->o2){
					l->o2->force_int -= l->sf2;
					l->o2->torque_int -= l->st2;
				}
			}
		}
	}

	
#ifdef _X_ALLOW_PHYSICS_DEBUG_
	Object *o = NULL;
	while(NextObject(&o)){
		if (o->ID >= MaxObjects)
			break;
		CForce[o->ID] = o->force_int - o->ForceExt;
		CTorque[o->ID] = o->torque_int - o->TorqueExt;
	}
#endif

#ifdef _X_ALLOW_PHYSICS_DEBUG_
	PhysicsTimeLinks += HuiGetTime(PhysicsTimer);
#endif
	
	msg_db_l(1);
}

void LinkRemoveContacts()
{
	for (int i=0;i<Link.num;i++)
		if (Link[i].type == LinkTypeContact){
			Link.erase(i);
			i --;
		}
}

void LinkCalcContactFriction()
{
	for (int i=0;i<Link.num;i++)
		if (Link[i].type==LinkTypeContact){
			sLink *l = &Link[i];
			Object *o1 = l->o1;
			Object *o2 = l->o2;
			vector f = l->sf1; // force on contact
			float f_n = fabs(l->n * f); // normal force
			float dv_orth = l->dv * l->n;
			vector dv_tang = l->dv - dv_orth * l->n;
			printf("f_n %f\n", f_n);

			float dv_max = 2;
			
			if (_vec_length_fuzzy_(dv_tang) > dv_max){
				// dynamic
				float f_max = l->param_f1 * f_n; // rc_static
				vector dir = dv_tang;
				_vec_normalize_(dir);
				l->f_fric = - dir * f_max;
				printf("dyn ");
			}else{
				// static
				float f_max = l->param_f2 * f_n; // rc_dynamic
				
				if (_vec_length_fuzzy_(dv_tang) > 0.01f){
					float x = 1.0f/((1.0f/o1->mass) + (o2->active_physics?(1.0f/o2->mass):0)) /*/ Elapsed * 0.001f*/ * 0.01f;
					l->f_fric = - dv_tang * x + l->n * (f * l->n) - f;
					printf("dv... %f  %f", x, dv_tang.length() * x);
				}else{
					l->f_fric = l->n * (f * l->n) - f;
				}
				float ll = _vec_length_(l->f_fric);
				if (ll > f_max)
					l->f_fric *= f_max / ll;
				printf("stat (%f  %f)  ", ll, f_max);
			}
			printf("%f  %f  %f\n", l->f_fric.x, l->f_fric.y, l->f_fric.z);
		}
//	return;
	for (int i=0;i<Link.num;i++)
		if (Link[i].type==LinkTypeContact){
			sLink *l = &Link[i];
			Object *o1 = l->o1;
			Object *o2 = l->o2;
			vector r1, r2;
			r1 = o1->_matrix.transform_normal(l->rho1);
			r2 = o2->_matrix.transform_normal(l->rho2);
			//l->f_fric *= 10;
			o1->force_int += l->f_fric;
			o1->torque_int += r1 ^ l->f_fric;
			o2->force_int -= l->f_fric;
			o2->torque_int -= r2 ^ l->f_fric;
#if 0
			CForce[o1->ID] += l->f_fric;
			CTorque[o1->ID] += r1 ^ l->f_fric;
			CForce[o2->ID] -= l->f_fric;
			CTorque[o2->ID] -= r1 ^ l->f_fric;
#endif
		}
}

inline void link_add_to_list(Array<Object*> &list, Object *o)
{
	for (int i=0;i<list.num;i++)
		if (list[i] == o)
			return;
	list.add(o);
}

void GodGetLinkedList(Object *o, Array<Object*> &list)
{
	list.clear();
	if (!o)
		return;
	for (int i=0;i<Link.num;i++){
		if ((Link[i].o1==o)&&(Link[i].o2))
			link_add_to_list(list,Link[i].o2);
		if ((Link[i].o2==o)&&(Link[i].o1))
			link_add_to_list(list,Link[i].o1);
	}
}
