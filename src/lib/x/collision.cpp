/*----------------------------------------------------------------------------*\
| Collision detection                                                          |
| -> model - model                                                             |
| -> model - terrain                                                           |
|                                                                              |
| last update: 2010.04.02 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include <algorithm>
#include "x.h"

#define USE_OBSERVERS
//#define GARBAGE_AS_COLLISIONS

CollisionData ColData;
CollisionData *pColData = &ColData;
static CollisionData CollInv;
int CollLevel = 0; // for function recursion
#ifdef _X_ALLOW_PHYSICS_DEBUG_
	sCollisionData PhysicsDebugColData;

	void DebugAddCollision(CollisionData &col)
	{
		//pthread_mutex_lock (&mx_col);
		for (int k=0;k<col.num;k++)
			if (PhysicsDebugColData.num < MAX_COLLISIONS){
				PhysicsDebugColData.fepth[PhysicsDebugColData.num] = col.depth[k];
				PhysicsDebugColData.pos[PhysicsDebugColData.num] = col.pos[k];
				PhysicsDebugColData.normal[PhysicsDebugColData.num++] = col.normal[k];
			}
		//pthread_mutex_unlock (&mx_col);
	}
#endif

#define inline

//--------------------------------------------------------------------------------------------------
// collision detection
//--------------------------------------------------------------------------------------------------

#ifdef terrain
#undef terrain
#endif

struct s_col_poly_data
{
	vector cp;
	int face, edge, vert;
};

inline void add_col_poly_data(Array<s_col_poly_data> &cut, const vector &cp, int vert, int edge, int face)
{
	s_col_poly_data d;
	d.cp = cp;
	d.face = face;
	d.edge = edge;
	d.vert = vert;
	cut.add(d);
}

// observer for collision detection

enum{
	OTPolyPoly,
	OTPolyBall
};

int NumObservers=0;
#define MODEL_MAX_OBSERVERS			4096
static CModel *obs_ma=NULL,*obs_mb=NULL;
static int obs_type,obs_i1,obs_i2;
static bool obs_ok=false;

struct sObserver
{
	CModel *ma,*mb;
	int type;
	int i1,i2,i3,i4;
	bool still_ok;
	bool operator == (const sObserver &o) const
	{
		if (ma != o.ma)		return false;
		if (mb != o.mb)		return false;
		if (i1 != o.i1)		return false;
		if (i2 != o.i2)		return false;
		return true;
	}
	bool operator < (const sObserver &o) const
	{
		if (ma < o.ma)		return true;
		if (ma > o.ma)		return false;
		if (mb < o.mb)		return true;
		if (mb > o.mb)		return false;
		if (i1 < o.i1)		return true;
		if (i1 > o.i1)		return false;
		if (i2 < o.i2)		return true;
		if (i2 > o.i2)		return false;
		/*if (i3 < o.i3)		return true;
		if (i3 > o.i3)		return false;
		if (i4 < o.i4)		return true;
		if (i4 > o.i4)		return false;*/
		return false;
	}
	bool operator == (int i)
	{
		if (ma != obs_ma)		return false;
		if (mb != obs_mb)		return false;
		if (i1 != obs_i1)		return false;
		if (i2 != obs_i2)		return false;
		return true;
	}
	bool operator < (int i)
	{
		if (ma < obs_ma)		return true;
		if (ma > obs_ma)		return false;
		if (mb < obs_mb)		return true;
		if (mb > obs_mb)		return false;
		if (i1 < obs_i1)		return true;
		if (i1 > obs_i1)		return false;
		if (i2 < obs_i2)		return true;
		if (i2 > obs_i2)		return false;
		/*if (i3 < o.i3)		return true;
		if (i3 > o.i3)		return false;
		if (i4 < o.i4)		return true;
		if (i4 > o.i4)		return false;*/
		return false;
	}
};
static sObserver observer[MODEL_MAX_OBSERVERS];//, *sorted_observer[MODEL_MAX_OBSERVERS];



// if > all known... return NumObservers
inline int _find_first_obs_ge_index_()
{
	//printf("find...%d\n", NumObservers);
	if (NumObservers < 16){
		for (int n=0;n<NumObservers;n++){
			if (observer[n] == 0)
				return n;
			if (!(observer[n] < 0))
				return n;
		}
		return NumObservers;
	}
	int dn = 1;
	while (dn < NumObservers)
		dn <<= 1;
	dn >>= 1;
	int n = dn;
	dn >>= 1;
	while(dn > 0){
	//printf("n...%d\n", n);
	//printf("dn...%d\n", dn);
		if (observer[n] == 0){
			//printf("_obs  %d\n", n);
			return n;//&observer[n];
		}
		if (observer[n] < 0){
			//printf("++\n", n);
			n += dn;
			if (n >= NumObservers){
				//printf("zu gross\n", n);
				n = NumObservers - 1;
				if (observer[n] < 0){
					//printf(" ausser\n");
					return NumObservers;
				}
			}
		}else{
			//printf("--\n");
			n -= dn;
			/*if (n < 0)
				msg_error("<0");*/
		}
		dn >>= 1;
	}
	//printf("_obs x  %d\n", n);
	if (n == NumObservers - 1){
		if (observer[n] < 0){
			//printf("_obs xx  \n");
			return NumObservers;
		}
	}
	// last step...
	if (observer[n] == 0){
		//printf("_obs 0  %d\n", n);
		return n;//&observer[n];
	}
	if (observer[n] < 0){
		//printf("_obs ++\n");
		return n+1;//&observer[n];
	}
	if (n == 1){
		if (observer[0] < 0){
			//printf("_obs 0 < 0\n");
			return 1;
		}
			//printf("_obs 0 !< 0\n");
			return 0;
	}
	return n;
	
}

void _add_observer_poly_poly_(int pli,int mi)
{
	//printf("add\n");

	/*for (int i=0;i<NumObservers;i++){
		if (observer[i] < 0)
			printf("%d <\n", i);
		if (observer[i] == 0)
			printf("%d =\n", i);
	}*/
	
	int n = _find_first_obs_ge_index_();
	//printf("%d   %d\n", n, NumObservers);
	if (observer[n] == 0){
		//msg_error("add: =");
		return;
	}
	if (n < NumObservers){
		//printf("schiebe\n");
		for (int i=NumObservers-1;i>=n;i--){
			//printf("%d = %d\n", i+1, i);
			observer[i+1] = observer[i];
		}
	}
		//printf("insert\n");
	observer[n].ma=obs_ma;
	observer[n].mb=obs_mb;
	observer[n].type=OTPolyPoly;
	observer[n].i1=obs_i1;
	observer[n].i2=obs_i2;
	observer[n].i3=pli;
	observer[n].i4=mi;
	observer[n].still_ok=true;
	NumObservers++;
	//std::sort(observer, observer + NumObservers);
	/*	printf("test\n");
	bool ok = true;
	if (NumObservers >= 2){
	for (int i=0;i<NumObservers-1;i++){
		if (observer[i] < observer[i + 1])
			printf("%d < %d\n", i, i+1);
		if (observer[i+1] < observer[i])
			printf("%d > %d\n", i, i+1);
		if (observer[i] == observer[i + 1])
			printf("%d = %d\n", i, i+1);
		if (!(observer[i] < observer[i+1])){
			ok = false;
		}
	}
	}
	if (!ok){
		printf("!ok\n");
		exit(0);
	}
	else
		printf("ok\n");*/
}

sObserver *_get_observer_(int type)
{
	/*for (int i=0;i<NumObservers;i++){
		sObserver *obs=&observer[i];
		if (obs->ma!=obs_ma)
			continue;
		if (obs->mb!=obs_mb)
			continue;
		if (obs->type!=type)
			continue;
		if (obs->i1!=obs_i1)
			continue;
		if (obs->i2!=obs_i2)
			continue;
		return obs;
	}*/
	int i = _find_first_obs_ge_index_();
	if (i == NumObservers)
		return NULL;
	if (observer[i] == 0)
		return &observer[i];
	return NULL;
	/*printf("%d\n", n);
	if (n >= 0){
		printf("a0\n");
		return &observer[n];
	}
	printf("a\n");
	return NULL;*/
}

