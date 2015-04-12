
#include "dxfw.h"

using namespace std;
using namespace base;





// **************** DevMesh ****************


DevMesh::DevMesh()
{
	mesh = NULL;
	adjacency = NULL;
	vdata = NULL;
	idata = NULL;
	adata = NULL;
}

DevMesh::DevMesh(const char *_path)
{
	mesh = NULL;
	adjacency = NULL;
	vdata = NULL;
	idata = NULL;
	adata = NULL;
	preload_path = _path;
}

DevMesh::~DevMesh()
{
	Clear(true);
}

bool DevMesh::Load(const char *path)
{
	if(mesh)
	{
		mesh->Release();
		mesh = NULL;
	}

	if(path[0] && path[strlen(path)-1]=='x')
	{
		HRESULT res = D3DXLoadMeshFromX(path,D3DXMESH_32BIT | D3DXMESH_MANAGED,
										Dev.GetDevice(),NULL,NULL,NULL,NULL,&mesh);
		return !FAILED(res) && mesh;
	}

	FileReaderStream file(path);
	TreeFileBuilder tfb;
	if(!tfb.LoadTreeBin(&file))
		return false;

	TreeFileRef root = tfb.GetRoot(false);
	TreeFileRef f_vb = root.SerChild("VertexBuffer");
	TreeFileRef f_ib = root.SerChild("IndexBuffer");
	TreeFileRef f_ab = root.SerChild("AttrBuffer");
	int vsize;
	f_vb.SerInt("VertexSize",vsize,3*sizeof(float));
	void  *vdata  = (void*)f_vb.Read_GetRawData("Data");
	int    vcount = f_vb.Read_GetRawSize("Data")/vsize;
	int   *idata  = (int*)f_ib.Read_GetRawData("Data");
	int    icount = f_ib.Read_GetRawSize("Data")/sizeof(int);
	DWORD *attr   = (DWORD*)f_ab.Read_GetRawData("Data");
	DWORD  acount = f_ab.Read_GetRawSize("Data")/sizeof(DWORD);
	if(acount*3 != icount)
		attr = NULL;

	DWORD FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX3 | D3DFVF_TEXCOORDSIZE3(1) | D3DFVF_TEXCOORDSIZE3(2);
	f_vb.SerDword("FVF",*(dword*)&FVF,FVF);

	vector<D3DXATTRIBUTERANGE> ranges;
	ranges.resize( f_ab.GetCloneArraySize("Range") );
	for(int i=0;i<(int)ranges.size();i++)
	{
		D3DXATTRIBUTERANGE *r = &ranges[i];
		TreeFileRef rf = f_ab.SerChild("Range",i);
		rf.SerDword("AttribId"		,*(dword*)&r->AttribId		,0);
		rf.SerDword("FaceStart"		,*(dword*)&r->FaceStart		,0);
		rf.SerDword("FaceCount"		,*(dword*)&r->FaceCount		,0);
		rf.SerDword("VertexStart"	,*(dword*)&r->VertexStart	,0);
		rf.SerDword("VertexCount"	,*(dword*)&r->VertexCount	,0);
	}

	bool ok = LoadVBIB(vdata,vcount,vsize,FVF,idata,icount,false,attr);
	if(!ok) return false;

	if(ranges.size()>0)
		ok = SUCCEEDED(mesh->SetAttributeTable(&ranges[0],(DWORD)ranges.size()));

	if(!ok)
		Clear(true);

	return ok;
}


