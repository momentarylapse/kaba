/*----------------------------------------------------------------------------*\
| Model                                                                        |
| -> can be a skeleton                                                         |
|    -> sub-models                                                             |
|    -> animation data                                                         |
| -> model                                                                     |
|    -> vertex and triangle data for rendering                                 |
|    -> consists of 4 skins                                                    |
|       -> 0-2 = visible detail levels (LOD) 0=high detail                     |
|       -> 3   = dynamical (for animation)                                     |
|    -> seperate physical skin (vertices, balls and convex polyeders)          |
|       -> absolute vertex positions in a seperate structure                   |
| -> strict seperation:                                                        |
|    -> dynamical data (changed during use)                                    |
|    -> unique data (only one instance for several copied models)              |
| -> can contain effects (fire, light, water,...)                              |
|                                                                              |
| vital properties:                                                            |
|  - vertex buffers get filled temporaryly per frame                           |
|                                                                              |
| last update: 2008.10.26 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#include "x.h"





//#define MODEL_TEMP_VB
#define DynamicNormalCorrect


#define MODEL_MAX_EDGES	65536


static void VecOut(vector v)
{
	msg_write(format("vector(	%.2f, %.2f,	%.2f	);",v.x,v.y,v.z));
}

void MoveTimeAdd(Model *m,int operation_no,float elapsed,float v,bool loop);


int TraceHitType,TraceHitIndex,TraceHitSubModel,TraceHitSubModelTemp;




// make a copy of all the data
void CopySkinNew(Model *m, Skin *orig, Skin **copy)
{
	msg_db_r("CopySkinNew",1);
	(*copy) = new Skin;
	(**copy) = *orig;

	// vertices
	(*copy)->vertex.make_own();

	// bone indices
	(*copy)->bone_index.make_own();

	// subs
	(*copy)->sub.make_own();
	for (int mm=0;mm<orig->sub.num;mm++){
		SubSkin *sub = &(*copy)->sub[mm];

		// copy data
		sub->triangle_index.make_own();
		sub->normal.make_own();
		sub->skin_vertex.make_own();
		
		// reset the vertex buffers
		(*copy)->sub[mm].vertex_buffer = -1;
		(*copy)->sub[mm].force_update = true;
	}
	(*copy)->copy_as_ref = false;
	msg_db_l(1);
}

// create a new skin but reference the data
void CopySkinAsReference(Model *m, Skin *orig, Skin **copy)
{
	msg_db_r("CopySkin(ref)",1);
	(*copy) = new Skin;
	(**copy) = *orig;
	
	// subs
	(*copy)->sub.make_own();
	
	// reset the vertex buffers
	for (int i=0;i<orig->sub.num;i++){
		(*copy)->sub[i].vertex_buffer = -1;
		(*copy)->sub[i].force_update = true;
	}
	(*copy)->copy_as_ref = true;
	// no     subs are referenced... so vertex buffer is also referenced...????
	msg_db_l(1);
}

// update physical data in world coordinates
void Model::_UpdatePhysAbsolute_()
{
	msg_db_r("CalcPhysAbs",1);
	PhysicalSkinAbsolute *a = &phys_absolute;
	PhysicalSkin *s = phys;


	/*bool nn=true;
	for (int i=0;i<16;i++){
		msg_write(f2s(m.e[i],3));
		nn &= ( m.e[i] == 0 );
	}
	if (nn){
		HuiRaiseError("null-matrix...kille");
	}*/


	if (!a->is_ok){
		msg_db_m("...",1);
		if (!a->p)
			a->p = new vector[s->num_vertices];
		if (!a->pl)
			a->pl = new plane[s->num_polys * MODEL_MAX_POLY_FACES];
		// transform vertices
		for (int i=0;i<s->num_vertices;i++)
			//VecTransform(a->p[i],Matrix,s->Vertex[i]);
			a->p[i] = _matrix * s->vertex[i];
		// convex polyhedron
		for (int i=0;i<s->num_polys;i++)
			for (int k=0;k<s->poly[i].num_faces;k++)
				PlaneTransform(a->pl[i * MODEL_MAX_POLY_FACES+k], _matrix, s->poly[i].face[k].pl);
		a->is_ok = true;
	}
	
	// doesn't need to be recursive... already done by GetCollision()!
	/*for (int i=0;i<Bone.num;i++)
		if (BoneModel[i]){
			MatrixMultiply(BoneMatrix[i],Matrix,BoneDMatrix[i]);
			BoneModel[i]->_UpdatePhysAbsolute_(BoneMatrix[i]);
		}*/
	msg_db_l(1);
}

// mark data as obsolete
void Model::_ResetPhysAbsolute_()
{
	msg_db_r("ResetPhysAbsolute",1);
	phys_absolute.is_ok=false;
	for (int i=0;i<bone.num;i++)
		if (bone[i].model)
			bone[i].model->_ResetPhysAbsolute_();
	msg_db_l(1);
}

//--------------------------------------------------------------------------------------------------
// hopefully these functions will be obsolete with the next fileformat

// how big is the model
void AppraiseDimensions(Model *m)
{
	float rad = 0;
	
	// bounding box (visual skin[0])
	m->min = m->max = v_0;
	for (int i=0;i<m->skin[0]->vertex.num;i++){
		m->min._min(m->skin[0]->vertex[i]);
		m->max._max(m->skin[0]->vertex[i]);
		float r = _vec_length_fuzzy_(m->skin[0]->vertex[i]);
		if (r > rad)
			rad = r;
	}

	// physical skin
	for (int i=0;i<m->phys->num_vertices;i++){
		float r = _vec_length_fuzzy_(m->phys->vertex[i]);
		if (r > rad)
			rad = r;
	}
	for (int i=0;i<m->phys->num_balls;i++){
		float r = _vec_length_fuzzy_(m->phys->vertex[m->phys->ball[i].index]) + m->phys->ball[i].radius;
		if (r > rad)
			rad = r;
	}
	m->radius = rad;
}

// make sure we have enough vertex buffers
void CreateVB(Model *m, Skin *s)
{
#ifdef MODEL_TEMP_VB
	return;
#else
	for (int t=0;t<s->sub.num;t++){
		if (s->sub[t].vertex_buffer < 0){
			if (m->material[t].num_textures == 1)
				s->sub[t].vertex_buffer = NixCreateVB(s->sub[t].num_triangles);
			else
				s->sub[t].vertex_buffer = NixCreateVBM(s->sub[t].num_triangles, m->material[t].num_textures);
		}
		s->sub[t].force_update = true;
	}
#endif
}


void PostProcessSkin(Model *m, Skin *s)
{

	CreateVB(m,s);

	// bounding box
	s->min = s->max = v_0;
	if (s->vertex.num > 0){
		s->min = s->max = s->vertex[0];
		for (int i=0;i<s->vertex.num;i++){
			s->min._min(s->vertex[i]);
			s->max._max(s->vertex[i]);
		}
	}
}

void PostProcessPhys(Model *m, PhysicalSkin *s)
{
	m->phys_absolute.p = NULL;
	m->phys_absolute.pl = NULL;
	m->_ResetPhysAbsolute_();
}

