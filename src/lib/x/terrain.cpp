/*----------------------------------------------------------------------------*\
| Terrain                                                                      |
| -> terrain of a world                                                        |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2008.11.02 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/

#include "x.h"

#define Index(x,z)		((x)*(num_z+1)+(z))
#define Index2(t,x,z)	((x)*(t->num_z+1)+(z))
//#define max(a,b)		(((a)>(b))?(a):(b))
//#define min(a,b)		(((a)<(b))?(a):(b))

void Terrain::reset()
{
	filename = "";
	error = false;
	num_x = num_z = 0;
	changed = false;
	vertex_buffer = -1;
}

Terrain::Terrain()
{
	reset();
}

Terrain::Terrain(const string &_filename_, const vector &_pos_)
{
	Load(_filename_, _pos_);
}

bool Terrain::Load(const string &_filename_, const vector &_pos_, bool deep)
{
	msg_db_r("loading terrain", 1);
	msg_write("loading terrain: " + _filename_);
	msg_right();

	reset();

	filename = _filename_;
	CFile *f = OpenFile(MapDir + filename + ".map");
	if (f){

		int ffv = f->ReadFileFormatVersion();
		if (ffv == 4){
			f->ReadByte();
			// Metrics
			num_x = f->ReadIntC();
			num_z = f->ReadInt();
			int num = (num_x + 1) * (num_z + 1);
			height.resize(num);
			normal.resize(num);
			vertex.resize(num);
			pattern.x = f->ReadFloat();
			pattern.y = 0;
			pattern.z = f->ReadFloat();
			// Textures
			int num_textures = f->ReadIntC();
			for (int i=0;i<num_textures;i++){
				texture_file[i] = f->ReadStr();
				texture_scale[i].x = f->ReadFloat();
				texture_scale[i].y = 0;
				texture_scale[i].z = f->ReadFloat();
			}
			// Material
			material_file = f->ReadStr();
			if (deep){

				// load textures
				material.num_textures = num_textures;
				for (int i=0;i<num_textures;i++)
					material.texture[i] = NixLoadTexture(texture_file[i]);

				// material file
				Material *material_from_file = MetaLoadMaterial(material_file);
				material.copy_from(NULL, material_from_file, false);

				// height
				for (int x=0;x<num_x+1;x++)
					for (int z=0;z<num_z+1;z++)
						height[Index(x,z)] = f->ReadFloat();
				for (int x=0;x<num_x/32+1;x++)
					for (int z=0;z<num_z/32+1;z++)
						partition[x][z] = -1;

				vertex_buffer = NixCreateVB(65536, num_textures);
			}
		}else{
			msg_error(format("wrong file format: %d (4 expected)",ffv));
			error = true;
		}

		FileClose(f);


		// generate normal vectors
		pos = _pos_;
		if (deep)
			Update(-1, -1, -1, -1, TerrainUpdateAll);
		// bounding box
		min = pos;
		max = pos + vector(pattern.x * num_x, 0, pattern.z * num_z);

		changed = false;
		force_redraw = true;
	}else
		error = true;
	msg_left();
	msg_db_l(1);
	return !error;
}

Terrain::~Terrain()
{
	msg_db_r("~Terrain",1);
	NixDeleteVB(vertex_buffer);
	msg_db_l(1);
}

// die Normalen-Vektoren in einem bestimmten Abschnitt der Karte neu berechnen
void Terrain::Update(int x1,int x2,int z1,int z2,int mode)
{
	msg_db_r("terrain::update",1);
	if (x1<0)		x1=0;
	if (x2<0)		x2=num_x;
	if (x2>=num_x)	x2=num_x;
	if (z1<0)		z1=0;
	if (z2<0)		z2=num_z;
	if (z2>=num_z)	z2=num_z;
	bool un=((mode & TerrainUpdateNormals)>0);
	bool uv=((mode & TerrainUpdateVertices)>0);
	bool up=((mode & TerrainUpdatePlanes)>0);
	float dhx,dhz;

	pl.resize(num_x * num_z * 2);

	// create (soft) normal vectors and vertices
	for (int i=x1;i<=x2;i++)
		for (int j=z1;j<=z2;j++){
			int n=Index(i,j);
			if (un){
				if ((i>1)&&(i<num_x-1))
					dhx=height[Index(i+2,j)]-height[Index(i-2,j)];
				else
					dhx=0;
				if ((j>1)&&(j<num_z-1))
					dhz=height[Index(i,j+2)]-height[Index(i,j-2)];
				else
					dhz=0;
				normal[n]=vector(-dhx/pattern.x,4,-dhz/pattern.z);
				normal[n].normalize();
			}
			if (uv)
				vertex[n]=pos+vector(pattern.x*(float)i,height[n],pattern.z*(float)j);
		}
	if (uv){
		int j = z2;
		for (int i=x1;i<=x2;i++)
			vertex[Index(i,j)]=pos+vector(pattern.x*(float)i,height[Index(i,j)],pattern.z*(float)j);
		int i = x2;
		for (j=z1;j<=z2;j++)
			vertex[Index(i,j)]=pos+vector(pattern.x*(float)i,height[Index(i,j)],pattern.z*(float)j);
	}

	// update planes (for collision detection)
	if (up)
		for (int i=x1;i<x2;i++)
			for (int j=z1;j<z2;j++){
				int nt=(i*num_z+j)*2;
				PlaneFromPoints(pl[nt  ],vertex[Index(i  ,j  )],vertex[Index(i  ,j+1)],vertex[Index(i+1,j+1)]);
				PlaneFromPoints(pl[nt+1],vertex[Index(i  ,j  )],vertex[Index(i+1,j+1)],vertex[Index(i+1,j  )]);
			}

	force_redraw = true;
	msg_db_l(1);
}

float Terrain::gimme_height(const vector &p) // liefert die interpolierte Hoehe zu einer Position
{
	float x = p.x;
	float z = p.z;
	if ((x<=min.x)||(z<=min.z)||(x>=max.x)||(z>=max.z))
		return p.y;
	x-=pos.x;
	z-=pos.z;
	bool SPI_OB=false;
	float he=0;
	int dr_z=(int)(z/pattern.z); // welches Dreieck?
	int dr_x=(int)(x/pattern.x);
	if (x-dr_x*pattern.x<z-dr_z*pattern.z)
		SPI_OB=true;
	int j=dr_z;
	int i=dr_x;
	if (SPI_OB){ // genaue Position auf dem Dreieck
		dhx=height[Index(i+1,j+1)]-height[Index(i,j+1)];
		dhz=height[Index(i,j)]-height[Index(i,j+1)];
		he=height[Index(dr_x,dr_z+1)] + dhx/pattern.x*(x-dr_x*pattern.x) + dhz/pattern.z*(dr_z*pattern.z+pattern.z-z);
	}else{
		dhx=height[Index(i+1,j  )]-height[Index(i  ,j  )];
		dhz=height[Index(i+1,j  )]-height[Index(i+1,j+1)];
		he=height[Index(dr_x+1,dr_z)]-dhx/pattern.x*(dr_x*pattern.x+pattern.x-x)-dhz/pattern.z*(z-dr_z*pattern.z);
	}

	return he+pos.y;
}

float Terrain::gimme_height_n(const vector &p, vector &n)
{
	float he=gimme_height(p);
	vector vdx=vector(pattern.x, dhx,0            );
	vector vdz=vector(0        ,-dhz,pattern.z);
	n=VecCrossProduct(vdz,vdx);
	n.normalize();
	return he;
}

// Daten fuer das Darstellen des Bodens
void Terrain::CalcDetail()
{
	for (int x1=0;x1<(num_x-1)/32+1;x1++)
		for (int z1=0;z1<(num_z-1)/32+1;z1++){
			int lx=(x1*32>num_x-32)?(num_x%32):32;
			int lz=(z1*32>num_z-32)?(num_z%32):32;
			int x0=x1*32;
			int z0=z1*32;
			float depth=(cur_cam->pos-vertex[Index(x0+lx/2,z0+lz/2)]).length()/pattern.x;
			int e=32;
			if (depth<500)	e=32;
			if (depth<320)	e=16;
			if (depth<160)	e=8;
			if (depth<100)	e=4;
			if (depth<70)	e=2;
			if (depth<40)	e=1;
			partition[x1][z1]=e;
		}
}

static int TempVertexIndex[2048];
static int TempTriangleIndex[2048];
static int TempEdgeIndex[2048];
static plane TempPlaneList[2048];

inline void add_edge(int &num, int e0, int e1)
{
	if (e0 > e1){
		int t = e0;
		e0 = e1;
		e1 = t;
	}
	/*for (int i=0;i<num;i++)
		if ((TempEdgeIndex[i*2] == e0) && (TempEdgeIndex[i*2+1] == e1))
			return;*/
	TempEdgeIndex[num * 2    ] = e0;
	TempEdgeIndex[num * 2 + 1] = e1;
	num ++;
}