bool DevMesh::LoadVBIB(const void *vdata,int vcount,int vsize,int FVF,int *idata,int icount,bool optimize,DWORD *attr)
{
	Clear(true);

	if(vsize<=0 || !vdata || vcount<=0 || !idata || icount<=0)
		return false;

	HRESULT res = D3D_OK;
	void *data;

	do {
		res = D3DXCreateMeshFVF(icount/3,vcount,D3DXMESH_MANAGED | D3DXMESH_32BIT,FVF,Dev.GetDevice(),&mesh);
		if(FAILED(res) || !mesh) break;

		// build vertexes
		res = mesh->LockVertexBuffer(0,&data);
		if(FAILED(res)) break;
		memcpy(data,vdata,vsize*vcount);
		res = mesh->UnlockVertexBuffer();
		if(FAILED(res)) break;

		// build indexes
		res = mesh->LockIndexBuffer(0,&data);
		if(FAILED(res)) break;
		memcpy(data,idata,4*icount);
		res = mesh->UnlockIndexBuffer();
		if(FAILED(res)) break;

		// build attributes
		res = mesh->LockAttributeBuffer(0,(DWORD**)&data);
		if(FAILED(res)) break;
		if(attr)	memcpy(data,attr,sizeof(DWORD)*icount/3);
		else		memset(data,0,sizeof(DWORD)*icount/3);
		res = mesh->UnlockAttributeBuffer();
		if(FAILED(res)) break;

		// optimize
		if(optimize)
		{
//			ID3DXMesh *m2;
//			res = mesh->CloneMeshFVF(D3DXMESH_MANAGED | D3DXMESH_32BIT,FVF,Dev.GetDevice(),&m2);
//			if(FAILED(res)) break;
//			mesh->Release();
//			mesh = m2;

			GenerateAdjacency(0.f);
			res = mesh->OptimizeInplace(
				D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT |
				D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_DEVICEINDEPENDENT,
				adjacency,adjacency,NULL,NULL);
			Clear(false);
			
			if(FAILED(res)) break;
		}

	} while(0);

	if(FAILED(res))
	{
		Clear(true);
		return false;
	}

	return true;
}

bool DevMesh::Save(const char *path)
{
	if(mesh && FilePathGetPart(path,false,false,true)==".x")
		return SUCCEEDED( D3DXSaveMeshToX(path,mesh,0,0,0,0,D3DXF_FILEFORMAT_TEXT) );

	if(!ReadBack())
		return false;

	TreeFileBuilder tfb;
	TreeFileRef root = tfb.GetRoot(true);
	TreeFileRef f_vb = root.SerChild("VertexBuffer");
	TreeFileRef f_ib = root.SerChild("IndexBuffer");
	TreeFileRef f_ab = root.SerChild("AttrBuffer");

	// vertexes
	f_vb.SerInt("VertexSize",vsize,vsize);
	f_vb.Write_SetRaw("Data",vdata,vsize*vcount);
	int FVF = mesh->GetFVF();
	f_vb.SerInt("FVF",FVF,FVF);

	// indexes
	f_ib.Write_SetRaw("Data",idata,icount*sizeof(int));

	// attributes
	DWORD *abuffer = NULL;
	if(SUCCEEDED(mesh->LockAttributeBuffer(0,&abuffer)))
		f_ab.Write_SetRaw("Data",abuffer,mesh->GetNumFaces());

	vector<D3DXATTRIBUTERANGE> ranges;
	DWORD arcount = 0;
	if(SUCCEEDED(mesh->GetAttributeTable(NULL,&arcount)))
	{
		ranges.resize(arcount);
		if(ranges.size()>0 && SUCCEEDED(mesh->GetAttributeTable(&ranges[0],NULL)))
		{
			for(int i=0;i<(int)ranges.size();i++)
			{
				D3DXATTRIBUTERANGE *r = &ranges[i];
				TreeFileRef rf = f_ab.SerChild("Range",i);
				rf.SerDword("AttribId"		,*(dword*)&r->AttribId		,0);
				rf.SerDword("FaceStart"		,*(dword*)&r->FaceStart		,0);
				rf.SerDword("FaceCount"		,*(dword*)&r->FaceCount		,0);
				rf.SerDword("VertexStart"	,*(dword*)&r->VertexStart	,0);
				rf.SerDword("VertexCount"	,*(dword*)&r->VertexCount	,0);
			}
		}
	}

	// save
	FileWriterStream file(path);
	bool ok = tfb.SaveTreeBin(&file);

	if(abuffer)
		mesh->UnlockAttributeBuffer();

	Clear(false);

	return ok;
}