color file_read_color(CFile *f)
{
	int a=f->ReadInt();
	int r=f->ReadInt();
	int g=f->ReadInt();
	int b=f->ReadInt();
	return color((float)a/255.0f,(float)r/255.0f,(float)g/255.0f,(float)b/255.0f);
}

void ApplyMaterial(Model *model, Material *m, Material *m2, bool user_colors)
{
	if (!user_colors){
		m->ambient = m2->ambient;
		m->diffuse = m2->diffuse;
		m->specular = m2->specular;
		m->emission = m2->emission;
		m->shininess = m2->shininess;
	}
	int nt = m->num_textures;
	if (nt > m2->num_textures)
		nt = m2->num_textures;
	for (int i=0;i<nt;i++)
		if (m->texture[i] < 0)
			m->texture[i] = m2->texture[i];
	if (m->transparency_mode == TransparencyModeDefault){
		m->transparency_mode = m2->transparency_mode;
		m->alpha_source = m2->alpha_source;
		m->alpha_destination = m2->alpha_destination;
		m->alpha_factor = m2->alpha_factor;
		m->alpha_z_buffer = m2->alpha_z_buffer;
	}
	m->reflection_mode = m2->reflection_mode;
	m->reflection_density = m2->reflection_density;
	m->cube_map = m2->cube_map;
#ifdef _X_ALLOW_FX_
	if ((m->cube_map < 0) && (m2->cube_map_size > 0) && (m->reflection_mode == ReflectionCubeMapDynamical)){
		m->cube_map = FxCubeMapNew(m2->cube_map_size);
		FxCubeMapCreate(m->cube_map, model);
	}
#endif
	m->shader = m2->shader;
	m->rc_static = m2->rc_static;
	m->rc_sliding = m2->rc_sliding;
	m->rc_jump = m2->rc_jump;
	m->rc_rolling = m2->rc_rolling;
}


static vector get_normal_by_index(int index)
{
	float wz = (float)(index >> 8) * pi / 255.0f;
	float wxy = (float)(index & 255) * 2 * pi / 255.0f;
	float swz = sin(wz);
	if (swz < 0)
		swz = - swz;
	float cwz = cos(wz);
	return vector( cos(wxy) * swz, sin(wxy) * swz, cwz);
}

void Model::ResetData()
{
	msg_db_r("ResetData", 2);
	registered = false;
	object_id = -1;
	script_data = NULL;
	
	for (int i=0;i<MODEL_NUM_SKINS;i++){
		_detail_needed_[i] = false;
		vertex_dyn[i] = NULL;
		normal_dyn[i] = NULL;
	}

	// "auto-animate"
	num_move_operations = -1;
	move_operation[0].move = 0;
	move_operation[0].time = 0;
	move_operation[0].operation = MoveOpSet;
	move_operation[0].param1 = 0;
	move_operation[0].param2 = 0;
	if (meta_move){
		for (int i=0;i<meta_move->num_moves;i++)
			if (meta_move->move[i].num_frames > 0){
				move_operation[0].move = i;
				break;
			}
	}

	if (active_physics)
		mass_inv = 1.0f / mass;
	else
		mass_inv = 0;
	theta = theta_0;
	g_factor = 1;
	test_collisions = true;
	allow_shadow = false;

	// fx
	fx.resize(_template->fx.num);
	foreachi(ModelEffectData &tf, _template->fx, i){
		if (tf.type == FXTypeLight)
			fx[i] = FxCreateLight(this, tf.vertex, tf.radius, tf.am, tf.di, tf.sp);
		else if (tf.type == FXTypeSound)
			fx[i] = FxCreateSound(this, tf.vertex, tf.filename, tf.radius, tf.speed);
		else if (tf.type == FXTypeScript)
			fx[i] = FxCreateScript(this, tf.vertex, tf.filename);
	}

	// script vars
	script_var = _template->script_var;

	// inventary
	inventary.resize(_template->item.num);
	for (int i=0;i<_template->item.num;i++)
		inventary[i] = MetaLoadModel(_template->item[i]);

	if (material.num > 0)
		SetMaterial(&material[0], SetMaterialFriction);

#ifdef _X_ALLOW_ODE_
	geom_id = 0;
	body_id = 0;
#endif
	
	vel = rot = v_0;
	msg_db_l(2);
}

void read_color(CFile *f, color &c)
{
	c.a = (float)f->ReadInt() / 255.0f;
	c.r = (float)f->ReadInt() / 255.0f;
	c.g = (float)f->ReadInt() / 255.0f;
	c.b = (float)f->ReadInt() / 255.0f;
}

void Model::reset()
{
	error = false;
	pos = ang = vel = rot = v_0;
	object_id = -1;
	on_ground = false;
	visible = true;
	rotating  = true;
	moved = false;
	frozen = false;
	time_till_freeze = 0;
	ground_id = -1;
	ground_normal = v_0;
	_detail_ = -1;


	vel_surf = acc = v_0;
	force_int = torque_int = v_0;
	force_ext = torque_ext = v_0;
}