// throw away observers not used anymore
void DoCollisionObservers()
{
	int no=NumObservers,di=0;
	for (int i=0;i<no;i++){
		if (observer[i].still_ok){
			observer[i].still_ok=false;
			if (di>0)
				observer[i-di]=observer[i];
		}else{
			di++;
			NumObservers--;
		}
	}
}

inline void _add_inv_col_(CollisionData &col)
{
	msg_db_r("add_inv_col",10);
	for (int i=0;i<CollInv.num;i++){
		col.depth[col.num] = CollInv.depth[i];
		col.normal[col.num] = -CollInv.normal[i];
		col.pos[col.num] = CollInv.pos[i];// - col.normal[col.num] * col.depth[col.num];
		col.num ++;
	}
	//msg_write(col.Num);
	msg_db_l(10);
}

const char *col_type;

#define set_col_type(ct)	(col_type=ct)

int num_bad_colls=0;

inline void _add_col_(CollisionData &col, float depth, const vector &n, const vector &pos)
{
	if (depth==0){
		msg_write("ignoring collision (depth=0)");
		msg_write(msg_get_trace());
		return;
	}
	if (n==v_0){
		msg_write("ignoring collision (normal=0)");
		msg_write(msg_get_trace());
		return;
	}
	if (col.num<MAX_COLLISIONS){
		if ((inf_f(depth)) || (inf_v(n)) || (inf_v(pos))){
			num_bad_colls++;
			if (num_bad_colls<100){
				msg_error("bad collision data");
				msg_write(col_type);
				msg_write(format("depth: %f",depth));
				msg_write(format("n: ( %f , %f , %f )",n.x,n.y,n.z));
				msg_write(format("pos: ( %f , %f , %f )",pos.x,pos.y,pos.z));
			}

		HuiRaiseError("boese Physik erkannt.... kille!");

			return;
		}
		col.depth[col.num] = depth;
		col.normal[col.num] = n;
		col.pos[col.num] = pos;
		col.num ++;
	}else{
		msg_error("too many collisions");
	}
}

inline void _get_nearest_point_in_line_(vector &l1,vector &l2,vector &p,vector &cp)
{
	vector l_dir = l2 - l1;
	_vec_normalize_(l_dir);
	cp=l1+(l_dir*(p-l1))*l_dir;
}


// ball - vertex
inline bool GetCollisionBallVertex(vector &ball_p,float ball_r,vector &p,CollisionData &col)
{
	msg_db_r("GetCol BallVertex",10);

	/*msg_write(string2("ball_p: (%f,%f,%f)",ball_p.x,ball_p.y,ball_p.z));
	msg_write(string2("ball_r: %f",ball_r));
	msg_write(string2("p: (%f,%f,%f)",p.x,p.y,p.z));*/

	// near enough?
	if (_vec_length_fuzzy_(ball_p-p)>ball_r){
		msg_db_l(10);
		return false;
	}

	// inside?
	float dist=_vec_length_(ball_p-p);
	//msg_write(string2("dist: %f",dist));
	if (dist<ball_r){
		vector v,n=(p-ball_p)/dist;
	//msg_write(string2("n: (%f,%f,%f)",n.x,n.y,n.z));
		set_col_type("ball vert");
		_add_col_(col,	ball_r-dist,
						n,
						p);
		msg_db_l(10);
		return true;
	}
	msg_db_l(10);
	return false;
}

// ball - edge
inline bool GetCollisionBallEdge(vector &ball_p,float ball_r,vector &e1,vector &e2,CollisionData &col)
{
	msg_db_r("GetCol BallEdge",10);

	/*msg_write(string2("ball_p: (%f,%f,%f)",ball_p.x,ball_p.y,ball_p.z));
	msg_write(string2("ball_r: %f",ball_r));
	msg_write(string2("e1: (%f,%f,%f)",e1.x,e1.y,e1.z));
	msg_write(string2("e2: (%f,%f,%f)",e2.x,e2.y,e2.z));*/
	// nearest point on edge
	vector n,cp;
	_get_nearest_point_in_line_(e1,e2,ball_p,cp);
	//msg_write(string2("cp: (%f,%f,%f)",cp.x,cp.y,cp.z));

	// on the edge?
	if (!_vec_between_(cp,e1,e2)){
		msg_db_l(10);
		return false;
	}

	// within the ball?
	vector dp=cp-ball_p;
	if (_vec_length_fuzzy_(dp)>ball_r){
		msg_db_l(10);
		return false;
	}
	float d=_vec_length_(dp);
	if (d>=ball_r){
		msg_db_l(10);
		return false;
	}

	set_col_type("ball edge");
	_add_col_(col, ball_r - d, dp/d, cp);

	msg_db_l(10);
	return true;
}

// own ball - partner ball
inline bool GetCollisionBallBall(	PhysicalSkinAbsolute *o_abs,Ball *bo,
									PhysicalSkinAbsolute *p_abs,Ball *bp,
									CollisionData &col)
{
	msg_db_r("GetCol BallBall",10);
	vector *vo=o_abs->p,*vp=p_abs->p;

	float d_opt = bo->radius + bp->radius;
	vector dp = vp[bp->index] - vo[bo->index];

	// "bounding box"
	if (_vec_length_fuzzy_(dp)>d_opt){
		msg_db_l(10);
		return false;
	}

	float d_act=_vec_length_(dp);
	float depth=d_opt-d_act;
	if (depth>0){
		col.depth[col.num] = depth;
		vector n=v_0;
		if (d_act>=0.00000001f)
			n=dp/d_act;

		// error testing (DEBUG!!!!)
		if (inf_v(n))
			msg_error("mgc obpb    Normal inf");
		if (_vec_length_(n)<0){
			msg_error("neg. post");
			msg_write(f2s(d_act,2));
		}
		set_col_type("ball ball");
		_add_col_(col, depth, n, vo[bo->index] + n * bo->radius);
		msg_db_l(10);
		return true;
	}

	msg_db_l(10);
	return false;
}

static float pl_dist[MODEL_MAX_POLY_FACES];