bool DevMesh::LoadCube(float w,float h,float d)
{
    Clear(true);

    return SUCCEEDED( D3DXCreateBox(Dev.GetDevice(),w,h,d,&mesh,NULL) );
}

bool DevMesh::LoadSphere(float r,int slices,int stacks)
{
    Clear(true);

    return SUCCEEDED( D3DXCreateSphere(Dev.GetDevice(),r,slices,stacks,&mesh,NULL) );
}

bool DevMesh::LoadCylinder(float r1,float r2,float len,int slices,int stacks)
{
    Clear(true);

    return SUCCEEDED( D3DXCreateCylinder(Dev.GetDevice(),r1,r2,len,slices,stacks,&mesh,NULL) );
}

bool DevMesh::LoadTorus(float rin,float rout,int sides,int rings)
{
    Clear(true);

    return SUCCEEDED( D3DXCreateTorus(Dev.GetDevice(),rin,rout,sides,rings,&mesh,NULL) );
}

bool DevMesh::LoadPolygon(float len,int sides)
{
    Clear(true);

    return SUCCEEDED( D3DXCreatePolygon(Dev.GetDevice(),len,sides,&mesh,NULL) );
}

void DevMesh::Clear(bool all)
{
	if(mesh && all)
	{
		mesh->Release();
		mesh = NULL;
	}
	if(adjacency) delete adjacency;
	if(vdata) delete vdata;
	if(idata) delete idata;
	if(adata) delete adata;
	adjacency = NULL;
	vdata = NULL;
	idata = NULL;
	adata = NULL;
}

void DevMesh::GenerateAdjacency(float epsilon)
{
	if(adjacency)
		delete adjacency;
	adjacency = NULL;
	if(!mesh || mesh->GetNumFaces()<=0)
		return;
	adjacency = new DWORD[3*mesh->GetNumFaces()];
	mesh->GenerateAdjacency(epsilon,adjacency);
}

void DevMesh::GenerateSoftAdjacency(float epsilon,float normal_dot,bool normal_dir)
{
	if(!mesh) return;
	if(normal_dot<=-1)
	{
		GenerateAdjacency(epsilon);
		return;
	}

	ReadBack();
	GenerateAdjacency(epsilon);

	for(int i=0;i<icount;i++)
		if(adjacency[i]>=0)
		{
			int base[2] = { (i/3) * 3, adjacency[i] * 3 };
			vec3 normal[2];

			for(int j=0;j<2;j++)
			{
				vec3 *v0 = (vec3*)(((byte*)vdata) + idata[base[j]  ]*vsize);
				vec3 *v1 = (vec3*)(((byte*)vdata) + idata[base[j]+1]*vsize);
				vec3 *v2 = (vec3*)(((byte*)vdata) + idata[base[j]+2]*vsize);
				normal[j] = (*v1-*v0).cross(*v2-*v0);
				normal[j].normalize();
				if(normal_dir)
					normal[j] *= -1;
			}

			float dot = normal[0].dot(normal[1]);
			if(dot<normal_dot)
				adjacency[i] = -1;
		}
}


bool DevMesh::ReadBack()
{
	Clear(false);
    if(!mesh) return false;

	HRESULT res = D3D_OK;
	void *p;
	vcount = mesh->GetNumVertices();
	vsize = mesh->GetNumBytesPerVertex();
	icount = mesh->GetNumFaces()*3;

	if(vcount<=0 || icount<=0)
		return false;

	do {
		vdata = new byte[vcount*vsize];
		res = mesh->LockVertexBuffer(0,(void**)&p);
		if(FAILED(res)) break;
		memcpy(vdata,p,vcount*vsize);
		res = mesh->UnlockVertexBuffer();
		if(FAILED(res)) break;

		idata = new int[icount];
		res = mesh->LockIndexBuffer(0,(void**)&p);
		if(FAILED(res)) break;
		memcpy(idata,p,icount*sizeof(int));
		res = mesh->UnlockIndexBuffer();
		if(FAILED(res)) break;

		adata = new DWORD[icount/3];
		res = mesh->LockAttributeBuffer(0,(DWORD**)&p);
		if(FAILED(res)) break;
		memcpy(adata,p,icount/3*sizeof(DWORD));
		res = mesh->UnlockAttributeBuffer();
		if(FAILED(res)) break;

	} while(0);

	if(FAILED(res))
	{
		Clear(false);
		return false;
	}

	return true;
}