// completely load an original model (all data is its own)
Model::Model(const string &filename)
{
	msg_db_r("loading model", 1);
	reset();
	msg_write("loading model: " + filename);
	msg_right();

	// load model from file
	CFile *f = OpenFile(ObjectDir + filename + ".model");
	if (!f){
		error = true;
		msg_error("-failed");
		msg_left();
		msg_db_l(1);
		return;
	}
	int ffv = f->ReadFileFormatVersion();
	if (ffv != 11){
		FileClose(f);
		error = true;
		msg_error(format("wrong file format: %d (11 expected)", ffv));
		msg_left();
		msg_db_l(1);
		return;
	}

	Array<float> temp_sv;
	_template = new ModelTemplate;
	_template->reset();
	_template->filename = filename;

// file format 11...
	// General
	f->ReadComment();
	// bounding box
	f->ReadVector(&min);
	f->ReadVector(&max);
	// skins
	f->ReadInt();
	// reserved
	f->ReadInt();
	f->ReadInt();
	f->ReadInt();

	// Materials
	int num_materials = f->ReadIntC();
	material.resize(num_materials);
	material_is_reference = false;
	for (int i=0;i<material.num;i++){
		Material *m = &material[i];
		Material *mat_from_file = MetaLoadMaterial(f->ReadStr());
		bool user_colors = f->ReadBool();
		m->ambient = file_read_color(f);
		m->diffuse = file_read_color(f);
		m->specular = file_read_color(f);
		m->emission = file_read_color(f);
		m->shininess = (float)f->ReadInt();
		m->transparency_mode = f->ReadInt();
		m->alpha_source = f->ReadInt();
		m->alpha_destination = f->ReadInt();
		m->alpha_factor = (float)f->ReadInt() * 0.01f;
		m->alpha_z_buffer = f->ReadBool();
		m->num_textures = f->ReadInt();
		for (int t=0;t<m->num_textures;t++)
			m->texture[t] = NixLoadTexture(f->ReadStr());
		ApplyMaterial(this, m, mat_from_file, user_colors);
	}
	
	// Physical Skin
	msg_db_m("Phys",1);
	phys = new PhysicalSkin;
	phys_is_reference = false;
	//   vertices
	phys->num_vertices = f->ReadIntC();
	phys->bone_nr = new int[phys->num_vertices];
	phys->vertex = new vector[phys->num_vertices];
	for (int i=0;i<phys->num_vertices;i++)
		phys->bone_nr[i] = f->ReadInt();
	for (int i=0;i<phys->num_vertices;i++)
		f->ReadVector(&phys->vertex[i]);
	//   triangles
	f->ReadInt();
	//   balls
	phys->num_balls = f->ReadInt();
	phys->ball = new Ball[phys->num_balls];
	for (int i=0;i<phys->num_balls;i++){
		phys->ball[i].index = f->ReadInt();
		phys->ball[i].radius = f->ReadFloat();
	}
	//   convex polyhedron
	phys->num_polys = f->ReadInt();
	phys->poly = new ConvexPolyhedron[phys->num_polys];
	for (int i=0;i<phys->num_polys;i++){
		ConvexPolyhedron *p = &phys->poly[i];
		p->num_faces = f->ReadInt();
		for (int j=0;j<p->num_faces;j++){
			p->face[j].num_vertices = f->ReadInt();
			for (int k=0;k<p->face[j].num_vertices;k++)
				p->face[j].index[k] = f->ReadInt();
			p->face[j].pl.n.x = f->ReadFloat();
			p->face[j].pl.n.y = f->ReadFloat();
			p->face[j].pl.n.z = f->ReadFloat();
			p->face[j].pl.d = f->ReadFloat();
		}
		// non redundand stuff
		p->num_vertices = f->ReadInt();
		p->vertex = new int[p->num_vertices];
		for (int k=0;k<p->num_vertices;k++)
			p->vertex[k] = f->ReadInt();
		p->num_edges = f->ReadInt();
		p->edge_index = new int[p->num_edges * 2];
		for (int k=0;k<p->num_edges*2;k++)
			p->edge_index[k] = f->ReadInt();
		// topology
		p->faces_joining_edge = new int[p->num_faces * p->num_faces];
		for (int k=0;k<p->num_faces;k++)
			for (int l=0;l<p->num_faces;l++)
				p->faces_joining_edge[k * p->num_faces + l] = f->ReadInt();
		p->edge_on_face = new bool[p->num_edges * p->num_faces];
		for (int k=0;k<p->num_edges;k++)
			for (int l=0;l<p->num_faces;l++)
			    p->edge_on_face[k * p->num_faces + l] = f->ReadBool();
	}

	// Visible Skin[d]
	for (int d=0;d<3;d++){
		msg_db_m("Skin",1);
		skin[d] = new Skin;
		Skin *s = skin[d];
		s->sub.resize(material.num);
		skin_is_reference[d] = false;
		s->copy_as_ref = false;

		// vertices
		int n_vert = f->ReadIntC();
		s->vertex.resize(n_vert);
		s->bone_index.resize(n_vert);
		for (int i=0;i<s->vertex.num;i++)
			f->ReadVector(&s->vertex[i]);
		for (int i=0;i<s->vertex.num;i++)
			s->bone_index[i] = f->ReadInt();

		// skin vertices
		int NumSkinVertices = f->ReadInt();
		temp_sv.resize(NumSkinVertices * 2);
		for (int i=0;i<NumSkinVertices * 2;i++)
			temp_sv[i] = f->ReadFloat();

		// sub skins
		for (int m=0;m<material.num;m++){
			SubSkin *sub = &s->sub[m];
			// triangles
			sub->num_triangles = f->ReadInt();
			sub->triangle_index.resize(sub->num_triangles * 3);
			sub->skin_vertex.resize(material[m].num_textures * sub->num_triangles * 6);
			sub->normal.resize(sub->num_triangles * 3);
			// vertices
			for (int i=0;i<sub->num_triangles * 3;i++)
				sub->triangle_index[i] = f->ReadInt();
			// skin vertices
			for (int i=0;i<material[m].num_textures * sub->num_triangles * 3;i++){
				int sv = f->ReadInt();
				sub->skin_vertex[i * 2    ] = temp_sv[sv * 2    ];
				sub->skin_vertex[i * 2 + 1] = temp_sv[sv * 2 + 1];
			}
			// normals
			for (int i=0;i<sub->num_triangles * 3;i++)
				sub->normal[i] = get_normal_by_index(f->ReadInt());

			f->ReadInt();
			sub->force_update = true;
			sub->vertex_buffer = -1;
		}
		f->ReadInt();
	}

	// Skeleton
	msg_db_m("Skel",1);
	bone.resize(f->ReadIntC());
	for (int i=0;i<bone.num;i++){
		f->ReadVector(&bone[i].pos);
		bone[i].parent = f->ReadInt();
		bone[i].model = MetaLoadModel(f->ReadStr());
	}

	// Animations
	int num_anims_all = f->ReadIntC();
	int num_anims = f->ReadInt();
	int num_frames_vert = f->ReadInt();
	int num_frames_skel = f->ReadInt();
	// animated?
	meta_move = NULL;
	if (num_anims_all > 0){
		meta_move = new MetaMove;
		meta_move->reset();
		meta_move->num_frames_skeleton = num_frames_skel;
		meta_move->num_frames_vertex = num_frames_vert;

		meta_move->move = new Move[num_anims_all];
		if (num_frames_vert > 0)
			for (int i=0;i<4;i++){
				int n_vert = 0;
				if (phys)
					n_vert = phys->num_vertices;
				if (i > 0)
					n_vert = skin[i - 1]->vertex.num;
				meta_move->skin[i].dpos = new vector[num_frames_vert * n_vert];
				memset(meta_move->skin[i].dpos, 0, sizeof(vector) * num_frames_vert * n_vert);
			}
		if (num_frames_skel > 0){
			meta_move->skel_dpos = new vector[num_frames_skel * bone.num];
			meta_move->skel_ang = new quaternion[num_frames_skel * bone.num];
		}
		int frame_s = 0, frame_v = 0;

		// moves
		for (int i=0;i<num_anims;i++){
			int index = f->ReadInt();
			
			// auto animation: use first move!
			if (i==0)
				move_operation[0].move = index;
			Move *m = &meta_move->move[index];
			f->ReadStr(); // name is irrelevant
			m->type = f->ReadInt();
			m->num_frames = f->ReadInt();
			m->frames_per_sec_const = f->ReadFloat();
			m->frames_per_sec_factor = f->ReadFloat();
			
			if (m->type == MoveTypeVertex){
				m->frame0 = frame_v;
				for (int fr=0;fr<m->num_frames;fr++){
					for (int s=0;s<4;s++){
						// s=-1: Phys
						int np = phys->num_vertices;
						if (s >= 1)
							np = skin[s - 1]->vertex.num;
						int num_vertices = f->ReadInt();
						for (int j=0;j<num_vertices;j++){
							int vertex_index = f->ReadInt();
							f->ReadVector(&meta_move->skin[s].dpos[frame_v * np + vertex_index]);
						}
					}
					frame_v ++;
				}
			}else if (m->type == MoveTypeSkeletal){
				m->frame0 = frame_s;
				bool *free_pos = new bool[bone.num];
				for (int j=0;j<bone.num;j++)
					free_pos[j] = f->ReadBool();
				m->inter_quad = f->ReadBool();
				m->inter_loop = f->ReadBool();
				for (int fr=0;fr<m->num_frames;fr++){
					for (int j=0;j<bone.num;j++){
						vector v;
						f->ReadVector(&v);
						QuaternionRotationV(meta_move->skel_ang[frame_s * bone.num + j], v);
						if (free_pos[j])
							f->ReadVector(&meta_move->skel_dpos[frame_s * bone.num + j]);
					}
					frame_s ++;
				}
				delete[](free_pos);
			}
		}
	}
	temp_sv.clear();

	// Effects
	msg_db_m("FX",1);
	int num_fx = f->ReadIntC();
	msg_db_m(i2s(num_fx).c_str(),2);
	_template->fx.resize(num_fx);
	for (int i=0;i<num_fx;i++){
		ModelEffectData *d = &_template->fx[i];
		string fxtype = f->ReadStr();
		msg_db_m(fxtype.c_str(),2);
		if (fxtype == "Script"){
			d->type = FXTypeScript;
			d->vertex = f->ReadInt();
			d->filename = f->ReadStr();
			f->ReadStr();
		}else if (fxtype == "Light"){
			d->type = FXTypeLight;
			d->vertex = f->ReadInt();
			d->radius = f->ReadFloat();
			read_color(f, d->am);
			read_color(f, d->di);
			read_color(f, d->sp);
		}else if (fxtype == "Sound"){
			d->type=FXTypeSound;
			d->vertex=f->ReadInt();
			d->radius = (float)f->ReadInt();
			d->speed = (float)f->ReadInt()*0.01f;
			d->filename = f->ReadStr();
		}else if (fxtype == "ForceField"){
			d->type =FXTypeForceField;
			f->ReadInt();
			(float)f->ReadInt();
			(float)f->ReadInt();
			f->ReadBool();
		}else
			msg_error("unknown effect: " + fxtype);
	}

	// Physics
	mass = f->ReadFloatC();
	for (int i=0;i<9;i++)
		theta_0.e[i] = f->ReadFloat();
	active_physics = f->ReadBool();
	passive_physics = f->ReadBool();
	radius = f->ReadFloat();

	// LOD-Distances"
	detail_dist[SkinHigh] = f->ReadFloatC();
	detail_dist[SkinMedium] = f->ReadFloat();
	detail_dist[SkinLow] = f->ReadFloat();
	

// object data
	// Object Data
	name = f->ReadStrC();
	description = f->ReadStr();

	// Inventary
	_template->item.resize(f->ReadIntC());
	for (int i=0;i<_template->item.num;i++){
		_template->item[i] = f->ReadStr();
		f->ReadInt();
	}

	// Script
	_template->script_filename = f->ReadStrC();
	_template->script_var.resize(f->ReadInt());
	for (int i=0;i<_template->script_var.num;i++)
		_template->script_var[i] = f->ReadFloat();

	msg_db_m("deleting file",1);
	FileClose(f);



	// do some post processing...
	msg_db_m("-calculating model",2);
	AppraiseDimensions(this);

	for (int i=0;i<MODEL_NUM_SKINS;i++)
		PostProcessSkin(this, skin[i]);

	PostProcessPhys(this, phys);




	// skeleton
	if (bone.num>0){
		msg_db_m("-calculating skeleton",2);
		bone_pos_0 = new vector[bone.num];
		for (int i=0;i<bone.num;i++){
			bone_pos_0[i] = _GetBonePos(i);
			MatrixTranslation(bone[i].dmatrix, bone_pos_0[i]);
		}
	}
	


	is_copy = false;
	ResetData();

	msg_left();
	msg_db_l(1);
}