// ball - polyhedron
inline bool GetCollisionBallPoly(	Ball *bo, PhysicalSkinAbsolute *o_abs,
									ConvexPolyhedron *pp, vector *vp, plane *plp,
									CollisionData &col)
{
	msg_db_r("GetCol BallPoly",10);
	bool hit=false;

	//vector c,ta,tb,tc,ea,eb;
	vector ball_p = o_abs->p[bo->index];
	float ball_r = bo->radius;

	float d_min=bo->radius;

	// ball - poly vertices
	for (int i=0;i<pp->num_vertices;i++)
		hit|=GetCollisionBallVertex(ball_p,ball_r,vp[pp->vertex[i]],col);

	// ball - poly edge
	for (int i=0;i<pp->num_edges;i++)
		hit|=GetCollisionBallEdge(ball_p,ball_r,vp[pp->edge_index[i*2  ]],vp[pp->edge_index[i*2+1]],col);

	for (int k=0;k<pp->num_faces;k++)
		pl_dist[k]=_plane_distance_(plp[k],ball_p); // increasing outwards!!!

	// ball - poly face (polygon)
	bool inside=false;
	int nearest=-1;
	float depth=TraceMax;
	for (int k=0;k<pp->num_faces;k++){
		// ball in plane?
		if ((pl_dist[k]>ball_r)||(pl_dist[k]<-ball_r))
			continue;
		// smallest depth?
		if (ball_r-pl_dist[k]<depth){
			// collision point on ball
			//vector cp_b=ball_p - ball_r*_get_normal_(plp[k]);
			// collision point on plane
			vector cp_p=ball_p - pl_dist[k] * plp[k].n;
			// point within polygon?
			// ...cut into triangles
			for (int i=0;i<pp->face[k].num_vertices-2;i++){
				float f,g;
				GetBaryCentric(cp_p,vp[pp->face[k].index[0]],vp[pp->face[k].index[i+1]],vp[pp->face[k].index[i+2]],f,g);
				if ((f>=0)&&(g>=0)&&(f+g<=1)){
					depth=ball_r-pl_dist[k];
					nearest=k;
					inside=true;
					break;
				}
			}
		}
	}
	if (inside){
		vector n = - plp[nearest].n;
		set_col_type("ball poly 1");
		_add_col_(col,depth,n, ball_p+ball_r*n);
		if (inf_v(n))
			msg_error("mgc obpp 2    Normal inf");
		hit=true;
	}
	
	// ball - poly volume
	/*inside=true;
	nearest=-1;
	depth=TraceMax;
	for (int k=0;k<pp->num_faces;k++){
		if (pl_dist[k]>-ball_r){//0){
			inside=false;
			break;
		}
		if (ball_r-pl_dist[k]<depth){
			depth=ball_r-pl_dist[k];
			nearest=k;
		}
	}
	if (inside){
		vector n=-_get_normal_(plp[nearest]);
		_add_col_(col,depth,n,	ball_p+ball_r*n, // on ball
								ball_p+pl_dist[nearest]*n); // on polyeder
		msg_db_l(10);
		return true;
	}*/

	msg_db_l(10);
	return hit;
}


// polyhedron - vertex
inline void GetCollisionPolyVertex(ConvexPolyhedron *po,vector *vo,plane *plo,
								   vector &p,Array<s_col_poly_data> &cut,int vert)
{
	msg_db_r("GetCol PolyVertex",10);

	// in volume?
	for (int i=0;i<po->num_faces;i++){
		pl_dist[i]=-_plane_distance_(plo[i],p);
		if (pl_dist[i]<0){
			msg_db_l(10);
			return;
		}
	}
	
	add_col_poly_data(cut, p, vert, 0, 0);
	msg_db_l(10);
}


// polyhedron faces - edge
inline void GetCollisionPolyFacesEdge(ConvexPolyhedron *po,vector *vo,plane *plo,
								 vector &e1, vector &e2,Array<s_col_poly_data> &cut,int edge)
{
	msg_db_r("GetCol PolyEdge",10);

	// points of intersection?
	int num_cuts=0;
	for (int i=0;i<po->num_faces;i++){
		// edge intersecting the plane (both points on different sides?)
		if (_plane_distance_(plo[i],e1) * _plane_distance_(plo[i],e2) > 0)
			continue;
		
		// intersection   line - plane
		vector cp;
		if (!_plane_intersect_line_(cp,plo[i],e1,e2))
			continue;
		// within edge?
		/*if (!_vec_between_(cp,e1,e2))
			continue;*/
		// within poly face (polygon)?
		bool inside=true;
		for (int k=0;k<po->num_faces;k++)
			if ((k!=i)&&(_plane_distance_(plo[k],cp)>0)){
				inside=false;
				break;
			}
		if (!inside)
			continue;
		add_col_poly_data(cut, cp, 0, edge, i);
		num_cuts ++;
		
		// convex -> there shouldn't be more than 2 cuts
		if (num_cuts==2)
			break;
	}

	msg_db_l(10);
}

inline bool poly_vert_on_edge(ConvexPolyhedron *p, int vert, int edge)
{
	return (p->edge_index[edge*2] == p->vertex[vert]) || (p->edge_index[edge*2+1] == p->vertex[vert]);
}

#define poly_edge_on_face(p, edge, face)				((p)->edge_on_face[(edge) * (p)->num_faces + (face)])
#define poly_faces_get_joining_edge(p, face1, face2)	((p)->faces_joining_edge[(face1) * (p)->num_faces + (face2)])

inline float line_dist_normal(const vector &l0, const vector &l1, const vector &L0, const vector &L1, vector &n)
{
    vector dir_l = l0 - l1;
    vector dir_L = L0 - L1;
	n = dir_l - dir_L;
	_vec_normalize_(n);
	float d = n * (l0 - L0);
	if (d < 0){
		d = - d;
		n = - n;
	}
	return d;
}

// (my volume <-> other vertex) <-> (my face <-> other edge)
// edge ends in volume, cuts face -> combine both points
inline void process_poly_dat_vv_fe(	Array<s_col_poly_data> &vv, Array<s_col_poly_data> &fe,
								    ConvexPolyhedron *po,vector *vo,plane *plo,
									ConvexPolyhedron *pp,vector *vp,plane *plp,
    								CollisionData &col, float sign)
{
	msg_db_r("process_vv_fe", 3);
	// other points in my volume
	for (int i=vv.num-1;i>=0;i--){
		// find my nearest plane... cut by an edge this vertex is on
		int jmin = -1;
		float dmin = 10000000000000000.0f; // "poly->diameter"...
		for (int j=fe.num-1;j>=0;j--)
			if (poly_vert_on_edge(pp, vv[i].vert, fe[j].edge)){
				int face = fe[j].face;
				//vector n = plo[face].n;
				float d = fabs(_plane_distance_(plo[face], vv[i].cp));
				if (d < dmin){
					dmin = d;
					jmin = j;
				}
			}

		if (jmin >= 0){
			int face = fe[jmin].face;
			int edge = fe[jmin].edge;

			// add collision
			vector n = plo[face].n;
			//printf("kante  %d %d  (me: %d)\n", vv[i].vert, fe[jmin].edge, po->num_faces);
			float dmin = fabs(_plane_distance_(plo[face], vv[i].cp));
			if (dmin > 0)
				_add_col_(col, dmin, n * sign, vv[i].cp);

			// remove deprecated data...(points where edges ending in this vertex cut this face)
			vv.erase(i);
			for (int j=fe.num-1;j>=0;j--)
				if (fe[j].face == face)
					if (poly_vert_on_edge(pp, vv[i].vert, fe[j].edge)){
						//printf("++ %d\n", j);
						fe.erase(j);
					}
		}
	}
	msg_db_l(3);
}

// (my edge <-> other face) <-> (my face <-> other edge)
// edge cuts 2 joining faces -> join all
inline void process_poly_dat_fe_fe(	Array<s_col_poly_data> &feo, Array<s_col_poly_data> &fep,
								    ConvexPolyhedron *po,vector *vo,plane *plo,
									ConvexPolyhedron *pp,vector *vp,plane *plp,
    								CollisionData &col, float sign)
{
	msg_db_r("process_fe_fe", 3);
	for (int i=feo.num-2;i>=0;i--){
		for (int j=feo.num-1;j>i;j--) // i < j
			// 2 points on the same edge (me)
			if (feo[i].edge == feo[j].edge){
				// faces joined by an edge (other)
				int edge = poly_faces_get_joining_edge(pp, feo[i].face, feo[j].face);
				if (edge < 0)
					continue;

				// this edge in fep?
				bool found = false;
				for (int k=fep.num-1;k>=0;k--)
					if (fep[k].edge == edge)
						if (poly_edge_on_face(po, feo[i].edge, fep[k].face)){
//						printf("Kante %d %d %d - %d %d\n", i, j, feo[i].edge, k, fep[k].edge);
						int e_o = feo[i].edge;
						vector n;
						float d = line_dist_normal(	vo[po->edge_index[e_o  * 2]], vo[po->edge_index[e_o  * 2 + 1]],
						    						vp[pp->edge_index[edge * 2]], vp[pp->edge_index[edge * 2 + 1]], n);
						vector cp = (feo[i].cp + feo[j].cp) * 0.5f; //fep[k].cp + feo[i].cp + feo[j].cp;
						//int num = 3;
				
						fep.erase(k);
						for (int l=k-1;l>=0;l--)
							if (fep[l].edge == edge)
								if (poly_edge_on_face(po, feo[i].edge, fep[l].face)){
//									printf("++\n");
									//cp += fep[k].cp;
									//num ++;
									//fep.erase(fep.begin() + l);
								}
						feo.erase(j);
						feo.erase(i);
						
						_add_col_(col, d, n * sign, cp);// cp / num);
						
						found = true;
						break;
					}
				if (found)
					break;
			}
	}
	msg_db_l(3);
}