// for collision detection:
//    get a part of the terrain
void Terrain::GetTriangleHull(void *hull, vector &_pos_, float _radius_)
{
	//msg_db_r("Terrain::GetTriangleHull", 1);
	TriangleHull *h = (TriangleHull*)hull;
	h->p = &vertex[0];
	h->index = TempVertexIndex;
	h->triangle_index = TempTriangleIndex;
	h->edge_index = TempEdgeIndex;
	h->pl = TempPlaneList;

	h->num_vertices = 0;
	h->num_triangles = 0;
	h->num_edges = 0;

	// how much do we need
	vector _min_ = _pos_ - vector(1,1,1) * _radius_ - pos;
	vector _max_ = _pos_ + vector(1,1,1) * _radius_ - pos;

	int x0=int(_min_.x/pattern.x);	if (x0<0)	x0=0;
	int z0=int(_min_.z/pattern.z);	if (z0<0)	z0=0;
	int x1=int(_max_.x/pattern.x+1);	if (x1>num_x)	x1=num_x;
	int z1=int(_max_.z/pattern.z+1);	if (z1>num_z)	z1=num_z;

	if ((x0>=x1)||(z0>=z1)){
		//msg_db_l(1);
		return;
	}

	//printf("%d %d %d %d\n", x0, x1, z0, z1);

	// c d
	// a b
	// (acd),(adb)
	h->num_vertices = (x1-x0) * (z1-z0);
	h->num_triangles = (x1-x0) * (z1-z0) * 2;
	h->num_edges = 0;
	int n = 0, n6 = 0;
	for (int x=x0;x<x1;x++)
		for (int z=z0;z<z1;z++){
			// vertex
			TempVertexIndex[n]=Index(x,z);
			// triangle 1
			TempTriangleIndex[n6  ]=Index(x  ,z  );
			TempTriangleIndex[n6+1]=Index(x  ,z+1);
			TempTriangleIndex[n6+2]=Index(x+1,z+1);
			TempPlaneList[n*2  ]=pl[(x*num_z+z)*2  ];
			// triangle 2
			TempTriangleIndex[n6+3]=Index(x  ,z  );
			TempTriangleIndex[n6+4]=Index(x+1,z+1);
			TempTriangleIndex[n6+5]=Index(x+1,z  );
			TempPlaneList[n*2+1]=pl[(x*num_z+z)*2+1];
			// edges
			add_edge(h->num_edges, Index(x  ,z  ), Index(x+1,z  ));
			add_edge(h->num_edges, Index(x  ,z  ), Index(x  ,z+1));
			add_edge(h->num_edges, Index(x  ,z  ), Index(x+1,z+1));
			n ++;
			n6 += 6;
		}
	//msg_db_l(1);
}

