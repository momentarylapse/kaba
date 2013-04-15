#include "x.h"

//#ifdef ...
	#pragma comment(lib,"ode.lib")
//#endif

//#define JUST_SINGLE_COLLISION

struct sObjectSD{
	vector Pos,Vel,Ang,Rot;
	Object *o,*p;
	bool ok;
}ObjectSD[2];

inline bool ainf_v(vector &v)
{
	if (inf_v(v))
		return true;
	return (v.length_fuzzy() > 100000000000.0f);
}

bool ObjectInfinity(Object *o)
{
	return ( ainf_v(o->pos) || ainf_v(o->vel)  || ainf_v(o->rot) || ainf_v(o->ang)  );
}

void SaveObjectData(Object *o,Object *p,int index)
{
	ObjectSD[index].Pos=o->pos;
	ObjectSD[index].Vel=o->vel;
	ObjectSD[index].Ang=o->ang;
	ObjectSD[index].Rot=o->rot;
	ObjectSD[index].ok=!ObjectInfinity(o);
	ObjectSD[index].o=o;
	ObjectSD[index].p=p;
}

void TestObjectData(Object *o,int index,const char *str)
{
	if (!ObjectSD[index].ok)
		return;
	if (ObjectInfinity(o)){
		ObjectSD[index].ok=false;
		msg_error("Fehler bei der Kollision");
		msg_write(str);
		msg_write(o->name + " <-> " + ObjectSD[index].p->name);
	}
}