// (my edge <-> other face) <-> (my face <-> other edge)
// 1 pair of points... each one's edge joined to the other's face -> combine
inline void process_poly_dat_fe_fe2(Array<s_col_poly_data> &feo, Array<s_col_poly_data> &fep,
								    ConvexPolyhedron *po,vector *vo,plane *plo,
									ConvexPolyhedron *pp,vector *vp,plane *plp,
    								CollisionData &col, float sign)
{
	msg_db_r("process_fe_fe2", 3);
	for (int i=feo.num-1;i>=0;i--)
		for (int j=fep.num-1;j>=0;j--){
			// my poly
			//printf("%d %d\n", feo[i].edge, fep[j].face);
			if (!poly_edge_on_face(po, feo[i].edge, fep[j].face))
				continue;
			// other poly
			if (!poly_edge_on_face(pp, fep[j].edge, feo[i].face))
				continue;
//			printf("Paar  %d %d\n", i, j);
			vector n;
			int e_o = feo[i].edge;
			int e_p = fep[j].edge;
			float d = line_dist_normal(	vo[po->edge_index[e_o * 2]], vo[po->edge_index[e_o * 2 + 1]],
			    						vp[pp->edge_index[e_p * 2]], vp[pp->edge_index[e_p * 2 + 1]], n);

			_add_col_(col, d, n * sign, feo[i].cp);// cp / num);
			
			feo.erase(i);
			fep.erase(j);
			break;
		}
	msg_db_l(3);
}


// polyhedron - polyhedron
inline bool GetCollisionPolyPoly(	ConvexPolyhedron *po,vector *vo,plane *plo,
									ConvexPolyhedron *pp,vector *vp,plane *plp,
									CollisionData &col)
{
	msg_db_r("GetCol PolyPoly",10);
	bool hit=false;


	// test observer
#ifdef USE_OBSERVERS
	sObserver *obs=_get_observer_(OTPolyPoly);
	if (obs){
		int pli=obs->i3;
		if (obs->i4==0){
			bool outside=true;
			for (int i=0;i<pp->num_vertices;i++)
				if (_plane_distance_(plo[pli],vp[pp->vertex[i]])<0){
					outside=false;
					break;
				}
			if (outside){
				obs->still_ok=true;
				msg_db_l(10);
				return false;
			}
		}else{
			bool outside=true;
			for (int i=0;i<po->num_vertices;i++)
				if (_plane_distance_(plp[pli],vo[po->vertex[i]])<0){
					outside=false;
					break;
				}
			if (outside){
				obs->still_ok=true;
				msg_db_l(10);
				return false;
			}
		}
	}
#endif

	Array<s_col_poly_data> cut[4];


	// my volume - other vertex
	for (int i=0;i<pp->num_vertices;i++)
		GetCollisionPolyVertex(po,vo,plo,vp[pp->vertex[i]],cut[0],i);

	// vertex - volume
	for (int i=0;i<po->num_vertices;i++)
		GetCollisionPolyVertex(pp,vp,plp,vo[po->vertex[i]],cut[1],i);

	
	// face - edge
	for (int i=0;i<pp->num_edges;i++)
		GetCollisionPolyFacesEdge(po,vo,plo,vp[pp->edge_index[i*2]],vp[pp->edge_index[i*2+1]],cut[2],i);

	// edge - faces
	for (int i=0;i<po->num_edges;i++)
		GetCollisionPolyFacesEdge(pp,vp,plp,vo[po->edge_index[i*2]],vo[po->edge_index[i*2+1]],cut[3],i);

	hit = (cut[0].num > 0) || (cut[1].num > 0) || (cut[2].num > 0) || (cut[3].num > 0);

	// post process data...
//	printf("------\n%d  %d  %d  %d\n", cut[0].num, cut[1].num, cut[2].num, cut[3].num);
	process_poly_dat_vv_fe(cut[0], cut[2], po,vo,plo, pp,vp,plp, col, 1);
	process_poly_dat_vv_fe(cut[1], cut[3], pp,vp,plp, po,vo,plo, col, -1);
//	printf("->  %d  %d  %d  %d\n", cut[0].num, cut[1].num, cut[2].num, cut[3].num);
	process_poly_dat_fe_fe(cut[3], cut[2], po,vo,plo, pp,vp,plp, col, 1);
	process_poly_dat_fe_fe(cut[2], cut[3], pp,vp,plp, po,vo,plo, col, -1);
//	printf("->  %d  %d  %d  %d\n", cut[0].num, cut[1].num, cut[2].num, cut[3].num);
	process_poly_dat_fe_fe2(cut[3], cut[2], po,vo,plo, pp,vp,plp, col, 1);
	process_poly_dat_fe_fe2(cut[2], cut[3], pp,vp,plp, po,vo,plo, col, -1);
//	printf("->  %d  %d  %d  %d\n", cut[0].num, cut[1].num, cut[2].num, cut[3].num);


#ifdef GARBAGE_AS_COLLISIONS
	// garbage
	for (int i=0;i<4;i++)
		for (int j=0;j<cut[i].num;j++)
			_add_col_(col, 0.001f, v_0, cut[i][j].cp);
#endif


#ifdef USE_OBSERVERS
	// find observer
	if (!hit){
		bool found_obs=false;
		for (int i=0;i<po->num_faces;i++){
			bool outside=true;
			for (int j=0;j<pp->num_vertices;j++){
				if (_plane_distance_(plo[i],vp[pp->vertex[j]])<0){
					outside=false;
					break;
				}
			}
			if (outside){
				_add_observer_poly_poly_(i,0);
				found_obs=true;
				break;
			}
		}
		if (!found_obs)
		for (int i=0;i<pp->num_faces;i++){
			bool outside=true;
			for (int j=0;j<po->num_vertices;j++)
				if (_plane_distance_(plp[i],vo[po->vertex[j]])<0){
					outside=false;
					break;
				}
			if (outside){
				_add_observer_poly_poly_(i,1);
				break;
			}
		}
	}
#endif

	msg_db_l(10);
	return hit;
}

// skin - skin (physical)
inline bool GetCollisionPhysPhys(	PhysicalSkin *o, PhysicalSkinAbsolute *o_abs,
									PhysicalSkin *p, PhysicalSkinAbsolute *p_abs,
									CollisionData &col)
{
	msg_db_r("GetCol PhysPhys",10);
	bool hit=false;
	
	// DEBUG!!!
	/*for (int i=0;i<o->num_vertices;i++)
		if (inf_v(o_abs->p[i]))
			msg_error("mgc oppp 1    vorher inf");
	for (int i=0;i<p->num_vertices;i++)
		if (inf_v(p_abs->p[i]))
			msg_error("mgc oppp 1    vorher inf p");*/


	// ball - ball
	for (int i=0;i<o->num_balls;i++)
		for (int j=0;j<p->num_balls;j++)
			hit|=GetCollisionBallBall(o_abs,&o->ball[i],p_abs,&p->ball[j],col);

	// ball - polyhedron
	for (int i=0;i<o->num_balls;i++)
		for (int j=0;j<p->num_polys;j++)
			hit|=GetCollisionBallPoly(&o->ball[i],o_abs,&p->poly[j],p_abs->p,&p_abs->pl[j*MODEL_MAX_POLY_FACES],col);

	// polyhedron - ball
	CollInv.num=0;
	for (int i=0;i<o->num_polys;i++)
		for (int j=0;j<p->num_balls;j++)
			hit|=GetCollisionBallPoly(&p->ball[j],p_abs,&o->poly[i],o_abs->p,&o_abs->pl[i*MODEL_MAX_POLY_FACES],CollInv);
	_add_inv_col_(col);

	// polyhedron - polyhedron
	for (int i=0;i<o->num_polys;i++)
		for (int j=0;j<p->num_polys;j++){
			obs_i1=i;
			obs_i2=j;
			hit|=GetCollisionPolyPoly(	&o->poly[i],o_abs->p,&o_abs->pl[i*MODEL_MAX_POLY_FACES],
										&p->poly[j],p_abs->p,&p_abs->pl[j*MODEL_MAX_POLY_FACES],col);
		}

	msg_db_l(10);
	return hit;
}