inline bool TracePattern(Terrain *t,vector &p1,vector &p2,vector &tp,int x,int z,float y_min,int dir,float range)
{
	// trace beam too high above this pattern?
	if ( (t->height[Index2(t,x,z)]<y_min) && (t->height[Index2(t,x,z+1)]<y_min) && (t->height[Index2(t,x+1,z)]<y_min) && (t->height[Index2(t,x+1,z+1)]<y_min) )
		return false;

	// 4 vertices for 2 triangles
	vector a=t->vertex[Index2(t,x  ,z  )];
	vector b=t->vertex[Index2(t,x+1,z  )];
	vector c=t->vertex[Index2(t,x  ,z+1)];
	vector d=t->vertex[Index2(t,x+1,z+1)];

	float dmin1=range,dmin2=range;
	vector ttp;
	vector v;

	// scan both triangles
	if (LineIntersectsTriangle(a,b,d,p1,p2,tp,false))
		dmin1=(tp-p1).length();
	if (LineIntersectsTriangle(a,c,d,p1,p2,ttp,false)){
		dmin2=(tp-p1).length();
		if (dmin2<dmin1){ // better than the first one?
			dmin1=dmin2;
			tp=ttp;
		}
	}

	// don't scan backwards...
	if ((dir==0)&&(tp.x<p1.x))	return false;
	if ((dir==1)&&(tp.z<p1.z))	return false;
	if ((dir==2)&&(tp.x>p1.x))	return false;
	if ((dir==3)&&(tp.z>p1.z))	return false;

	return (dmin1<range);
}