bool DevMesh::Upload()
{
	HRESULT res = D3D_OK;
	void *p;

	if(!mesh || !vdata || !idata) return false;

	do {
		res = mesh->LockVertexBuffer(0,(void**)&p);
		if(FAILED(res)) break;
		memcpy(p,vdata,vcount*vsize);
		res = mesh->UnlockVertexBuffer();
		if(FAILED(res)) break;

		res = mesh->LockIndexBuffer(0,(void**)&p);
		if(FAILED(res)) break;
		memcpy(p,idata,icount*sizeof(int));
		res = mesh->UnlockIndexBuffer();
		if(FAILED(res)) break;

		if(adata)
		{
			res = mesh->LockAttributeBuffer(0,(DWORD**)&p);
			if(FAILED(res)) break;
			memcpy(p,adata,icount/3*sizeof(DWORD));
			res = mesh->UnlockAttributeBuffer();
			if(FAILED(res)) break;
		}
	} while(0);

	return SUCCEEDED(res);
}

void DevMesh::ApplyMatrixInMemory(D3DXMATRIX *mtx)
{
	if(!vdata) return;
	D3DXVec3TransformCoordArray(
		(D3DXVECTOR3*)vdata,vsize,
		(D3DXVECTOR3*)vdata,vsize,
		mtx,vcount);
}

void DevMesh::FlipTrisInMemory()
{
	if(!idata) return;
	for(int i=0;i<icount;i+=3)
	{
		int t = idata[i+1];
		idata[i+1] = idata[i+2];
		idata[i+2] = t;
	}
}

bool DevMesh::ComputeTangentFrame(tMeshAttrib tex_attr,tMeshAttrib normal_attr,
								tMeshAttrib tangent_attr,tMeshAttrib binormal_attr,
								float dot_normal,float dot_tangent,float eps_singular,int options)
{
	if(!mesh) return false;

	options |= D3DXTANGENT_WEIGHT_BY_AREA;// | D3DXTANGENT_GENERATE_IN_PLACE;
	if(!ATTR_IS_PRESENT(tex_attr)) options |= D3DXTANGENT_CALCULATE_NORMALS;

	ID3DXMesh *new_mesh = NULL;
	HRESULT res = D3DXComputeTangentFrameEx(
		mesh,														// mesh
		ATTR_GET_USAGE(tex_attr),		ATTR_GET_ID(tex_attr),		// input texcoord
		ATTR_GET_USAGE(tangent_attr),	ATTR_GET_ID(tangent_attr),	// out tangent
		ATTR_GET_USAGE(binormal_attr),	ATTR_GET_ID(binormal_attr),	// out binormal
		ATTR_GET_USAGE(normal_attr),	ATTR_GET_ID(normal_attr),	// out normal
		options,						// options
		adjacency,						// adjacency
		dot_tangent,					// tangent/binormal merge treshold
		eps_singular,					// singular point treshold
		dot_normal,						// normal threshold
		&new_mesh,						// mesh out
		NULL							// vertex remap out
		);

	assert(res!=D3DERR_INVALIDCALL);
	assert(res!=D3DXERR_INVALIDDATA);
	assert(res!=E_OUTOFMEMORY);

	if(new_mesh)
	{
		mesh->Release();
		mesh = new_mesh;
	}

	Clear(false);

	return SUCCEEDED(res);
}