// ball - hull
inline bool GetCollisionBallHull(Ball *bo,vector *vo,TriangleHull *hull,CollisionData &col)
{
	msg_db_r("GetCol BallHull",10);
	bool hit=false;
	float r=bo->radius;
	vector ball_p=vo[bo->index];

	// hull vertices - ball volume
	for (int i=0;i<hull->num_vertices;i++)
		hit|=GetCollisionBallVertex(ball_p,r,hull->p[hull->index[i]],col);

	// hull edges - ball volume
	for (int i=0;i<hull->num_edges;i++)
		hit|=GetCollisionBallEdge(ball_p,r,hull->p[hull->edge_index[i*2  ]],hull->p[hull->edge_index[i*2+1]],col);

	// hull triangle - ball volume
	for (int i=0;i<hull->num_triangles;i++){
		vector a=hull->p[hull->triangle_index[i*3  ]];
		vector b=hull->p[hull->triangle_index[i*3+1]];
		vector c=hull->p[hull->triangle_index[i*3+2]];
		plane pl=hull->pl[i];

		// ball touches plane?
		float d=_plane_distance_(pl,ball_p);
		if ((d<-r)||(d>r))
			continue;

		// where do we touch?
		vector cp=ball_p-d*pl.n;
		float f,g;
		_get_bary_centric_(cp,pl,a,b,c,f,g);
		if ((f>0)&&(g>0)&&(f+g<1)){
			set_col_type("ball hull");
			_add_col_(col,r-d,-pl.n,cp);
			hit|=true;
		}
	}
	msg_db_l(10);
	return hit;
}

#if 0
// triangle face - edge
inline bool GetCollisionTriangleEdge(plane &pl,vector &a,vector &b,vector &c,vector &e1,vector &e2,sCollisionData &col)
{
	msg_db_r("GetCol TriaEdge",10);
	// point of intersetion (plane - line)
	vector cp;
	if (!_plane_intersect_line_(cp,pl,e1,e2)){
		msg_db_l(10);
		return false;
	}

	// on edge?
	if (!_vec_between_(cp,e1,e2)){
		msg_db_l(10);
		return false;
	}

	// within triangle?
	float f,g;
	_get_bary_centric_(cp,pl,a,b,c,f,g);
	if ((f<0)||(g<0)||(f+g>1)){
		msg_db_l(10);
		return false;
	}

	//msg_write("hit");

// nearest exit?
	float d,d_min=TraceMax;
	int nearest=-1;
	vector e_dir=e2-e1;
	_vec_normalize_(e_dir);
	vector n=_get_normal_(pl);

	vector coll_n,coll_p,coll_pp;


	// sort edge...
	vector e1_s;
	if (e_dir*n<0){
		e_dir=-e_dir;
		e1_s=e2;
	}else
		e1_s=e1;
	// don't use e1,e2 from here on!!!

	// edge too tangential on triangle?
	if (n*e_dir<0.2f){
		msg_db_l(10);
		return false;
	}

	// e1 on triangle face
	d=(cp-e1_s)*n;
	//msg_write(f2s(d,2));
	if (d<d_min){
		d_min=d;
		nearest=0;
		coll_n=n; // triangle is "single sided"...
		coll_p=cp;
		coll_pp=e1_s;
	}

	// edge on triangle edge
	/*for (int i=0;i<3;i++){
		// triangle edge
		vector te1,te2;
		if (i==0){
			te1=a;
			te2=b;
		}else if (i==1){
			te1=b;
			te2=c;
		}else{
			te1=c;
			te2=a;
		}

		// plane from edge (e1,e2) and (te1,te2)
		plane tepl;
		vector ten=VecCrossProduct(te2-te1,e_dir); // should point outward
		_vec_normalize_(ten);
		PlaneFromPointNormal(tepl,te1,ten);

		// distance
		d=-_plane_distance_(tepl,cp);
		//msg_write(f2s(d,2));
		if ((d>0)&&(d<d_min)){
			nearest=i+2;
			d_min=d;
			coll_n=ten;
			coll_p=cp+d*ten;
			coll_pp=cp;
		}
	}*/

	/*if (nearest>1)
		msg_write("zur Seite abgelenkt!");*/

	if (nearest>=0){
		set_col_type("tria edge");
		_add_col_(col,d_min,coll_n,coll_p);
	}
	msg_db_l(10);
	return (nearest>=0);
}

#endif


// triangle face - edge
inline void GetCollisionTriangleEdge(plane &pl,vector &a,vector &b,vector &c,vector &e1,vector &e2,Array<s_col_poly_data> &cut, int edge, int face)
{
	msg_db_r("GetCol TriaEdge",10);

	// edge intersecting plane? (different sides....)
	if (_plane_distance_(pl, e1) * _plane_distance_(pl, e2) > 0){
		msg_db_l(10);
		return;
	}
	
	// point of intersetion (plane - line)
	vector cp;
	if (!_plane_intersect_line_(cp,pl,e1,e2)){
		msg_db_l(10);
		return;
	}

	// on edge?
	/*if (!_vec_between_(cp,e1,e2)){
		msg_db_l(10);
		return;
	}*/

	// within triangle?
	float f,g;
	_get_bary_centric_(cp,pl,a,b,c,f,g);
	if ((f<0)||(g<0)||(f+g>1)){
		msg_db_l(10);
		return;
	}

	add_col_poly_data(cut, cp, 0, edge, face);
	
	msg_db_l(10);
}

// find the nearest point on the triangle boundary (a,b,c, normal n)
//   from a given point within a triangle in a direction
inline void _get_point_on_tria_boundary_(vector &cp,vector &dir,vector &n,vector &a,vector &b,vector &c,vector &bp,float &d)
{
	vector nxdir=n^dir;
	float da=(a-cp)*dir;
	float db=(b-cp)*dir;
	float dc=(c-cp)*dir;
	float nxda=(a-cp)*nxdir;
	float nxdb=(b-cp)*nxdir;
	float nxdc=(c-cp)*nxdir;

	vector e1,e2;

	// find the two edges "crossing" cp (in direction <n>)
	// then find the one more in direction <dir>
	if (nxda*nxdb>0){ // (b,c) and (c,a)
		e1=c;
		e2=(da>db)?a:b;
	}else if (nxdb*nxdc>0){ // (a,b) and (c,a)
		e1=a;
		e2=(db>dc)?b:c;
	}else{ // (a,b) and (b,c)
		e1=b;
		e2=(da>dc)?a:c;
	}

	_vec_normalize_(nxdir);
	plane pl;
	_plane_from_point_normal_(pl,cp,nxdir);
	_plane_intersect_line_(bp,pl,e1,e2);
	d=(bp-cp)*dir;
}