// only used by GetCopy() !!!
Model::Model()
{
	_template = (ModelTemplate*)0x42;
}

void Model::SetMaterial(Material *m, int mode)
{
	if ((mode & SetMaterialFriction)>0){
		rc_jump = m->rc_jump;
		rc_sliding = m->rc_sliding;
		rc_rolling = m->rc_rolling;
		rc_static = m->rc_static;
	}
}

void CopyPhysicalSkin(PhysicalSkin *orig, PhysicalSkin **copy)
{
	(*copy) = new PhysicalSkin;
	(**copy) = (*orig);
}

Model *Model::GetCopy(int mode)
{
	msg_db_r("model::GetCopy",1);

	if (is_copy)
		msg_error("model: copy of copy");


	Model *m = new Model();
	//memcpy(m, this, sizeof(Model));
	*m = *this;

	//Fx.forget(); ...
	

	// "copy" presettings (just using references)
	m->is_copy = true;
	for (int i=0;i<MODEL_NUM_SKINS;i++){
		m->skin_is_reference[i] = true;
		m->_detail_needed_[i] = false;
		m->vertex_dyn[i] = NULL;
		m->normal_dyn[i] = NULL;
	}
	m->phys_is_reference = true;
	m->material_is_reference = true;
	m->registered = false;
	m->visible = true;

	// skins
	if ((mode & ModelCopyChangeable) > 0){
		msg_db_m("-new skins",2);
		for (int i=0;i<MODEL_NUM_SKINS;i++){
			CopySkinNew(this, skin[i], &m->skin[i]);
			m->skin_is_reference[i] = false;
		}
		CopyPhysicalSkin(phys, &m->phys);
		m->phys_is_reference = false;
	}else if ((meta_move) || (bone.num > 0)){
		msg_db_m("-new skins ref",2);
		for (int i=0;i<MODEL_NUM_SKINS;i++){
			CopySkinAsReference(this, skin[i], &m->skin[i]);
			m->skin_is_reference[i] = false;
		}
	}

	// skeleton
	if (bone.num > 0){
		msg_db_m("-new skeleton",2);
		m->bone_pos_0 = new vector[bone.num];
		memcpy(m->bone_pos_0, bone_pos_0, sizeof(vector) * bone.num);
		// copy already done by "*m = *this"...
		for (int i=0;i<bone.num;i++){
			if (((mode & ModelCopyRecursive) > 0) && bone[i].model)
				m->bone[i].model = MetaCopyModel(bone[i].model);
			else if ((mode & ModelCopyKeepSubModels) > 0)
				m->bone[i].model = bone[i].model;
			else
				m->bone[i].model = NULL;
		}
	}

	// effects
	// loaded by ResetData()

	m->_ResetPhysAbsolute_();
	m->phys_absolute.p = NULL;
	m->phys_absolute.pl = NULL;



	// reset
	if ((mode & ModelCopyInverse) > 0)
		ResetData();
	else
		m->ResetData();

#ifdef _X_DEBUG_MODEL_MANAGER_
	msg_write(p2s(Template));
	msg_write(p2s(m->Template));
#endif

	msg_db_l(1);
	return m;
}