void DevMesh::GenerateDegenerateEdges(int flags,tMeshAttrib normal_attr,DevMesh *out)
{
	if(!mesh) return;

	ReadBack();
	GenerateAdjacency(0);

	vector<byte> nvdata;
	nvdata.resize(icount*vsize);

	for(int i=0;i<icount;i++)
		memcpy(&nvdata[i*vsize],&((byte*)vdata)[idata[i]*vsize],vsize);

	if(ATTR_IS_PRESENT(normal_attr))
	{
		D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];
		mesh->GetDeclaration(decl);

		int offs = -1;
		for(int i=0;decl[i].Stream!=0xFF;i++)
			if(decl[i].Usage==ATTR_GET_USAGE(normal_attr) && decl[i].UsageIndex==ATTR_GET_ID(normal_attr))
				offs = decl[i].Offset;

		if(offs>=0)
		{
			for(int i=0;i<icount;i+=3)
			{
				D3DXVECTOR3 d1 = *(D3DXVECTOR3*)&nvdata[(i+1)*vsize] - *(D3DXVECTOR3*)&nvdata[i*vsize];
				D3DXVECTOR3 d2 = *(D3DXVECTOR3*)&nvdata[(i+2)*vsize] - *(D3DXVECTOR3*)&nvdata[i*vsize];
				D3DXVECTOR3 n;
				D3DXVec3Cross(&n,&d1,&d2);
				D3DXVec3Normalize(&n,&n);
				*(D3DXVECTOR3*)&nvdata[ i   *vsize+offs] = n;
				*(D3DXVECTOR3*)&nvdata[(i+1)*vsize+offs] = n;
				*(D3DXVECTOR3*)&nvdata[(i+2)*vsize+offs] = n;
			}
		}
	}


	vector<int> nidata;
	nidata.resize(icount);

	for(int i=0;i<icount;i++)
		nidata[i] = i;

	for(int i=0;i<icount;i++)
	{
		if(adjacency[i]==0xFFFFFFFF) continue;
//		if(adjacency[i]<=i) continue;
		int j = -1;
		for(int k=0;k<3;k++)
			if(adjacency[adjacency[i]*3+k]==i/3)
				j = adjacency[i]*3+k;
		if(j<0) continue;
		int a0 = i, b0 = j;
		int a1 = a0 - (a0%3) + ((a0+1)%3);
		int b1 = b0 - (b0%3) + ((b0+1)%3);
		nidata.push_back(a0);
		nidata.push_back(b1);
		nidata.push_back(a1);
		nidata.push_back(a1);
		nidata.push_back(b1);
		nidata.push_back(b0);
	}

	out->LoadVBIB(&nvdata[0],(int)nvdata.size()/vsize,vsize,mesh->GetFVF(),&nidata[0],(int)nidata.size());
}


static int _decltype_to_size(int decl)
{
	if(decl==D3DDECLTYPE_FLOAT1) return sizeof(float);
	if(decl==D3DDECLTYPE_FLOAT2) return 2*sizeof(float);
	if(decl==D3DDECLTYPE_FLOAT3) return 3*sizeof(float);
	if(decl==D3DDECLTYPE_FLOAT4) return 4*sizeof(float);
	if(decl==D3DDECLTYPE_D3DCOLOR) return sizeof(DWORD);
	return 0;
}