/*inline void _do_bad_ending_(vector &e1,vector &e2,vector &e_dir,vector &cp,plane &pl,vector &a,vector &b,vector &c,sCollisionData &col)
{
	// projection onto triangle plane
	vector n=-_get_normal_(pl);
	float depth=-_plane_distance_(pl,e1);
	vector p=e1-depth*n;
	vector pp=e1;
		
	vector p2;
	float dd;
	vector dir;
	if (p!=cp){
		dir = p - cp;
		_vec_normalize_(dir);
		if ((n!=dir)&&(n!=-dir)){
			_get_point_on_tria_boundary_(cp,dir,-n,a,b,c,p2,dd);
#if 0
			msg_write("test");
			msg_write(string2("(%f  %f  %f)",p2.x,p2.y,p2.z));
			msg_write(string2("(%f  %f  %f)",cp.x,cp.y,cp.z));
			msg_write(string2("(%f  %f  %f)",p.x,p.y,p.z));
			msg_write(string2("(%f  %f  %f)",n.x,n.y,n.z));
			msg_write(string2("(%f  %f  %f)",dir.x,dir.y,dir.z));
#endif
			if (_vec_between_(p2,cp,p)){
				//msg_write("between");
				pp=cp+(pp-cp)*_vec_factor_between_(p2,cp,p);
				p=p2;
				depth=_vec_length_(p-pp);
			}
		}
	}
	_add_col_(col,depth,n,pp,p);
}*/

inline void _do_bad_ending_(vector &e1,vector &e2,vector &e_dir,vector &cp,plane &pl,vector &a,vector &b,vector &c,CollisionData &col)
{
	msg_db_r("do bad ending",10);
	if (inf_pl(pl))
		HuiRaiseError("plane inf... bad ending");
	if (inf_v(e1))
		HuiRaiseError("e1 inf... bad ending");
	if (inf_v(e2))
		HuiRaiseError("e2 inf... bad ending");
	if (inf_v(e_dir))
		HuiRaiseError("e_dir inf... bad ending");
	if (inf_v(a))
		HuiRaiseError("a inf... bad ending");
	if (inf_v(b))
		HuiRaiseError("b inf... bad ending");
	if (inf_v(c))
		HuiRaiseError("c inf... bad ending");
	// the direct way (along the edge e1,e2)
	float depth=-_plane_distance_(pl,e1);//(cp-e1)*e_dir;
	vector n=-pl.n;
	//vector p=e1;
	//vector pp=cp;
	vector p=e1-depth*n;
	vector pp=e1;

	// through the triangle edge
	vector p2;
	float dd;
	vector dir;
	if (p!=cp){
		dir = p - cp;
		_vec_normalize_(dir);
		if ((n!=dir)&&(n!=-dir)){
			vector ni=-n;
			_get_point_on_tria_boundary_(cp,dir,ni,a,b,c,p2,dd);
#if 0
			msg_write("test");
			msg_write(string2("(%f  %f  %f)",p2.x,p2.y,p2.z));
			msg_write(string2("(%f  %f  %f)",cp.x,cp.y,cp.z));
			msg_write(string2("(%f  %f  %f)",p.x,p.y,p.z));
			msg_write(string2("(%f  %f  %f)",n.x,n.y,n.z));
			msg_write(string2("(%f  %f  %f)",dir.x,dir.y,dir.z));
#endif
			if (_vec_between_(p2,cp,p)){
				//msg_write("between");
				//msg_db_m("between",0);
				pp=cp+(pp-cp)*_vec_factor_between_(p2,cp,p);
				p=p2;
				depth=_vec_length_(p-pp);
				if (depth==0){
					//HuiRaiseError("depth==0    b");
				}
				n=(p-pp)/depth;
			}
		}
	}
	if (depth==0){
		//HuiRaiseError("depth==0    a");
		//msg_db_m("depth=0  a",0);
	}

	// through the triangle edge
	/*vector tp;
	for (int i=0;i<3;i++){
		vector l1,l2;
		if (i==0){
			l1=a;
			l2=b;
		}else if (i==1){
			l1=b;
			l2=c;
		}else{
			l1=c;
			l2=a;
		}
		_get_nearest_point_in_line_(l1,l2,cp,tp);
		if (_vec_between_(tp,l1,l2)){
			float d=_vec_length_(tp-cp);
			if (d<depth){
				depth=d;
				n = e_dir ^ (l2 - l1);
				_vec_normalize_(n);
				p=tp+n*d;
				pp=tp;
			}
		}
	}*/

	set_col_type("bad ending");
	_add_col_(col,depth,n,p);
	msg_db_l(10);
}

struct s_cp{
	vector p;
	int tria;
	float h;
};

static s_cp cp[128];

// edge - hull
#if 0
inline bool GetCollisionEdgeHull(vector &e1,vector &e2,sTriangleHull *hull,sCollisionData &col)
{
	msg_db_r("GetCol EdgeHull",10);

	int num_cps=0;

	// test all triangles for intersections
	for (int i=0;i<hull->NumTriangles;i++){
		// point of intersection
		vector t_cp;
		if (!_plane_intersect_line_(t_cp,hull->pl[i],e1,e2))
			continue;

		// on edge?
		if (!_vec_between_(t_cp,e1,e2))
			continue;

		// within triangle?
		float f,g;
		_get_bary_centric_(	t_cp,hull->pl[i],
							hull->p[hull->triangle_index[i*3  ]],
							hull->p[hull->triangle_index[i*3+1]],
							hull->p[hull->triangle_index[i*3+2]],f,g);
		if ((f<0)||(g<0)||(f+g>1))
			continue;

		// add intersection
		cp[num_cps].tria=i;
		cp[num_cps++].p=t_cp;
	}

	// any hit at all?
	if (num_cps==0){
		msg_db_l(10);
		return false;
	}

	// sort intersections
	vector e_dir=e2-e1;
	VecNormalize(e_dir,e_dir); // needed later...
	for (int i=0;i<num_cps;i++)
		cp[i].h=e_dir*cp[i].p;
	for (int i=0;i<num_cps;i++)
		for (int j=i+1;j<num_cps;j++)
			if (cp[i].h>cp[j].h){
				s_cp t=cp[i];
				cp[i]=cp[j];
				cp[j]=t;
			}
	

	// bad endings....
	if (e_dir*_get_normal_(hull->pl[cp[0].tria])>0){
		int tria=cp[0].tria;
		_do_bad_ending_(e1,e2,e_dir,cp[0].p,
						hull->pl[tria],
						hull->p[hull->triangle_index[tria*3  ]],
						hull->p[hull->triangle_index[tria*3+1]],
						hull->p[hull->triangle_index[tria*3+2]],col);

		for (int i=1;i<num_cps;i++)
			cp[i-1]=cp[i];
		num_cps--;
	}
	if ((num_cps%2)==1){
		int tria=cp[num_cps-1].tria;
		vector e_dir_i=-e_dir;
		_do_bad_ending_(e2,e1,e_dir_i,cp[num_cps-1].p,
						hull->pl[tria],
						hull->p[hull->triangle_index[tria*3  ]],
						hull->p[hull->triangle_index[tria*3+1]],
						hull->p[hull->triangle_index[tria*3+2]],col);

		num_cps--;
	}

	// pairs of intersections
	/*for (int i=0;i<num_cps;i+=2){
		// average of both normal vectors
		vector n1=_get_normal_(hull->pl[cp[i  ].tria]);
		vector n2=_get_normal_(hull->pl[cp[i+1].tria]);
		vector n= (n1-(e_dir*n1)*e_dir) + (n2-(e_dir*n2)*e_dir);
		VecNormalize(n,n);
		
		// find both triangles' edges in this direction
		float d1,d2;
		vector bp1,bp2;
		_get_point_on_tria_boundary_(	cp[i  ].p,n,n1,
										hull->p[hull->triangle_index[cp[i  ].tria*3  ]],
										hull->p[hull->triangle_index[cp[i  ].tria*3+1]],
										hull->p[hull->triangle_index[cp[i  ].tria*3+2]],
										bp1,d1);
		_get_point_on_tria_boundary_(	cp[i+1].p,n,n2,
										hull->p[hull->triangle_index[cp[i+1].tria*3  ]],
										hull->p[hull->triangle_index[cp[i+1].tria*3+1]],
										hull->p[hull->triangle_index[cp[i+1].tria*3+2]],
										bp2,d2);
		_add_col_(col,d1,-n,cp[i  ].p,bp1);
		_add_col_(col,d2,-n,cp[i+1].p,bp2);
	}*/

	msg_db_l(10);
	return false;
}
#endif