static void DoHit(Object *o1,Object *o2,CollisionData *col,int i_max,float rc_jump,float rc_sliding,float rc_rolling)
{
	msg_db_r("DoHit",10);
	int i;
	float f[MAX_COLLISIONS],d_all=0;
	float fff[MAX_COLLISIONS];
	for (i=0;i<col->num;i++){
		vector dv= o1->vel - o2->vel + VecCrossProduct(col->pos[i]-o2->pos,o2->rot)-VecCrossProduct(col->pos[i]-o1->pos,o1->rot);
		fff[i]=VecDotProduct(dv,col->normal[i]);
		//msg_write(f2s(fff[i],6));
		if (fff[i]>0)
			d_all+=col->depth[i];
	}
	for (i=0;i<col->num;i++)
		//f[i]=1.0f/col->Num;
		if (fff[i]>0)
			f[i]=col->depth[i]/d_all;
		else
			f[i]=0;


#ifdef JUST_SINGLE_COLLISION
	f[0]=1;
	col->Num=1;
	col->Normal[0]=col->Normal[i_max];
	col->Pos[0]=col->Pos[i_max];
	col->Depth[0]=col->Depth[i_max];
#endif

	// ganz linkes Koordinatensystem!
	vector w1=-o1->rot;
	vector w2=-o2->rot;

	vector rho1[MAX_COLLISIONS],rho2[MAX_COLLISIONS];
	float dp[MAX_COLLISIONS];

	//CollDP__=v0;

	// Anfangswerte speichern
	//vector w1_0=w1,w2_0=w2,v1_0=o1->Vel-o1->VelS,v2_0=o2->Vel-o2->VelS;
	vector w1_0=w1,w2_0=w2,v1_0=o1->vel,v2_0=o2->vel;

	for (i=0;i<col->num;i++){
		vector cp=col->pos[i],n=col->normal[i];

		// Zwischen-Variablen (Drehachsen....)
		rho1[i]=cp-o1->pos;
		vector d1=VecCrossProduct(n,rho1[i]);
		rho2[i]=cp-o2->pos;
		vector d2=VecCrossProduct(n,rho2[i]);
		vector ti_d1,t_w1,ti_d2,t_w2;
		ti_d1 = o1->theta_inv * d1;
		t_w1 = o1->theta * w1_0;
		ti_d2 = o2->theta_inv * d2;
		t_w2 = o2->theta * w2_0;
		if (!o1->active_physics)
			rho1[i]=d1=ti_d1=v_0;
		if (!o2->active_physics)
			rho2[i]=d2=ti_d2=v_0;

		// Energie-Erhaltung: dp=0 oder...
		dp[i]=(	2*VecDotProduct(n,v1_0-v2_0)
				+ VecDotProduct(ti_d1,t_w1)
				+ VecDotProduct(w1_0,d1)
				- VecDotProduct(ti_d2,t_w2)
				- VecDotProduct(w2_0,d2)
			)/( o1->mass_inv + o2->mass_inv
				+ VecDotProduct(ti_d1,d1)
				+ VecDotProduct(ti_d2,d2)
			);
		dp[i]*=f[i];
		//msg_write(f2s(dp[i]/o2->Mass,3));
		//msg_write(f2s(f[i],3));
		//if (dp<0)	dp=0;
		//CollDP__+=n*dp[i]*inv_mass2;


		// normale Reibung
		float dp_r=dp[i]*(2-rc_jump)/2; // maximaler Energieverlust bei dp/2

		// Impuls-Uebertrag
		o1->vel-=n*dp_r*o1->mass_inv;
		o2->vel+=n*dp_r*o2->mass_inv;
		vector dw1,dw2;
		dw1 = o1->theta_inv * (-dp_r*d1);
		w1+=dw1;
		dw2 = o2->theta_inv * (dp_r*d2);
		w2+=dw2;
	}

	vector v1,v2;
	v1=o1->vel+o1->vel_surf;
	v2=o2->vel+o2->vel_surf;

	for (i=0;i<col->num;i++){
		vector n=col->normal[i_max];

		// tangentiale Reibung
			// tangentiale (relative) Geschwindigkeit und deren Richtung berechnen
		vector dv= v1 - v2 - VecCrossProduct(rho2[i],w2)+VecCrossProduct(rho1[i],w1);
		float dv_orth=VecDotProduct(n,dv);
		vector dv_tang=dv-n*dv_orth;
		float dv_tang_l=dv_tang.length();
		//CollDV=dv_tang;
		if (fabs(dv_tang_l)>fabs(dv_orth*0.001f)){ // gegen zu kleine Werte -> Rundungsfehler!
			//msg_write("tang");
			vector dir_tang=dv_tang/dv_tang_l;
			vector dr_ti1,dr_ti2;
			dr_ti1 = o1->theta_inv * VecCrossProduct(dir_tang,rho1[i]);
			dr_ti2 = o2->theta_inv * VecCrossProduct(dir_tang,rho2[i]);
			// maximal uebertragbaren Impuls in tangentialer Richtung
			float dp_max= dv_tang_l / (
										o1->mass_inv + o2->mass_inv
										-VecDotProduct(
												dir_tang,
												VecCrossProduct(dr_ti1,rho1[i])+
												VecCrossProduct(dr_ti2,rho2[i])
										));
			// wirklichen Impuls aus Reibung
			float dp_t=dp[i]*rc_sliding;
			if (dp_t>dp_max)
				dp_t=dp_max;
			// Impuls uebertragen
			vector dp_tang=dir_tang*dp_t;

			o1->vel-=dp_tang*o1->mass_inv;
			o2->vel+=dp_tang*o2->mass_inv;
			vector dLr1=VecCrossProduct(dp_tang,rho1[i]);
			vector dLr2=VecCrossProduct(dp_tang,rho2[i]);
			vector dw1,dw2;
			dw1 = o1->theta_inv * (-dLr1);
			w1+=dw1;
			dw2 = o2->theta_inv * dLr2;
			w2+=dw2;
		}
	}
	//o1->Vel-=o1->VelS;
	//o2->Vel-=o2->VelS;

	// wieder links-rechts
	float rf=(float)pow(1-rc_rolling,Engine.Elapsed);
	o1->rot=-w1*rf;
	o2->rot=-w2*rf; // Reibung eigentlich ueber Differenzen...mit Tensor....

	if (!o1->active_physics)
		o1->rot=o1->vel=v_0;
	if (!o2->active_physics)
		o2->rot=o2->vel=v_0;

	//if (StopOnCollision)
	//	TimeScale=0;
	msg_db_l(10);
}



#ifdef _X_ALLOW_ODE_
inline void vx2ode(vector *vv, dVector3 v)
{
	v[0] = vv->x;
	v[1] = vv->y;
	v[2] = vv->z;
}

static void HandleCollisionsSemiOde(Object *o1, Object *o2, CollisionData *col, float rc_jump, float rc_static, float rc_gliding, float rc_rolling)
{
	if (o1->body_id == o2->body_id){
		msg_error("HandleCollisionsSemiOde: beide Objekte id=id");
		msg_write(p2s(o1->body_id));
		msg_write(o1->name);
		msg_write(o2->name);
		return;
	}
	msg_db_r("HandleCollisionsOde", 1);
	//msg_write(col->Num);
	dContact c;
	c.surface.mode = dContactApprox1 | dContactBounce | dContactSoftCFM | dContactSoftERP;
	c.surface.mu = ((o1->vel - o2->vel).length_fuzzy() + (o1->rot - o2->rot).length_fuzzy() < 1) ? rc_static : rc_gliding;
	c.surface.bounce = rc_jump;
	c.surface.bounce_vel = 0.1;
	c.surface.soft_cfm = 0.01;
	c.surface.soft_erp = 0.4;
	c.geom.g1 = o1->geom_id;
	c.geom.g2 = o2->geom_id;
	for (int i=0;i<col->num;i++){
		vx2ode(&col->pos[i], c.geom.pos);
		col->normal[i] = -col->normal[i];
		vx2ode(&col->normal[i], c.geom.normal);
		c.geom.depth = col->depth[i];
		dJointID joint_id = dJointCreateContact(world_id, contactgroup, &c);
		dJointAttach(joint_id, o1->body_id, o2->body_id);
	}
	msg_db_l(1);
}
#endif