bool DevMesh::ReorderVertexFields(int FVF,tMeshAttrib attr_map[][2])
{
	if(!mesh) return false;

	ReadBack();

	HRESULT res = D3D_OK;
	void *data;
	ID3DXMesh *m2 = NULL;

	do {
		res = D3DXCreateMeshFVF(icount/3,vcount,D3DXMESH_MANAGED | D3DXMESH_32BIT,FVF,Dev.GetDevice(),&m2);
		if(FAILED(res)) break;

		// read declarations
		D3DVERTEXELEMENT9 srcd[MAX_FVF_DECL_SIZE], dstd[MAX_FVF_DECL_SIZE];
		mesh->GetDeclaration(srcd);
		m2->GetDeclaration(dstd);

		// build copy table
		int nvsize = m2->GetNumBytesPerVertex();
		int copy_offs[1024];
		memset(copy_offs,-1,sizeof(copy_offs));

		if(attr_map)
		{
			for(int i=0;ATTR_IS_PRESENT(attr_map[i][0]);i++)
			{
				int doffs=-1,soffs=-1,size=0;
				for(int j=0;dstd[j].Stream!=0xFF;j++)
					if(dstd[j].Usage==ATTR_GET_USAGE(attr_map[i][0]) && dstd[j].UsageIndex==ATTR_GET_ID(attr_map[i][0]))
					{
						doffs = dstd[j].Offset;
						size = _decltype_to_size(dstd[j].Type);
					}
				for(int j=0;srcd[j].Stream!=0xFF;j++)
					if(srcd[j].Usage==ATTR_GET_USAGE(attr_map[i][1]) && srcd[j].UsageIndex==ATTR_GET_ID(attr_map[i][0]))
						soffs = srcd[j].Offset;
				if(doffs>=0 && soffs>=0 && size>0)
					for(int j=0;j<size/4;j++)
						copy_offs[doffs/4+j] = soffs/4+j;
			}
		}
		else
		{
			for(int i=0;dstd[i].Stream!=0xFF;i++)
			{
				int doffs=-1,soffs=-1,size=0;
				for(int j=0;srcd[j].Stream!=0xFF;j++)
					if(dstd[i].Usage==srcd[j].Usage && dstd[i].UsageIndex==srcd[i].UsageIndex)
					{
						doffs = dstd[j].Offset;
						soffs = srcd[j].Offset;
						size = _decltype_to_size(dstd[j].Type);
						break;
					}

				if(doffs>=0 && soffs>=0 && size>0)
					for(int j=0;j<size/4;j++)
						copy_offs[doffs/4+j] = soffs/4+j;
			}
		}

		// build vertexes
		res = m2->LockVertexBuffer(0,&data);
		if(FAILED(res)) break;
		for(int i=0;i<vcount;i++)
			for(int j=0;j<nvsize/4;j++)
				if(copy_offs[j]>=0)
					*(((DWORD*)data)+(i*nvsize/4+j)) = *(((DWORD*)vdata)+(i*vsize/4+copy_offs[j]));
				else
					*(((DWORD*)data)+(i*nvsize/4+j)) = 0;
		res = m2->UnlockVertexBuffer();
		if(FAILED(res)) break;

		// build indexes
		res = m2->LockIndexBuffer(0,&data);
		if(FAILED(res)) break;
		memcpy(data,idata,4*icount);
		res = m2->UnlockIndexBuffer();
		if(FAILED(res)) break;

		// build attributes
		res = m2->LockAttributeBuffer(0,(DWORD**)&data);
		if(FAILED(res)) break;
		memset(data,0,sizeof(DWORD)*icount/3);
		res = m2->UnlockAttributeBuffer();
		if(FAILED(res)) break;
	} while(0);

	if(FAILED(res))
	{
		if(m2) m2->Release();
		m2 = NULL;
		return false;
	}

	Clear(true);
	mesh = m2;

	return true;
}