inline bool hull_vert_on_edge(TriangleHull *hull, int vert, int edge)
{
	return (hull->edge_index[edge*2] == hull->index[vert]) || (hull->edge_index[edge*2+1] == hull->index[vert]);
}

inline bool hull_edge_on_tria(TriangleHull *hull, int edge, int face)
{
	int e0 = hull->edge_index[edge * 2];
	int e1 = hull->edge_index[edge * 2 + 1];
	int a = hull->triangle_index[face * 3];
	int b = hull->triangle_index[face * 3 + 1];
	int c = hull->triangle_index[face * 3 + 2];
	if ((e0 != a) && (e0 != b) && (e0 != c))
		return false;
	if ((e1 != a) && (e1 != b) && (e1 != c))
		return false;
	return true;
}

inline int hull_trias_get_joining_edge(TriangleHull *hull, int t1, int t2)
{
	int t1a = hull->triangle_index[t1 * 3    ];
	int t1b = hull->triangle_index[t1 * 3 + 1];
	int t1c = hull->triangle_index[t1 * 3 + 2];
	int t2a = hull->triangle_index[t2 * 3    ];
	int t2b = hull->triangle_index[t2 * 3 + 1];
	int t2c = hull->triangle_index[t2 * 3 + 2];
	int e0, e1;
	bool b1a = ((t1a == t2a) || (t1a == t2b) || (t1a == t2c));
	bool b1b = ((t1b == t2a) || (t1b == t2b) || (t1b == t2c));
	bool b1c = ((t1c == t2a) || (t1c == t2b) || (t1c == t2c));
	if (b1a){
		e0 = t1a;
		if (b1b)
			e1 = t1b;
		else if (b1c)
			e1 = t1c;
		else
			return -1;
	}else if (b1b){
		e0 = t1b;
		if (b1c)
			e1 = t1c;
		else
			return -1;
	}else
		return -1;
	for (int i=0;i<hull->num_edges;i++){
		if ((hull->edge_index[i*2] == e0) && (hull->edge_index[i*2+1] == e1))
			return i;
		if ((hull->edge_index[i*2] == e1) && (hull->edge_index[i*2+1] == e0))
			return i;
	}
	return -1;
}

// (poly edge <-> hull triangle) <-> (poly face <-> hull edge)
// edge ends cuts hull only once
inline void process_hull_dat(	Array<s_col_poly_data> &ef, Array<s_col_poly_data> &fe,
								    ConvexPolyhedron *po,vector *vo,plane *plo,
									TriangleHull *hull,
    								CollisionData &col)
{
	msg_db_r("process_hull", 3);
	for (int i=ef.num-1;i>=0;i--){
		int edge = ef[i].edge;
		
		// other cuts on same poly-edge?
		int other_cut = -1;
		for (int j=ef.num-1;j>=0;j--)
			if ((ef[j].edge == edge) && (j != i)){
				other_cut = j;
				break;
			}
		if (other_cut >= 0){
			// both trias share an edge?
			int f1 = ef[i].face;
			int f2 = ef[other_cut].face;
			int e = hull_trias_get_joining_edge(hull, f1, f2);
			if (e >= 0){
				vector e0 = vo[po->edge_index[edge * 2]];
				vector e1 = vo[po->edge_index[edge * 2 + 1]];

				vector he_dir = hull->p[hull->edge_index[e * 2]] - hull->p[hull->edge_index[e * 2 + 1]];
				vector pe_dir = e0 - e1;
				vector n = he_dir ^ pe_dir;
				_vec_normalize_(n);
				float d = n * (e0 - hull->p[hull->edge_index[e * 2]]);
				if (d < 0){
					n = - n;
					d = - d;
				}
				if (n * hull->pl[f1].n > 0)
					continue;
				
				_add_col_(col, d, n, (ef[i].cp + ef[other_cut].cp) * 0.5f);
				ef.erase(i);
				if (other_cut > i)
					ef.erase(other_cut - 1);
				else{
					ef.erase(other_cut);
					i --;
				}
			}
			continue;
		}

		// distance to poly-edge end point?
		vector e0 = vo[po->edge_index[edge * 2]];
		vector e1 = vo[po->edge_index[edge * 2 + 1]];
		vector edge_n = - hull->pl[ef[i].face].n;
		float d0 = - _plane_distance_(hull->pl[ef[i].face], e0);
		float d1 = - _plane_distance_(hull->pl[ef[i].face], e1);
//		printf("fe  %f  %f\n", d0, d1);
		float edge_d = max(d0, d1);

		// do we cut a triangle-edge by lifting the poly-edge orthogonal to the triangle?
		float f, g;
		int tria = ef[i].face;
		vector ee = e0;
		if ((e0 - e1) * edge_n < 0)
			ee = e1;
		_get_bary_centric_(ee - edge_d * edge_n, hull->pl[tria],
		    			hull->p[hull->triangle_index[tria * 3]], hull->p[hull->triangle_index[tria * 3 + 1]], hull->p[hull->triangle_index[tria * 3 + 2]], f, g);
//		printf("%f %f\n", f, g);
		if ((f < 0) || (g < 0) || (f + g > 1)){
			// cutting the triangle...
			float d = edge_d;
			int para = -1;
			// find the nearest point ("same" poly-face and tria)
			for (int j=0;j<fe.num;j++)
				if (poly_edge_on_face(po, ef[i].edge, fe[j].face))
				    if (hull_edge_on_tria(hull, fe[j].edge, ef[i].face)){
						float para_d = _vec_length_fuzzy_(ef[i].cp - fe[j].cp);
						if (para_d < d){
//							printf("para\n");
							d = para_d;
							para = j;
						}
					}
			vector n;
			if (para < 0){
				//n = edge_n;
	//			printf("hull/poly... (T_T)\n");
				continue;
			}else{
				int he = fe[para].edge;
				vector he_dir = hull->p[hull->edge_index[he * 2]] - hull->p[hull->edge_index[he * 2 + 1]];
				vector pe_dir = e0 - e1;
				n = he_dir ^ pe_dir;
				_vec_normalize_(n);
				d = n * (e0 - hull->p[hull->edge_index[he * 2]]);
				if (d < 0){
					n = - n;
					d = - d;
				}
				fe.erase(para);
			}
			// add collision
			_add_col_(col, d, n, ef[i].cp);
		}else{
			_add_col_(col, edge_d, edge_n, ef[i].cp);
		}

		// delete...
		ef.erase(i);
	}
	msg_db_l(3);
}