bool Terrain::Trace(vector &p1, vector &p2, vector &dir, float range, vector &tp, bool simple_test)
{
	msg_db_m("terrain::Trace",4);
	float dmin=range+1;
	vector c;

	if ((p2.x==p1.x)&&(p2.z==p1.z)&&(p2.y<p1.y)){
		float h=gimme_height(p1);
		if (p2.y<h){
			tp=vector(p1.x,h,p1.z);
			return true;
		}
	}

	vector pr1 = p1 - pos;
	vector pr2 = p2 - pos;
	float w=(float)atan2(p2.z-p1.z,p2.x-p1.x);
	int x,z;
	float y_rel;

// scanning directions

	// positive x (to the right)
	if ((w>=-pi/4)&&(w<pi/4)){
		int x0 = max( int(pr1.x/pattern.x), 0 );
		int x1 = min( int(pr2.x/pattern.x), num_x );
		for (x=x0;x<x1;x++){
			z = int( ( ( pattern.x*((float)x+0.5f) - pr1.x ) * (pr2.z-pr1.z) / (pr2.x-pr1.x) + pr1.z ) / pattern.z - 0.5f );
			if ( ( z > num_z - 2) || ( z < 0 ) )
				break;
			if ( p1.y > p2.y )	y_rel = pr1.y + ( ( x + 1 ) * pattern.x - pr1.x ) * ( pr2.y - pr1.y ) / ( pr2.x - pr1.x );
			else				y_rel = pr1.y + (   x       * pattern.x - pr1.x ) * ( pr2.y - pr1.y ) / ( pr2.x - pr1.x );
			if (TracePattern(this,p1,p2,tp,x,z  ,y_rel,0,range))	return true;
			if (TracePattern(this,p1,p2,tp,x,z+1,y_rel,0,range))	return true;
		}
	}

	// positive z (forward)
	if ((w>=pi/4)&&(w<pi*3/4)){
		int z0 = max( int(pr1.z/pattern.z), 0 );
		int z1 = min( int(pr2.z/pattern.z), num_z );
		for (z=z0;z<z1;z++){
			x = int( ( ( pattern.z*((float)z+0.5f) - pr1.z ) * (pr2.x-pr1.x) / (pr2.z-pr1.z) + pr1.x ) / pattern.x - 0.5f );
			if ( ( x > num_x - 2) || ( x < 0 ) )
				break;
			if ( p1.y > p2.y )	y_rel = pr1.y + ( ( z + 1 ) * pattern.z - pr1.z ) * ( pr2.y - pr1.y ) / ( pr2.z - pr1.z );
			else				y_rel = pr1.y + (   z       * pattern.z - pr1.z ) * ( pr2.y - pr1.y ) / ( pr2.z - pr1.z );
			if (TracePattern(this,p1,p2,tp,x  ,z,y_rel,1,range))	return true;
			if (TracePattern(this,p1,p2,tp,x+1,z,y_rel,1,range))	return true;
		}
	}

	// negative x (to the left)
	if ((w>=pi*3/4)||(w<-pi*3/4)){
		int x0 = min( int(pr1.x/pattern.x), num_x );
		int x1 = max( int(pr2.x/pattern.x), 0 );
		for (x=x0;x>x1;x--){
			z = int( ( ( pattern.x*((float)x+0.5f) - pr1.x ) * (pr2.z-pr1.z) / (pr2.x-pr1.x) + pr1.z ) / pattern.z - 0.5f );
			if ( ( z > num_z - 2) || ( z < 0 ) )
				break;
			if ( p1.y < p2.y )	y_rel = pr1.y + ( ( x + 1 ) * pattern.x - pr1.x ) * ( pr2.y - pr1.y ) / ( pr2.x - pr1.x );
			else				y_rel = pr1.y + (   x       * pattern.x - pr1.x ) * ( pr2.y - pr1.y ) / ( pr2.x - pr1.x );
			if (TracePattern(this,p1,p2,tp,x,z  ,y_rel,2,range))	return true;
			if (TracePattern(this,p1,p2,tp,x,z+1,y_rel,2,range))	return true;
		}
	}

	// negative z (backward)
	if ((w>=-pi*3/4)&&(w<-pi/4)){
		int z0 = min( int(pr1.z/pattern.z), num_z );
		int z1 = max( int(pr2.z/pattern.z), 0 );
		for (z=z0;z>z1;z--){
			x = int( ( ( pattern.z*((float)z+0.5f) - pr1.z ) * (pr2.x-pr1.x) / (pr2.z-pr1.z) + pr1.x ) / pattern.x - 0.5f );
			if ( ( x > num_x - 2) || ( x < 0 ) )
				break;
			if ( p1.y < p2.y )	y_rel = pr1.y + ( ( z + 1 ) * pattern.z - pr1.z ) * ( pr2.y - pr1.y ) / ( pr2.z - pr1.z );
			else				y_rel = pr1.y + (   z       * pattern.z - pr1.z ) * ( pr2.y - pr1.y ) / ( pr2.z - pr1.z );
			if (TracePattern(this,p1,p2,tp,x  ,z,y_rel,3,range))	return true;
			if (TracePattern(this,p1,p2,tp,x+1,z,y_rel,3,range))	return true;
		}
	}
	tp=p1;
	return false;
}

