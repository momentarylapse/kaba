/*----------------------------------------------------------------------------*\
| Object                                                                       |
| -> physical entities of a model in the game                                  |
| -> manages physics on its own                                                |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2008.10.26 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "x.h"

#ifdef object
#undef object
#endif



static int num_insane=0;

inline bool ainf_v(vector &v)
{
	if (inf_v(v))
		return true;
	return (v.length_fuzzy() > 100000000000.0f);
}

inline bool TestVectorSanity(vector &v,char *name)
{
	if (ainf_v(v)){
		num_insane++;
		v=v_0;
		if (num_insane>100)
			return false;
		msg_error(format("Vektor %s unendlich!!!!!!!",name));
		return true;
	}
	return false;
}



#define _realistic_calculation_


#define VelThreshold			1.0f
#define AccThreshold			10.0f
#define MaxTimeTillFreeze		1.0f

#define unfreeze(object)	(object)->time_till_freeze = MaxTimeTillFreeze; (object)->frozen = false



#if 0


// Standart-Objekt
Object::Object(const char *filename, const char *name, const vector &pos)
{
	msg_db_r("Object", 1);
	msg_db_m(filename, 2);
	strcpy(Name, name);
	Pos = pos;
	Theta = Theta0
	Matrix3Identity( ThetaInv );

	// load data from template
/*	sObjectTemplate *o = MetaLoadObjectTemplate(filename);
	Template = o;
	if (!o){
		msg_db_l(1);
		return;
	}*/
	

	model = MetaLoadModel(filename);
	Radius = model->Radius;
	Mass = model->Mass;
	ActivePhysics = model->ActivePhysics;
	PassivePhysics = model->PassivePhysics;
	MassInv = ActivePhysics ? (1.0f / model->Mass) : 0;
//	MaxLife = Life = model->MaxLife;
/*	if (strlen(model->ItemFilename) > 0)
		ItemFilename = model->ItemFilename;*/
	Inventary_ma = model->Inventary_ma;
	ScriptVar_ma = model->ScriptVar_ma;
	
	if (!ActivePhysics){
		Mass = 100000000.0f;
		TimeTillFreeze = -1;
	}

	//model->object = this;
	Visible = true;
	Rotating = true;
	TimeTillFreeze = ActivePhysics ? MaxTimeTillFreeze : -1;
	frozen = !ActivePhysics;
	GFactor = 1;
	Vel = Rot = ang = v_0;
	ForceExt = TorqueExt = ForceInt = TorqueInt = v_0;
	OnGround = false;
	//SetMaterial(&model->Material[0], SetMaterialFriction);
	//UpdateMatrix();
	//UpdateTheta();
	if (ShadowLevel > 1)
		AllowShadow = ActivePhysics;
	msg_db_l(1);
}

static matrix _terrain_matrix_;

// neutral object (for terrains,...)
Object::Object(Model *mod)
{
	//msg_right();
	//msg_write("terrain object");
	memset(this,0,sizeof(Object));
	strcpy(Name,"-terrain-");
	model=mod;
	Visible=false;
	Mass = 10000.0f;
	MassInv = 0;
	Radius = 30000000;
	Matrix3Identity( Theta );
	//Matrix3Identity( ThetaInv );
	memset(&ThetaInv, 0, sizeof(ThetaInv));
	Matrix = &_terrain_matrix_;
	//Theta=Theta*10000000.0f;
	GFactor=1;
	Vel=Rot=ang=v_0;
	ForceExt=TorqueExt=ForceInt=TorqueInt=v_0;
	ActivePhysics=false;
	PassivePhysics=true;
	Rotating=false;
	frozen = true;
	TimeTillFreeze = -1;
	//SpecialID=IDFloor;
	TimeTillFreeze=-1;
	frozen=true;
	ReCalcMatrix();
	UpdateTheta();
	//msg_left();
}

Object::~Object()
{
	msg_db_r("~Object",1);
	Inventary_ma.clear();
	ScriptVar_ma.clear();
	if (model)
		MetaDeleteModel(model);
	msg_db_l(1);
}
#endif

// neutral object (for terrains,...)
Object::Object()
{
	//msg_right();
	//msg_write("terrain object");
	reset();
	name = "-terrain-";
	visible = false;
	mass = 10000.0f;
	mass_inv = 0;
	radius = 30000000;
	Matrix3Identity( theta_0 );
	Matrix3Identity( theta );
	memset(&theta_inv, 0, sizeof(theta_inv));
	MatrixIdentity(_matrix);
	//theta = theta * 10000000.0f;
	g_factor = 1;
	active_physics = false;
	passive_physics = true;
	rotating = false;
	frozen = true;
	time_till_freeze = -1;
	//SpecialID=IDFloor;
	//UpdateMatrix();
	//UpdateTheta();
	//msg_left();
}

void Object::AddForce(const vector &f, const vector &rho)
{
	if (Elapsed <= 0)
		return;
	if (!active_physics)
		return;
	force_ext += f;
	torque_ext += VecCrossProduct(rho, f);
	//TestVectorSanity(f, "f addf");
	//TestVectorSanity(rho, "rho addf");
	//TestVectorSanity(torque, "Torque addf");
	unfreeze(this);
}