bool DevMesh::UnwrapUV(float max_stretch,int tex_w,int tex_h,float gutter,int tex_id,float normal_dot,bool normal_dir)
{
	if(!mesh) return false;

	ID3DXMesh *new_mesh = NULL;

	GenerateSoftAdjacency(0.f,normal_dot,normal_dir);

	HRESULT res = D3DXUVAtlasCreate(
		mesh,					// mesh
		0,						// max charts
		max_stretch,			// max stretch
		tex_w, tex_h,			// texture size
		gutter,					// gutter
		tex_id,					// texture index
		adjacency,				// adjacency
		NULL,					// false edges
		NULL,					// IMT array
		NULL,					// callback
		0,						// callback frequency
		NULL,					// callback param
		D3DXUVATLAS_DEFAULT,	// options
		&new_mesh,				// out mesh
		NULL,					// face partitioning
		NULL,					// vertex remap
		NULL,					// max stretch out
		NULL					// num charts out
	);

	assert(res!=D3DERR_INVALIDCALL);
	assert(res!=D3DXERR_INVALIDDATA);
	assert(res!=E_OUTOFMEMORY);

	if(SUCCEEDED(res) && new_mesh)
	{
		mesh->Release();
		mesh = new_mesh;
	}

	Clear(false);

	return SUCCEEDED(res);
}

bool DevMesh::CleanMesh()
{
	if(GetIndexCount()<3)
		return true;

	ID3DXMesh *new_mesh = NULL;

	GenerateAdjacency(0);

	ID3DXBuffer *err = NULL;
	HRESULT res = D3DXCleanMesh(
		D3DXCLEAN_SIMPLIFICATION,	// flags
		mesh,						// mesh
		adjacency,					// adjacency
		&new_mesh,					// new mesh
		adjacency,					// out adjacency
		&err						// errors
	);

	assert(res!=D3DERR_INVALIDCALL);
	assert(res!=D3DXERR_INVALIDDATA);
	assert(res!=E_OUTOFMEMORY);

	if(err)
		err->Release();

	if(SUCCEEDED(res) && new_mesh)
	{
		mesh->Release();
		mesh = new_mesh;
	}

	Clear(false);

	return SUCCEEDED(res);
}

bool DevMesh::Optimize()
{
	if(!mesh) return true;
	GenerateAdjacency(0);
	return SUCCEEDED(mesh->OptimizeInplace(
				D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_DONOTSPLIT,
				adjacency,adjacency,NULL,NULL));
}

bool DevMesh::Simplify(int size,bool count_faces)
{
	if(GetIndexCount()<3)
		return true;

	ID3DXMesh *new_mesh = NULL;

	GenerateAdjacency(0);

	HRESULT res = D3DXSimplifyMesh(
		mesh,													// mesh
		adjacency,												// adjacency
		NULL,													// attribute weights
		NULL,													// vertex weights
		size,													// min value
		count_faces ? D3DXMESHSIMP_FACE : D3DXMESHSIMP_VERTEX,	// options
		&new_mesh												// new mesh
	);

	assert(res!=D3DERR_INVALIDCALL);
	assert(res!=D3DXERR_INVALIDDATA);
	assert(res!=E_OUTOFMEMORY);

	if(SUCCEEDED(res) && new_mesh)
	{
		mesh->Release();
		mesh = new_mesh;
	}

	Clear(false);

	return SUCCEEDED(res);
}

int DevMesh::GetVertexStride()
{
	if(!mesh) return 0;
	return mesh->GetNumBytesPerVertex();
}

int	DevMesh::GetVertexCount()
{
	if(!mesh) return 0;
	return mesh->GetNumVertices();
}

bool DevMesh::CopyVertexData(void *buffer)
{
	void *data;
	if(!mesh) return false;
	HRESULT res = mesh->LockVertexBuffer(0,&data);
	if(FAILED(res)) return false;
	memcpy(buffer,data,GetVertexDataSize());
	res = mesh->UnlockVertexBuffer();
	if(FAILED(res)) return false;
	return true;
}

int DevMesh::GetIndexCount()
{
	if(!mesh) return 0;
	return mesh->GetNumFaces()*3;
}

bool DevMesh::CopyIndexData(int *buffer)
{
	void *data;
	if(!mesh) return false;
	HRESULT res = mesh->LockIndexBuffer(0,&data);
	if(FAILED(res)) return false;
	memcpy(buffer,data,4*GetIndexCount());
	res = mesh->UnlockIndexBuffer();
	if(FAILED(res)) return false;
	return true;
}