// polyhedron - hull
inline bool GetCollisionPolyHull(ConvexPolyhedron *po,vector *vo,plane *plo,
                                 TriangleHull *hull,
                                 CollisionData &col)
{
	msg_db_r("GetCol PolyHull",10);
	bool hit=false;


	Array<s_col_poly_data> cut[4];


	// poly volume - hull vertex
	//    no!
	/*for (int i=0;i<hull->num_vertices;i++)
		GetCollisionPolyVertex(po,vo,plo,hull->p[hull->Index[i]],cut[0],i);*/

/*	// vertex - volume
	//    no!
	for (int i=0;i<po->num_vertices;i++)
		GetCollisionPolyVertex(pp,vp,plp,vo[po->vertex[i]],cut[1],i);*/

	//printf("%d\n", hull->num_edges);
	
	// poly face - hull edge
	for (int i=0;i<hull->num_edges;i++)
		GetCollisionPolyFacesEdge(po,vo,plo,hull->p[hull->edge_index[i*2]],hull->p[hull->edge_index[i*2+1]],cut[2],i);

	// edge - faces
	for (int i=0;i<po->num_edges;i++)
		for (int j=0;j<hull->num_triangles;j++)
			GetCollisionTriangleEdge(	hull->pl[j],
										hull->p[hull->triangle_index[j*3  ]], hull->p[hull->triangle_index[j*3+1]], hull->p[hull->triangle_index[j*3+2]],
										vo[po->edge_index[i*2  ]], vo[po->edge_index[i*2+1]], cut[3], i, j);

	hit = (cut[0].num > 0) || (cut[1].num > 0) || (cut[2].num > 0) || (cut[3].num > 0);

	

	// post process data...
//	printf("------\n%d  %d  %d  %d\n", cut[0].num, cut[1].num, cut[2].num, cut[3].num);
	process_hull_dat(cut[3], cut[2], po,vo,plo, hull, col);
//	printf("->  %d  %d  %d  %d\n", cut[0].num, cut[1].num, cut[2].num, cut[3].num);

	
#ifdef GARBAGE_AS_COLLISIONS
	// garbage
	for (int i=0;i<4;i++)
		for (int j=0;j<cut[i].num;j++)
			_add_col_(col, 0.001f, v_0, cut[i][j].cp);
#endif
	

	msg_db_l(10);
	return hit;
}

inline bool GetCollisionPhysHull(	PhysicalSkin *o,PhysicalSkinAbsolute *o_abs,
									TriangleHull *hull,
									CollisionData &col)
{
	msg_db_r("GetCol PhysHull",10);
	bool hit=false;

	for (int i=0;i<o->num_balls;i++)
		hit|=GetCollisionBallHull(&o->ball[i],o_abs->p,hull,col);

	for (int i=0;i<o->num_polys;i++)
		hit|=GetCollisionPolyHull(&o->poly[i],o_abs->p,&o_abs->pl[i*MODEL_MAX_POLY_FACES],hull,col);

	msg_db_l(10);
	return hit;
}



// model - model
bool CollideModels(CModel *m1, CModel *m2)
{
	msg_db_r("Collide mm",4);

	// reset ColData if we are in a top level call
	if (CollLevel == 0)
		pColData->num = 0;
	CollLevel ++;

	// own skeleton -> recursion
	if (m1->bone.num>0){
		for (int i=0;i<m1->bone.num;i++)
			if (m1->bone[i].model){
				if (!m1->bone[i].model->test_collisions)
					continue;
				CollideModels(m1->bone[i].model, m2);
			}
	}

	// partner skeleton -> recursion
	if (m2->bone.num>0){
		for (int i=0;i<m2->bone.num;i++)
			if (m2->bone[i].model){
				if (!m2->bone[i].model->test_collisions)
					continue;
				CollideModels(m1, m2->bone[i].model);
			}
	}
	


	// update world coordinates
	m1->_UpdatePhysAbsolute_();
	m2->_UpdatePhysAbsolute_();

	// initiate observers
	obs_ma=m1;
	obs_mb=m2;

	// direct
	GetCollisionPhysPhys(	m1->phys, &m1->phys_absolute,
							m2->phys, &m2->phys_absolute,
							*pColData);

	CollLevel --;
	msg_db_l(4);
	return (pColData->num > 0);
}

// model - hull
bool CollideModelTerrain(CModel *m, TriangleHull *hull)
{
	msg_db_r("Collide mt",4);

	// reset ColData if we are in a top level call
	if (CollLevel == 0)
		pColData->num = 0;
	CollLevel ++;
	
	// own skeleton -> recursion
	if (m->bone.num>0){
		for (int i=0;i<m->bone.num;i++)
			if (m->bone[i].model){
				if (!m->bone[i].model->test_collisions)
					continue;
				CollideModelTerrain(m->bone[i].model, hull);
			}
	}

	// update world coordinates
	m->_UpdatePhysAbsolute_();

	// initiate observers (not yet)
	obs_ma=NULL;
	obs_mb=NULL;

	// direct
	msg_db_m("phys hull...",4);
	GetCollisionPhysHull(m->phys, &m->phys_absolute, hull, *pColData);

	CollLevel --;
	msg_db_l(4);
	return (pColData->num > 0);
}



// object - object
bool CollideObjects(CObject *o1, CObject *o2)
{
	pColData->num = 0;

#if 0
	if (/*(o1->OnGround)&&(o2->partner->OnGround)&&*/(!o1->Moved)&&(!o2->Moved))
		return false;
#endif
	if ((o1->frozen) && (o2->frozen))
		return false;

	if ((!o1->test_collisions) || (!o2->test_collisions))
		return false;

	// physical ability to collide?
	if (((o1->active_physics) && (o2->passive_physics)) || ((o1->passive_physics) && (o2->active_physics))){

		msg_db_r("Coll object - object", 3);
		msg_db_m(o1->name.c_str(), 4);
		msg_db_m(o2->name.c_str(), 4);

		// too far away?
		if (!o1->pos.bounding_cube(o2->pos, o1->radius + o1->radius)){
			msg_db_l(3);
			return 0;
		}

		// Real-Real
		CollLevel = 0;
		pColData->o1 = o1;
		pColData->o2 = o2;
		CollideModels((CModel*)o1, (CModel*)o2);
		msg_db_l(3);

	}
#ifdef _X_ALLOW_PHYSICS_DEBUG_
	if (pColData->Num > 0)
		DebugAddCollision(*pColData);
#endif
	return (pColData->num > 0);
}

// Objekt - Modell
/*bool CollideObjects(CModel *partner,matrix *mat,matrix *mat_old,sCollisionData &col,bool set_crash)
{
	if (!partner)
		return 0;

	if ((!model->TestCollisions)||(!partner->TestCollisions))
		return 0;

	// physikalische Kollisionsfaehigkeit
	if ((ActivePhysics)||(PassivePhysics)){

		// zu weit von einander entfernt?
		vector mp=v_0;
		VecTransform(mp,*mat,mp);
		float d=Radius+partner->Diameter;
		if (!VecBoundingBox(Pos,mp,d))
			return false;

		// Real-Real
		col.Num=0;
		model->GetCollision(partner,col);

	}
	return col.Num;
}*/

#ifdef _X_ALLOW_TERRAIN_

TriangleHull temp_hull;

// Objekt - Terrain
bool CollideObjectTerrain(CObject *o, CTerrain *terrain)
{
	pColData->num = 0;
//	if ((!o->Moved))
	
	if (o->frozen)
		return false;


	// physikalische Kollisionsfaehigkeit
	if (o->active_physics){

		msg_db_r("Coll object - [Terrain]",3);
		msg_db_m(o->name.c_str(), 4);
	/*for (int i=0;i<16;i++)
		msg_write(f2s(Matrix.e[i],3));
	msg_write(string2("Pos:  %f %f %f",Pos.x,Pos.y,Pos.z));
	msg_write(string2("Ang:  %f %f %f",Ang.x,Ang.y,Ang.z));*/

		// zu weit von einander entfernt?
		/*if (!VecBoundingBox(Pos,partner->Pos,Radius+partner->Radius))
			return 0;*/

		terrain->GetTriangleHull(&temp_hull, o->pos, o->radius);
	/*for (int i=0;i<16;i++)
		msg_write(f2s(Matrix.e[i],3));
	msg_write(string2("Pos:  %f %f %f",Pos.x,Pos.y,Pos.z));
	msg_write(string2("Ang:  %f %f %f",Ang.x,Ang.y,Ang.z));*/

		CollLevel = 0;
		pColData->o1 = o;
		pColData->o2 = terrain_object;
		CollideModelTerrain((CModel*)o, &temp_hull);

		msg_db_l(3);
	}

#ifdef _X_ALLOW_PHYSICS_DEBUG_
	if (pColData->Num > 0)
		DebugAddCollision(*pColData);
#endif


	return (pColData->num > 0);
}
#endif