void Object::AddTorque(const vector &t)
{
	if (Elapsed <= 0)
		return;
	if (!active_physics)
		return;
	torque_ext += t;
	//TestVectorSanity(Torque,"Torque addt");
	unfreeze(this);
}

void Object::MakeVisible(bool _visible_)
{
	if (_visible_ == visible)
		return;
	if (_visible_)
		GodRegisterModel((Model*)this);
	else
		GodUnregisterModel((Model*)this);
	visible = _visible_;
}

void Object::DoPhysics()
{
	if (Elapsed<=0)
		return;
	msg_db_r("object::DoPhysics",2);
	msg_db_m(name.c_str(),3);
	

	if (_vec_length_fuzzy_(force_int) * mass_inv > AccThreshold)
	{unfreeze(this);}

	if ((active_physics) && (!frozen)){

		if (inf_v(pos))	msg_error("inf   CalcMove Pos  1");
		if (inf_v(vel))	msg_error("inf   CalcMove Vel  1");

			// linear acceleration
			acc = force_int * mass_inv;

			// integrate the equations of motion.... "euler method"
			vel += acc * Elapsed;
			pos += vel * Elapsed;

		if (inf_v(acc))	msg_error("inf   CalcMove Acc");
		if (inf_v(vel))	msg_error("inf   CalcMove Vel  2");
		if (inf_v(pos))	msg_error("inf   CalcMove Pos  2");

		//}

		// rotation
		if ((rot != v_0) || (torque_int != v_0)){

			quaternion q, q_dot, q_w;
			QuaternionRotationV( q, ang );
			q_w = quaternion( 0, rot );
			q_dot = 0.5f * q_w * q;
			q += q_dot * Elapsed;
			ang = q.get_angles();

			#ifdef _realistic_calculation_
				vector L = theta * rot + torque_int * Elapsed;
				UpdateTheta();
				rot = theta_inv * L;
			#else
				UpdateTheta();
				rot += theta_inv * torque_int * Elapsed;
			#endif
		}
	}

	// new orientation
	UpdateMatrix();
	UpdateTheta();

	_ResetPhysAbsolute_();

	// reset forces
	force_int = torque_int = v_0;

	// did anything change?
	moved = false;
	//if ((Pos!=Pos_old)||(ang!=ang_old))
	//if ( (vel_surf!=v_0) || (VecLengthFuzzy(Pos-Pos_old)>2.0f*Elapsed) )//||(VecAng!=ang_old))
	if (active_physics){
		if ( (vel_surf != v_0) || (_vec_length_fuzzy_(vel) > VelThreshold) || (_vec_length_fuzzy_(rot) * radius > VelThreshold))
			moved = true;
	}else{
		frozen = true;
	}
	// would be best to check on the sub models....
	/*if (model)
		if (model->bone.num > 0)
			moved = true;*/

	if (moved){
		unfreeze(this);
		on_ground=false;
	}else if (!frozen){
		time_till_freeze -= Elapsed;
		if (time_till_freeze < 0){
			frozen = true;
			force_ext = torque_ext = v_0;
		}else
			on_ground = false;
	}

	msg_db_l(2);
}


#if 0
Object *Object::CuttingPlane(plane pl)
{
	/*msg_db_out(3,"\nObjectCP");
	msg_db_out(3,Name);
	Object *child=NULL;
	if (model){
		// abgespaltenes Objekt
		child=new Object(NULL);
		(*child)=(*this);
		child->model=model->GetCopy();true);
		plane ipl;
		ipl.a=-pl.a;	ipl.b=-pl.b;	ipl.c=-pl.c;	ipl.d=-pl.d;
		child->model->CuttingPlane(ipl,&Matrix);
		child->Pos+=vector(1000,0,0);
		// eigenes Objekt
		//if (!model->isCopy) // nicht das Orginal verschandeln!
			model=model->GetCopy();true);
		model->CuttingPlane(pl,&Matrix);
	}
	return child;*/
	return NULL;
}
#endif

/*
bool Object::Trace(vector &p1, vector &p2, vector &dir, float range, vector &tp, bool simple_test)
{
	if (!PassivePhysics)
		return false;
	msg_db_r("object::Trace",10);
	msg_db_m(Name,10);

	bool r=model->Trace(p1, p2, dir, range, tp, simple_test);

	msg_db_l(10);
	return r;
}*/

// den Traegheitstensor in Welt-Koordinaten ausrichten
void Object::UpdateTheta()
{
	if (active_physics){
		matrix3 r,r_inv;
		Matrix3Rotation( r, ang );
		Matrix3Transpose( r_inv, r );
		theta = ( r * theta_0 * r_inv );
		Matrix3Inverse( theta_inv, theta );
	}else{
		// Theta and ThetaInv already = identity
		memset(&theta_inv, 0, sizeof(matrix3));
	}
}

void Object::UpdateMatrix()
{
	matrix trans,rot;
	MatrixRotation(rot, ang);
	MatrixTranslation(trans, pos);
	MatrixMultiply(_matrix, trans, rot);
}

// scripts have to call this after 
void Object::UpdateData()
{
	unfreeze(this);
	if (!active_physics){
		UpdateMatrix();
		UpdateTheta();
	}

	// set ode data..
}