void Terrain::Draw()
{
	msg_db_r("terrain::Draw",3);
	redraw = false;
	// c d
	// a b
	// (acd),(adb)
	CalcDetail(); // how detailed shall it be?


	// do we have to recreate the terrain?
	if (force_redraw)
		redraw = true;
	else
		for (int x=0;x<num_x/32;x++)
			for (int z=0;z<num_z/32;z++)
				if (partition_old[x][z]!=partition[x][z])
					redraw = true;

	// recreate (in vertex buffer)
	if (redraw){
		NixVBClear(vertex_buffer);

		// number of 32-blocks (including the partially filled ones)
		int nx = ( num_x - 1 ) / 32 + 1;
		int nz = ( num_z - 1 ) / 32 + 1;
		/*nx=num_x/32;
		nz=num_z/32;*/

		// loop through the 32-blocks
		for (int x1=0;x1<nx;x1++)
			for (int z1=0;z1<nz;z1++){

				// block size?    (32 or cut off...)
				int lx = ( x1 * 32 > num_x - 32 ) ? ( num_x % 32 ) : 32;
				int lz = ( z1 * 32 > num_z - 32 ) ? ( num_z % 32 ) : 32;

				// start
				int x0=x1*32;
				int z0=z1*32;
				int e=partition[x1][z1];
				if (e<0)	continue;

				// loop through the squares
				for (int x=x0;x<=x0+lx-e;x+=e)
					for (int z=z0;z<=z0+lz-e;z+=e){
						// x,z "real" pattern indices

						// vertices
						vector va=vertex[Index(x  ,z  )];
						vector vb=vertex[Index(x+e,z  )];
						vector vc=vertex[Index(x  ,z+e)];
						vector vd=vertex[Index(x+e,z+e)];

						// normal vectors
						vector na=normal[Index(x  ,z  )];
						vector nb=normal[Index(x+e,z  )];
						vector nc=normal[Index(x  ,z+e)];
						vector nd=normal[Index(x+e,z+e)];

						// left border correction
						if (x==x0)		if (x1>0){ int p=partition[x1-1][z1];	if (p>0)	if (e<p){
							int a0=p*int(z/p);
							if (a0+p<=num_z){
								float t0=float(z%p)/(float)p;
								float t1=t0+float(e)/float(p);
								va=vertex[Index(x,a0)]*(1-t0)+vertex[Index(x,a0+p)]*t0;
								na=normal[Index(x,a0)]*(1-t0)+normal[Index(x,a0+p)]*t0;
								vc=vertex[Index(x,a0)]*(1-t1)+vertex[Index(x,a0+p)]*t1;
								nc=normal[Index(x,a0)]*(1-t1)+normal[Index(x,a0+p)]*t1;
							}
						}}

						// right border correction
						if (x==x0+32-e)	if (x1<nx-1){ int p=partition[x1+1][z1];	if (p>0)	if (e<p){
							int a0=p*int(z/p);
							if (a0+p<num_x){
								float t0=float(z%p)/(float)p;
								float t1=t0+float(e)/float(p);
								vb=vertex[Index(x+e,a0)]*(1-t0)+vertex[Index(x+e,a0+p)]*t0;
								nb=normal[Index(x+e,a0)]*(1-t0)+normal[Index(x+e,a0+p)]*t0;
								vd=vertex[Index(x+e,a0)]*(1-t1)+vertex[Index(x+e,a0+p)]*t1;
								nd=normal[Index(x+e,a0)]*(1-t1)+normal[Index(x+e,a0+p)]*t1;
							}
						}}

						// bottom border correction
						if (z==z0)		if (z1>0){ int p=partition[x1][z1-1];	if (p>0)	if (e<p){
							int a0=p*int(x/p);
							if (a0+p<=num_x){
								float t0=float(x%p)/(float)p;
								float t1=t0+float(e)/float(p);
								va=vertex[Index(a0,z)]*(1-t0)+vertex[Index(a0+p,z)]*t0;
								na=normal[Index(a0,z)]*(1-t0)+normal[Index(a0+p,z)]*t0;
								vb=vertex[Index(a0,z)]*(1-t1)+vertex[Index(a0+p,z)]*t1;
								nb=normal[Index(a0,z)]*(1-t1)+normal[Index(a0+p,z)]*t1;
							}
						}}

						// top border correction
						if (z==z0+32-e)		if (z1<nz-1){ int p=partition[x1][z1+1];	if (p>0)	if (e<p){
							int a0=p*int(x/p);
							if (a0+p<num_z){
								float t0=float(x%p)/(float)p;
								float t1=t0+float(e)/float(p);
								vc=vertex[Index(a0,z+e)]*(1-t0)+vertex[Index(a0+p,z+e)]*t0;
								nc=normal[Index(a0,z+e)]*(1-t0)+normal[Index(a0+p,z+e)]*t0;
								vd=vertex[Index(a0,z+e)]*(1-t1)+vertex[Index(a0+p,z+e)]*t1;
								nd=normal[Index(a0,z+e)]*(1-t1)+normal[Index(a0+p,z+e)]*t1;
							}
						}}

						// multitexturing
						float ta[8],tb[8],tc[8],td[8];
						for (int i=0;i<material.num_textures;i++){
							ta[i*2]=(float) x   *texture_scale[i].x,	ta[i*2+1]=(float) z   *texture_scale[i].z;
							tb[i*2]=(float)(x+e)*texture_scale[i].x,	tb[i*2+1]=(float) z   *texture_scale[i].z;
							tc[i*2]=(float) x   *texture_scale[i].x,	tc[i*2+1]=(float)(z+e)*texture_scale[i].z;
							td[i*2]=(float)(x+e)*texture_scale[i].x,	td[i*2+1]=(float)(z+e)*texture_scale[i].z;
						}

						// add to buffer
						if (material.num_textures==1){
							NixVBAddTria(vertex_buffer,	va,na,ta[0],ta[1],
														vc,nc,tc[0],tc[1],
														vd,nd,td[0],td[1]);
							NixVBAddTria(vertex_buffer,	va,na,ta[0],ta[1],
														vd,nd,td[0],td[1],
														vb,nb,tb[0],tb[1]);
						}else{
							NixVBAddTriaM(vertex_buffer,	va,na,ta,
															vc,nc,tc,
															vd,nd,td);
							NixVBAddTriaM(vertex_buffer,	va,na,ta,
															vd,nd,td,
															vb,nb,tb);
						}
					}
			}
	}

#ifdef _X_ALLOW_LIGHT_
	Light::Apply(cur_cam->pos);
#endif

	material.apply();

	// the actual drawing
	NixSetWorldMatrix(m_id);
	NixDraw3D(vertex_buffer);

	pos_old = cur_cam->pos;
	force_redraw = false;
	for (int x=0;x<num_x/32;x++)
		for (int z=0;z<num_z/32;z++)
			partition_old[x][z]=partition[x][z];
	NixSetShader(-1);
	msg_db_l(3);
}