#define CollDepthBias		0.01f

void HandleCollision()
{
	msg_db_r("HandleCollision",10);

	Object *o1 = pColData->o1;
	Object *o2 = pColData->o2;
	
	float rc_jump	=1-((1-o1->rc_jump)*(1-o2->rc_jump));
	float rc_static	=(o1->rc_static+o2->rc_static)/2;
	float rc_sliding=(o1->rc_sliding+o2->rc_sliding)/2;//1-((1-RCJump)*(1-partner->RCJump));
	float rc_rolling=(o1->rc_rolling+o2->rc_rolling)/2;
//	float rc_jump	=0.5f;
//	float rc_static	=0.8f;
//	float rc_sliding=0.4f;
//	float rc_rolling=0.90f;

	//msg_write(Name);
	//msg_write(string2("rs %f   rsl %f   rr %f   rj %f   (%f %f  /  %f %f)", rc_static, rc_sliding, rc_rolling, rc_jump, RCStatic, RCJump, partner->RCStatic, partner->RCJump));

	

	//msg_write(col->Num);
	float d_max = pColData->depth[0];
	int i_max=0;
	for (int k=1;k<pColData->num;k++)
		if (pColData->depth[k]>d_max){
			d_max=pColData->depth[k];
			i_max=k;
		}

#ifdef _X_ALLOW_ODE_
	HandleCollisionsSemiOde(o1, o2, pColData, rc_jump, rc_static, rc_sliding, rc_rolling);
#else

	SaveObjectData(o1, o2, 0);
	SaveObjectData(o2, o1, 1);

	//msg_write(i_max);
	float d=d_max-CollDepthBias;
	float mass_inv=1/(o1->mass + o2->mass);
	//if ((!o1->active_physics)||(!o2->active_physics))
	//	mass_inv=0;
	o1->pos -= pColData->normal[i_max] * d * o2->mass * mass_inv;
	o2->pos += pColData->normal[i_max] * d * o1->mass * mass_inv;

	TestObjectData(o1,0,"a");
	TestObjectData(o2,1,"a");
	if (!ObjectSD[0].ok){
		msg_write(format("d = %f   mass_inv = %f     %f %f)",d,mass_inv,d*o2->mass*mass_inv,d*o1->mass			*mass_inv));

		msg_write(i_max);
		for (int k=1;k<pColData->num;k++)
			msg_write(f2s(pColData->depth[k],4));
	}


	DoHit(o1,o2,pColData,i_max,rc_jump,rc_sliding,rc_rolling);


	TestObjectData(o1,0,"b");
	TestObjectData(o2,1,"b");
	if (!ObjectSD[0].ok){
		msg_write(format("d = %f   mass_inv = %f     %f %f)",d,mass_inv,d*o2->mass*mass_inv,d*o1->mass			*mass_inv));

		msg_write(i_max);
		for (int k=1;k<pColData->num;k++)
			msg_write(format("dep %f   n (%f %f %f)  p(%f %f %f)", pColData->depth[k], pColData->normal[k].x, pColData->normal[k].y, pColData->normal[k].z,
			    										pColData->pos[k].x, pColData->pos[k].y, pColData->pos[k].z));
	}

#endif

	float f;
	o1->acc = -World.gravity;//o1->ForceExt;
	o2->acc = -World.gravity;//o2->ForceExt;
	f = -VecDotProduct(o1->acc,pColData->normal[i_max]);
	if (f < 0){
		if (o1->on_ground){
			float f2=VecDotProduct(o1->acc,o1->ground_normal);
			if (f2>f){
				o1->ground_normal=-pColData->normal[i_max];
				o1->ground_id=o2->object_id;
			}
		}else{
			o1->ground_normal=-pColData->normal[i_max];
			o1->on_ground=true;
			o1->ground_id=o2->object_id;
		}
	}
	f = VecDotProduct(o2->acc,pColData->normal[i_max]);
	if (f < 0){
		if (o2->on_ground){
			float f2=VecDotProduct(o2->acc,o2->ground_normal);
			if (f2>f){
				o2->ground_normal=pColData->normal[i_max];
				o2->ground_id=o1->object_id;
			}
		}else{
			o2->ground_normal=pColData->normal[i_max];
			o2->on_ground=true;
			o2->ground_id=o1->object_id;
		}
	}
	TestObjectData(o1,0,"c");
	TestObjectData(o2,1,"c");

	msg_db_l(10);
}