// delete only the data owned by this model
//    don't delete sub models ...done by meta
Model::~Model()
{
	msg_db_r("~model",1);
#ifdef _X_DEBUG_MODEL_MANAGER_
	msg_write(p2s(this));
	msg_write(isCopy);
#endif

	// own data
	for (int i=0;i<MODEL_NUM_SKINS;i++){
		if (vertex_dyn[i])
			delete[](vertex_dyn[i]);
		if (normal_dyn[i])
			delete[](normal_dyn[i]);
	}

	// animation
	if ((meta_move) && (!is_copy)){
		if (meta_move->skel_dpos)
			delete[](meta_move->skel_dpos);
		if (meta_move->skel_ang)
			delete[](meta_move->skel_ang);
		for (int i=0;i<4;i++)
			if (meta_move->skin[i].dpos)
				delete[](meta_move->skin[i].dpos);
		delete[](meta_move->move);
		delete(meta_move);
	}

	// physical
	if (!phys_is_reference){
		msg_db_m("del phys", 2);
		delete[](phys->bone_nr);
		delete[](phys->vertex);
		for (int i=0;i<phys->num_polys;i++){
			delete[](phys->poly[i].vertex);
			delete[](phys->poly[i].edge_index);
			delete[](phys->poly[i].edge_on_face);
			delete[](phys->poly[i].faces_joining_edge);
		}
		delete[](phys->poly);
		delete[](phys->ball);
		delete(phys);
	}
	if (phys_absolute.p)
		delete[](phys_absolute.p);
	if (phys_absolute.pl)
		delete[](phys_absolute.pl);

	// skin
	for (int i=0;i<MODEL_NUM_SKINS;i++)
		if (!skin_is_reference[i]){
			msg_db_m(format("del skin[%d]",i).c_str(), 2);
			Skin *s = skin[i];

			// vertex buffer
			for (int t=0;t<s->sub.num;t++)
				if (s->sub[t].vertex_buffer >= 0)
					NixDeleteVB(s->sub[t].vertex_buffer);

			if (s->copy_as_ref){
				// referenced data... don't really delete... but "unlink"
				s->vertex.forget();
				s->bone_index.forget();
				for (int t=0;t<s->sub.num;t++){
					s->sub[t].triangle_index.forget();
					s->sub[t].normal.forget();
					s->sub[t].skin_vertex.forget();
				}
			}else{
				s->vertex.clear();
				s->bone_index.clear();
				for (int t=0;t<s->sub.num;t++){
					s->sub[t].triangle_index.clear();
					s->sub[t].normal.clear();
					s->sub[t].skin_vertex.clear();
				}
			}
			s->sub.clear();

			// own / own data
			delete(skin[i]);
		}

	// skeletton
	if (bone.num > 0){
		bone.clear();
		delete[](bone_pos_0);
	}

	// material
	if (material_is_reference)
		material.forget();
	else
		material.clear();

	// fx
	for (int i=0;i<fx.num;i++)
		if (fx[i])
			FxDelete(fx[i]);
	fx.clear();

	script_var.clear();
	inventary.clear();

	// template
	if (!is_copy){
		_template->script_var.clear();
		for (int i=0;i<_template->item.num;i++)
			_template->item[i].clear();
		_template->item.clear();
		delete(_template);
	}
	
	msg_db_l(1);
}

// non-animated state
vector Model::_GetBonePos(int index)
{
	int r = bone[index].parent;
	if (r < 0)
		return bone[index].pos;
	return bone[index].pos + _GetBonePos(r);
}

int get_num_trias(Skin *s)
{
	int n = 0;
	for (int i=0;i<s->sub.num;i++)
		n += s->sub[i].num_triangles;
	return n;
}

void Model::SetBoneModel(int index, Model *sub)
{
	if ((index < 0) || (index >= bone.num)){
		msg_error(format("Model::SetBoneModel: (%s) invalid bone index %d", GetFilename().c_str(), index));
		return;
	}
	msg_db_r("Model::SetBoneModel",2);
	
	// remove the old one
#ifdef _X_ALLOW_GOD_
	if (bone[index].model)
		if (registered)
			GodUnregisterModel(bone[index].model);
#endif

	// add the new one
	bone[index].model = sub;
#ifdef _X_ALLOW_GOD_
	if (sub)
		if (registered)
			GodRegisterModel(sub);
#endif
	msg_db_l(2);
}