int DevMesh::GetRangeCount()
{
	if(!mesh) return 0;
	DWORD size = 0;
	if(FAILED(mesh->GetAttributeTable(NULL,&size)))
		return 0;
	return size;
}

bool DevMesh::CopyRangeData(DevMesh::Range *buffer)
{
	if(!mesh) return false;
	return SUCCEEDED(mesh->GetAttributeTable((D3DXATTRIBUTERANGE*)buffer,NULL));
}

void DevMesh::DrawSection(int id)
{
	if(mesh)
		mesh->DrawSubset(id);
}

void DevMesh::DrawRange(const Range &r)
{
	if(!mesh) return;
	DWORD fvf = mesh->GetFVF();
	IDirect3DVertexBuffer9 *vb = NULL;
	IDirect3DIndexBuffer9 *ib = NULL;
	mesh->GetVertexBuffer(&vb);
	mesh->GetIndexBuffer(&ib);

	if(fvf && vb && ib)
	{
		Dev->SetFVF(fvf);
		Dev->SetStreamSource(0,vb,0,mesh->GetNumBytesPerVertex());
		Dev->SetIndices(ib);
		Dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,r.vtx_start,r.vtx_count,r.face_start*3,r.face_count);
	}

	if(vb) vb->Release();
	if(ib) ib->Release();
}

void DevMesh::DrawTriangleRange(int first_tri,int num_tris)
{
	if(!mesh) return;
	DWORD fvf = mesh->GetFVF();
	IDirect3DVertexBuffer9 *vb = NULL;
	IDirect3DIndexBuffer9 *ib = NULL;
	mesh->GetVertexBuffer(&vb);
	mesh->GetIndexBuffer(&ib);

	if(fvf && vb && ib)
	{
		Dev->SetFVF(fvf);
		Dev->SetStreamSource(0,vb,0,mesh->GetNumBytesPerVertex());
		Dev->SetIndices(ib);
		Dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,mesh->GetNumVertices(),first_tri*3,num_tris);
	}

	if(vb) vb->Release();
	if(ib) ib->Release();
}

void DevMesh::OnBeforeReset()
{
}

void DevMesh::OnAfterReset()
{
}

void DevMesh::OnPreload()
{
	if(preload_path.size()>0)
		Load(preload_path.c_str());
}

/*
void DevMesh::MakeDeclFromFVFCode(int FVF,int &decl,int &id)
{
	if(FVF==D3DFVF_XYZ		) { decl = D3DDECLUSAGE_POSITION;	id = 0; return; }
	if(FVF==D3DFVF_NORMAL	) { decl = D3DDECLUSAGE_NORMAL;		id = 0;	return; }
	if(FVF==D3DFVF_PSIZE	) { decl = D3DDECLUSAGE_PSIZE;		id = 0;	return; }
	if(FVF==D3DFVF_TEX1		) { decl = D3DDECLUSAGE_TEXCOORD;	id = 0;	return; }
	if(FVF==D3DFVF_TEX2		) { decl = D3DDECLUSAGE_TEXCOORD;	id = 1;	return; }
	if(FVF==D3DFVF_TEX3		) { decl = D3DDECLUSAGE_TEXCOORD;	id = 2;	return; }
	if(FVF==D3DFVF_TEX4		) { decl = D3DDECLUSAGE_TEXCOORD;	id = 3;	return; }
	if(FVF==D3DFVF_TEX5		) { decl = D3DDECLUSAGE_TEXCOORD;	id = 4;	return; }
	if(FVF==D3DFVF_TEX6		) { decl = D3DDECLUSAGE_TEXCOORD;	id = 5;	return; }
	if(FVF==D3DFVF_TEX7		) { decl = D3DDECLUSAGE_TEXCOORD;	id = 6;	return; }
	if(FVF==D3DFVF_TEX8		) { decl = D3DDECLUSAGE_TEXCOORD;	id = 7;	return; }
	decl = D3DX_DEFAULT;
	id = 0;
}
*/