void Model::CalcMove()
{
	if (TimeScale == 0)
		return;

	msg_db_r("model::CalcMove",3);

#ifdef _X_ALLOW_MODEL_ANIMATION_
	if (meta_move){

		msg_db_m("meta_move",10);

		// for handling special cases (-1,-2)
		int num_ops = num_move_operations;

		// "auto-animated"
		if (num_move_operations == -1){
			// default: just run a single animation
			MoveTimeAdd(this, 0, Elapsed, 0, true);
			move_operation[0].operation = MoveOpSet;
			move_operation[0].move = 0;
			num_ops = 1;
		}

		// skeleton edited by script...
		if (num_move_operations == -2){
			num_ops = 0;
		}

		// make sure we have something to store the animated data in...
		for (int s=0;s<MODEL_NUM_SKINS;s++)
			if (_detail_needed_[s]){
				if (vertex_dyn[s] == NULL)
					vertex_dyn[s] = new vector[skin[s]->vertex.num];
				//memset(vertex_dyn[s], 0, sizeof(vector) * Skin[s]->vertex.num);
				memcpy(vertex_dyn[s], &skin[s]->vertex[0], sizeof(vector) * skin[s]->vertex.num);
				int nt = get_num_trias(skin[s]);
				if (normal_dyn[s] == NULL)
					normal_dyn[s] = new vector[nt * 3];
				int offset = 0;
				for (int i=0;i<material.num;i++){
					memcpy(&normal_dyn[s][offset], &skin[s]->sub[i].normal[0], sizeof(vector) * skin[s]->sub[i].num_triangles * 3);
					offset += skin[s]->sub[i].num_triangles;
				}
			}


	// vertex animation

		msg_db_m("vertex anim",10);

		bool vertex_animated=false;
		for (int op=0;op<num_ops;op++){
			if (move_operation[op].move<0)	continue;
			Move *m = &meta_move->move[move_operation[op].move];
			if (m->num_frames == 0)
				continue;


			if (m->type == MoveTypeVertex){
				vertex_animated=true;

				// frame data
				int fr=(int)(move_operation[op].time); // current frame (relative)
				float dt=move_operation[op].time-(float)fr;
				int f1 = m->frame0 + fr; // current frame (absolute)
				int f2 = m->frame0 + (fr+1)%m->num_frames; // next frame (absolute)

				// transform vertices
				for (int s=0;s<MODEL_NUM_SKINS;s++)
					if (_detail_needed_[s]){
						Skin *sk = skin[s];
						for (int p=0;p<sk->vertex.num;p++){
							vector dp1 = meta_move->skin[s + 1].dpos[f1 * sk->vertex.num + p]; // first value
							vector dp2 = meta_move->skin[s + 1].dpos[f2 * sk->vertex.num + p]; // second value
							vertex_dyn[s][p] += dp1*(1-dt) + dp2*dt;
						}
						for (int i=0;i<sk->sub.num;i++)
							sk->sub[i].force_update = true;
					}
			}
		}
		
	// skeletal animation

		msg_db_m("skeletal",10);

		for (int i=0;i<bone.num;i++){
			Bone *b = &bone[i];

			// reset (only if not being edited by script)
			/*if (Nummove_operations != -2){
				b->cur_ang = q_id;//quaternion(1, v_0);
				b->cur_pos = b->Pos;
			}*/

			// operations
			for (int iop=0;iop<num_ops;iop++){
				MoveOperation *op = &move_operation[iop];
				if (op->move < 0)
					continue;
				Move *m = &meta_move->move[op->move];
				if (m->num_frames == 0)
					continue;
				if (m->type != MoveTypeSkeletal)
					continue;
				quaternion w,w0,w1,w2,w3;
				vector p,p1,p2;

			// calculate the alignment belonging to this argument
				int fr = (int)(op->time); // current frame (relative)
				int f1 = m->frame0 + fr; // current frame (absolute)
				int f2 = m->frame0 + (fr+1)%m->num_frames; // next frame (absolute)
				float df = op->time-(float)fr; // time since start of current frame
				w1 = meta_move->skel_ang[f1 * bone.num + i]; // first value
				p1 = meta_move->skel_dpos[f1*bone.num + i];
				w2 = meta_move->skel_ang[f2*bone.num + i]; // second value
				p2 = meta_move->skel_dpos[f2*bone.num + i];
				m->inter_quad = false;
				/*if (m->InterQuad){
					w0=m->ang[i][(f-1+m->NumFrames)%m->NumFrames]; // last value
					w3=m->ang[i][(f+2             )%m->NumFrames]; // third value
					// interpolate the current alignment
					QuaternionInterpolate(w,w0,w1,w2,w3,df);
					p=(1.0f-df)*p1+df*p2 + SkeletonDPos[i];
				}else*/{
					// interpolate the current alignment
					QuaternionInterpolate(w,w1,w2,df);
					p=(1.0f-df)*p1+df*p2 + b->pos;
				}


			// execute the operations

				// overwrite
				if (op->operation == MoveOpSet){
					b->cur_ang = w;
					b->cur_pos = p;

				// overwrite, if current doesn't equal 0
				}else if (op->operation == MoveOpSetNewKeyed){
					if (w.w!=1)
						b->cur_ang=w;
					if (p!=v_0)
						b->cur_pos=p;

				// overwrite, if last equals 0
				}else if (op->operation == MoveOpSetOldKeyed){
					if (b->cur_ang.w==1)
						b->cur_ang=w;
					if (b->cur_pos==v_0)
						b->cur_pos=p;

				// w = w_old         + w_new * f
				}else if (op->operation == MoveOpAdd1Factor){
					QuaternionScale(w, op->param1);
					b->cur_ang = w * b->cur_ang;
					b->cur_pos += op->param1 * p;

				// w = w_old * (1-f) + w_new * f
				}else if (op->operation == MoveOpMix1Factor){
					QuaternionInterpolate(b->cur_ang, b->cur_ang, w, op->param1);
					b->cur_pos = (1 - op->param1) * b->cur_pos + op->param1 * p;

				// w = w_old * a     + w_new * b
				}else if (op->operation == MoveOpMix2Factor){
					QuaternionScale(b->cur_ang, op->param1);
					QuaternionScale(w         , op->param2);
					QuaternionInterpolate(b->cur_ang, b->cur_ang, w, 0.5f);
					b->cur_pos = op->param1 * b->cur_pos + op->param2 * p;
				}
			}

			// bone has root -> align to root
			if (b->parent >= 0)
				b->cur_pos = bone[b->parent].dmatrix * b->pos;

			// create matrices (model -> skeleton)
			matrix t,r;
			MatrixTranslation(t, b->cur_pos);
			MatrixRotationQ(r, b->cur_ang);
			MatrixMultiply(b->dmatrix, t, r);
		}

		// create the animated data
		if (bone.num > 0)
			for (int s=0;s<MODEL_NUM_SKINS;s++){
				if (!_detail_needed_[s])
					continue;
				Skin *sk = skin[s];
				// transform vertices
				for (int p=0;p<sk->vertex.num;p++){
					int b = sk->bone_index[p];
					vector pp = sk->vertex[p] - bone_pos_0[b];
					// interpolate vertex
					vertex_dyn[s][p] = bone[b].dmatrix * pp;
					//vertex_dyn[s][p]=pp;
				}
				// normal vectors
				int offset = 0;
				for (int mm=0;mm<sk->sub.num;mm++){
					SubSkin *sub = &sk->sub[mm];
				#ifdef DynamicNormalCorrect
					for (int t=0;t<sub->num_triangles*3;t++)
						normal_dyn[s][t + offset] = bone[sk->bone_index[sub->triangle_index[t]]].dmatrix.transform_normal(sub->normal[t]);
						//normal_dyn[s][t + offset]=sub->Normal[t];
				#else
					memcpy(&normal_dyn[s][offset], &sub->Normal[0], sub->num_triangles * 3 * sizeof(vector));
				#endif
					sub->force_update = true;
					offset += sub->num_triangles;
				}
			}
	}

#endif

	
	// reset for the next frame
	for (int s=0;s<MODEL_NUM_SKINS;s++)
		_detail_needed_[s] = false;

	// update effects
	/*for (int i=0;i<NumFx;i++){
		sEffect **pfx=(sEffect**)fx;
		int nn=0;
		while( (*pfx) ){
			vector vp;
			VecTransform(vp, Matrix, Skin[0]->vertex[(*pfx)->vertex]);
			FxUpdateByModel(*pfx,vp,vp);
			pfx++;
		}
	}*/

	msg_db_m("rec",10);

	// recursion
	for (int i=0;i<bone.num;i++)
		if (bone[i].model){
			MatrixMultiply(bone[i].model->_matrix, _matrix, bone[i].dmatrix);
			bone[i].model->CalcMove();
		}
	msg_db_l(3);
}



//###############################################################################
// kuerzester Abstand:
// spaeter mit _vec_between_ und unterschiedlichen TPs rechnen, statt mit _vec_length_
//###############################################################################
bool Model::Trace(vector &p1, vector &p2, vector &dir, float range, vector &tp, bool simple_test)
{
	if (!passive_physics)
		return false;
	
	msg_db_r("model::Trace",5);
	bool hit=false;
	vector c;
	float dmin=range;
	// skeleton -> recursion
	if (bone.num>0){
		vector c;
		for (int i=0;i<bone.num;i++)
			if (bone[i].model){
				vector p2t=p1+dmin*p2;
				if (bone[i].model->Trace(p1, p2t, dir, dmin, c, simple_test)){
					if (simple_test){
						msg_db_l(5);
						return true;
					}
					hit=true;
					float d=_vec_length_(p1-c);
					if (d < dmin){
						TraceHitSubModelTemp = i;
						dmin=d;
						tp=c;
					}
				}
			}
		range=dmin;
	}

	if (!phys){
		msg_db_l(5);
		return hit;
	}
	if (phys->num_vertices<1){
		msg_db_l(5);
		return hit;
	}

	dmin=range+1;
	float d;


// Modell nah genug am Trace-Strahl?
	vector o,tm;
	plane pl;
	o = _matrix * v_0; // Modell-Mittelpunkt (absolut)
	tm=(p1+p2)/2; // Mittelpunkt des Trace-Strahles
	// Wuerfel um Modell und Trace-Mittelpunkt berschneiden sich?
	if (!o.bounding_cube(tm, radius + range / 2)){
		msg_db_l(5);
		return false;
	}
	// Strahl schneidet Ebene mit Modell (senkrecht zum Strahl)
	PlaneFromPointNormal(pl,o,dir);
	_plane_intersect_line_(c,pl,p1,p2);
	// Schnitt nah genau an Modell?
	if (!o.bounding_cube(c, radius * 2)){
		msg_db_l(5);
		return false;
	}

	// sich selbst absolut ausrichten
	_UpdatePhysAbsolute_();
	vector *p = phys_absolute.p;


// Trace-Test
	// Kugeln
	for (int i=0;i<phys->num_balls;i++){
		PlaneFromPointNormal(pl, p[phys->ball[i].index],dir);
		if (!_plane_intersect_line_(c,pl,p1,p2))
			continue;
		if (_vec_length_fuzzy_(p[phys->ball[i].index]-c) > phys->ball[i].radius)
			continue;
		float d = _vec_length_(p[phys->ball[i].index]-c);
		if (d < phys->ball[i].radius){
			c -= dir*(float)(sqrt(phys->ball[i].radius * phys->ball[i].radius - d * d));
			if (!_vec_between_(c, p1, p2))
				continue;
			if (simple_test){
				msg_db_l(5);
				return true;
			}
			d=_vec_length_(c-p1);
			// naehester Tracepunkt?
			if (d<dmin){
				hit=true;
				dmin=d;
				tp=c;
			}
		}
	}
	// Polyeder
	for (int i=0;i<phys->num_polys;i++){
		for (int k=0;k<phys->poly[i].num_faces;k++){
			if (!_plane_intersect_line_(c,phys_absolute.pl[i*MODEL_MAX_POLY_FACES+k],p1,p2))
				continue;
			if (!_vec_between_(c,p1,p2))
				continue;
			bool inside=true;
			for (int j=0;j<phys->poly[i].num_faces;j++)
				if ((j!=k)&&(_plane_distance_(phys_absolute.pl[i*MODEL_MAX_POLY_FACES+j],c)>0))
					inside=false;
			if (!inside)
				continue;
			d=_vec_length_(c-p1);
			// naehester Tracepunkt?
			if (d<dmin){
				hit=true;
				dmin=d;
				tp=c;
			}
		}
	}
	//return (dmin<range);
	msg_db_l(5);
	return hit;
}


vector _cdecl Model::GetVertex(int index, int skin_no)
{
	Skin *s = skin[skin_no];
	vector v;
	if (meta_move){ // animated
		int b = s->bone_index[index];
		v = s->vertex[index] - bone_pos_0[b];//Move(b);
		v = bone[b].dmatrix * v;
		v = _matrix * v;
	}else{ // static
		v = _matrix * s->vertex[index];
	}
	return v;
}

// reset all animation data for a model (needed in each frame before applying animations!)
void Model::ResetAnimation()
{
	num_move_operations = 0;
	/*for (int i=0;i<NumSkelettonPoints;i++)
		if (sub_model[i])
			ModelMoveReset(sub_model[i]);*/
}

// did the animation reach its end?
bool Model::IsAnimationDone(int operation_no)
{
	int move_no = move_operation[operation_no].move;
	if (move_no < 0)
		return true;
	// in case animation doesn't exist
	if (meta_move->move[move_no].num_frames == 0)
		return true;
	return (move_operation[operation_no].time >= (float)(meta_move->move[move_no].num_frames - 1));
}

// dumbly add the correct animation time, ignore animation's ending
void MoveTimeAdd(Model *m,int operation_no,float elapsed,float v,bool loop)
{
#ifdef _X_ALLOW_MODEL_
	int move_no = m->move_operation[operation_no].move;
	if (move_no < 0)
		return;
	Move *move = &m->meta_move->move[move_no];
	if (move->num_frames == 0)
		return; // in case animation doesn't exist

	// how many frames have passed
	float dt = elapsed * ( move->frames_per_sec_const + move->frames_per_sec_factor * v );

	// add the correct time
	m->move_operation[operation_no].time += dt;
	// time may now be way out of range of the animation!!!

	if (m->IsAnimationDone(operation_no)){
		if (loop)
			m->move_operation[operation_no].time -= float(move->num_frames) * (int)((float)(m->move_operation[operation_no].time / move->num_frames));
		else
			m->move_operation[operation_no].time = (float)(move->num_frames) - 1;
	}
#endif
}

// apply an animate to a model
//   a new animation "layer" is being added for mixing several animations
//   the variable <time> is being increased
bool Model::Animate(int mode, float param1, float param2, int move_no, float &time, float elapsed, float vel_param, bool loop)
{
	if (!meta_move)
		return false;
	msg_db_r("model::Move",3);
	if (num_move_operations < 0)
		num_move_operations = 0;
	int n = num_move_operations ++;
	move_operation[n].move = move_no;
	move_operation[n].operation = mode;
	move_operation[n].param1 = param1;
	move_operation[n].param2 = param2;
	move_operation[n].time = time;

	MoveTimeAdd(this, n, elapsed, vel_param, loop);
	time = move_operation[n].time;
	bool b = IsAnimationDone(n);
	msg_db_l(3);
	return b;
}

// get the number of frames for a particular animation
int Model::GetFrames(int move_no)
{
	if (!meta_move)
		return 0;
	return meta_move->move[move_no].num_frames;
}

// edit skelettal animation via script
void Model::BeginEditAnimation()
{
	if (!meta_move)
		return;
	num_move_operations = -2;
	for (int i=0;i<bone.num;i++){
		bone[i].cur_ang = q_id;//quaternion(1,v_0);
		bone[i].cur_pos = bone[i].pos;
	}
}

// make sure, the model/skin is editable and not just a reference.... (soon)
void Model::BeginEdit(int detail)
{
	msg_db_r("model.BeginEdit", 1);
	MetaModelMakeEditable(this);
	if (skin_is_reference[detail]){
		Skin *o = skin[detail];
		CopySkinNew(this, o, &skin[detail]);
		skin_is_reference[detail] = false;
	}
	msg_db_l(1);
}

// force an update for this model/skin
void Model::EndEdit(int detail)
{
	for (int i=0;i<material.num;i++){
		skin[detail]->sub[i].force_update = true;
		if (skin[detail]->sub[i].num_triangles > NixVBGetMaxTrias(skin[detail]->sub[i].vertex_buffer)){
			NixDeleteVB(skin[detail]->sub[i].vertex_buffer);
			skin[detail]->sub[i].vertex_buffer = -1;
		}
	}
}


// make sure we can edit this object without destroying an original one
void Model::MakeEditable()
{
	msg_db_r("model.MakeEditable", 1);
	// original -> create copy
	if (!is_copy)
		MetaModelMakeEditable(this);

	// must have its own materials
	if (material_is_reference){
		material.make_own();
		material_is_reference = false;
	}
	msg_db_l(1);
}

string Model::GetFilename()
{
	return _template->filename;
}


void Model::JustDraw(int mat_no, int detail)
{
	msg_db_r("model.JustDraw",5);
	_detail_needed_[detail] = true;
	Skin *s = skin[detail];
	SubSkin *sub = &s->sub[mat_no];
	vector *p = &s->vertex[0];
	if (vertex_dyn[detail])
		p = vertex_dyn[detail];
	vector *n = &sub->normal[0];
	if (normal_dyn[detail])
		n = normal_dyn[detail];
	int *tv = &sub->triangle_index[0];
	float *sv = &sub->skin_vertex[0];
	Material *m = &material[mat_no];

#ifdef MODEL_TEMP_VB

	TODO
	int nt=0;
	for (int t=0;t<num_textures;t++){
		NixVBClear(VBTemp);
		for (int i=nt;i<nt+s->texture_num_triangles[t];i++){
			int va=tv[i*3  ]  ,vb=tv[i*3+1]  ,vc=tv[i*3+2];
			int sa=ts[i*3  ]*2,sb=ts[i*3+1]*2,sc=ts[i*3+2]*2;
			NixVBAddTria(VBTemp,	p[va],n[i*3  ],sv[sa],sv[sa+1],
									p[vb],n[i*3+1],sv[sb],sv[sb+1],
									p[vc],n[i*3+2],sv[sc],sv[sc+1]);
		}
		/*NixVBFillIndexed(	VBTemp,
								s->num_vertices,
								s->num_triangles,
								&s->vertex[0],
								&s->Normal[0],
								&s->skin_vertex[0],
								&s->triangle_index[0]);*/
		NixDraw3D(Texture[t],VBTemp,mat);
#ifdef _X_ALLOW_FX_
		if ((CubeMap>=0)&&(ReflectionDensity>0)){
			NixSetMaterial(Ambient,Black,Black,0,Emission);
			FxCubeMapDraw(CubeMap,VBTemp,mat,ReflectionDensity);
		}
#endif
		nt+=s->texture_num_triangles[t];
	}

#else
	//-----------------------------------------------------

	if (sub->force_update){
			
		// vertex buffer existing?
		if (sub->vertex_buffer < 0){
			msg_db_m("vb not existing -> new",3);
			if (m->num_textures == 1)
				sub->vertex_buffer = NixCreateVB(sub->num_triangles);
			else
				sub->vertex_buffer = NixCreateVBM(sub->num_triangles, m->num_textures);
		}
		msg_db_m("empty",6);
		NixVBClear(sub->vertex_buffer);
		msg_db_m("new...",6);
		if (m->num_textures == 1){
			for (int i=0;i<sub->num_triangles;i++){
				//msg_write(i);
				int va=tv[i*3  ]  ,vb=tv[i*3+1]  ,vc=tv[i*3+2];
				NixVBAddTria(	sub->vertex_buffer,
								p[va],n[i*3  ],sv[i*6  ],sv[i*6+1],
								p[vb],n[i*3+1],sv[i*6+2],sv[i*6+3],
								p[vc],n[i*3+2],sv[i*6+4],sv[i*6+5]);
			}
		}else{
			for (int i=0;i<sub->num_triangles;i++){
				float tc[3][MODEL_MAX_TEXTURES * 2];
				for (int k=0;k<3;k++)
					for (int j=0;j<m->num_textures;j++){
						tc[k][j * 2    ] = sub->skin_vertex[j * sub->num_triangles * 6 + i * 6 + k * 2];
						tc[k][j * 2 + 1] = sub->skin_vertex[j * sub->num_triangles * 6 + i * 6 + k * 2 + 1];
					}
				//msg_write(i);
				int va=tv[i*3  ]  ,vb=tv[i*3+1]  ,vc=tv[i*3+2];
				NixVBAddTriaM(	sub->vertex_buffer,
								p[va],n[i*3  ],tc[0],
								p[vb],n[i*3+1],tc[1],
								p[vc],n[i*3+2],tc[2]);
			}
		}
		msg_db_m("--ok",6);
		sub->force_update = false;
	}


	NixSetWorldMatrix(_matrix);
	if (m->num_textures == 1){
		NixSetTexture(m->texture[0]);
		NixDraw3D(sub->vertex_buffer);
	}else{
		NixSetTextures(m->texture, m->num_textures);
		NixDraw3DM(sub->vertex_buffer);
	}
#ifdef _X_ALLOW_FX_
	if ((m->cube_map >= 0) && (m->reflection_density > 0)){
		//_Pos_ = *_matrix_get_translation_(_matrix);
		NixSetMaterial(m->ambient, Black, Black, 0, m->emission);
		FxCubeMapDraw(m->cube_map, sub->vertex_buffer, m->reflection_density);
	}
#endif
#endif
	msg_db_l(5);
}

#if 0
void Model::SortingTest(vector &pos,const vector &dpos,matrix *mat,bool allow_shadow)
{
	for (int i=0;i<bone.num;i++)
		if (boneModel[i]){
			vector sub_pos;
			MatrixMultiply(boneMatrix[i],*mat,boneDMatrix[i]);
			VecTransform(sub_pos,boneMatrix[i],v_0);
			boneModel[i]->SortingTest(sub_pos,dpos,&BoneMatrix[i],allow_shadow);
		}
	int _Detail_=SkinHigh;
	bool trans=false;
	float ld=_vec_length_(dpos); // real distance to the camera
	float ld_2=ld*DetailFactorInv; // more adequate distance value

	// which level of detail?
	if (ld_2>DetailDist[0])		_Detail_=SkinMedium;
	if (ld_2>DetailDist[1])		_Detail_=SkinLow;
	if (ld_2>DetailDist[2]){	_Detail_=-1;	return;	}

	// transparent?
	if (Material[0].TransparencyMode>0){
		if (Material[0].TransparencyMode==TransparencyModeFunctions)
			trans=true;
		if (Material[0].TransparencyMode==TransparencyModeFactor)
			trans=true;
	}

	if (meta_move){
		_Detail_Needed_[_Detail_]=true;
		if ( ( ShadowLevel > 0) && ShadowLowerDetail && allow_shadow && ( _Detail_ == SkinHigh ) )
			_Detail_Needed_[SkinMedium]=true; // shadows...
	}

	// send for sorting
	MetaAddSorted(this,pos,mat,_Detail_,ld,trans,allow_shadow);
}
#endif

void Model::Draw(int detail, bool set_fx, bool allow_shadow)
{
	if	(detail<SkinHigh)
		return;
//	msg_write("d");
#ifdef _X_ALLOW_FX_
	// Schatten?
/*	if ((ShadowLevel>0)&&(set_fx)&&(detail==SkinHigh)&&(allow_shadow)){//(TransparencyMode<=0))
		int sd = ShadowLowerDetail ? SkinMedium : SkinHigh;
		//if (Skin[sd]->num_triangles>0)
			FxAddShadow(this, sd);
			//FxAddShadow(Skin[sd],vertex_dyn[sd],Matrix,100000);//Diameter*5);
	}*/

	// ist die Oberflaeche ein Spiegel?
	/*if (Material[0].ReflectionMode==ReflectionMirror)
		FxDrawMirrors(Skin[detail],mat);*/
#endif


	for (int i=0;i<material.num;i++){
		MetaSetMaterial(&material[i]);

		// endlich wirklich mahlen!!!
		JustDraw(i, detail);
		NixSetShader(-1);
		NixSetAlpha(AlphaNone);
	}

/*
#ifdef _X_ALLOW_FX_
	if (material[0].reflection_mode == ReflectionMetal)
		FxDrawMetal(skin[detail], _matrix, material[0].reflection_density);
#endif

	if (material[0].transparency_mode > 0)
		NixSetAlpha(AlphaNone);*/
}

