
#include "dxfw.h"

using namespace std;
using namespace base;



static bool _load_scrambled(const char *path,vector<byte> &data)
{
	if(!NFS.GetFileBytes(path,data))
		return false;

	unsigned int key = 1716253875;
	for(int i=0;i<(int)data.size();i++)
	{
		data[i] ^= (key>>24);
		key = key*134775813 + 13;
	}

	return true;
}

static bool _save_scrambled(const char *path,const vector<byte> &data)
{
	vector<byte> cdata = data;
	
	if(cdata.size()<=0)
		return false;

	unsigned int key = 1716253875;
	for(int i=0;i<(int)cdata.size();i++)
	{
		cdata[i] ^= (key>>24);
		key = key*134775813 + 13;
	}

	return NFS.DumpRawVector(path,cdata);
}



// ----------------


DevShaderSet::DevShaderSet(const char *path,DefGenerator *dg,const char *v)
{
	def_generator = dg;
	version = v;

	if(Dev.GetIsReady())	Load(path,dg);
	else					p_path = path;
}

DevShaderSet::~DevShaderSet()
{
	if( final_save_path.size()>0 )
		SaveShaderCache( final_save_path.c_str() );

	ClearAll();
}

bool DevShaderSet::Load(const char *path,DefGenerator *dg)
{
	// clear
	ClearAll();

	// load file
	if(!ScrambleLoad(path,source))
		return false;
	original_path = path;

	// build defines
	if(dg)	dg(defs);
	else	defs.push_back(":");

	// hash
	MD5Hash md5;
	byte hash[16];
	md5.Update((byte*)&source[0],(int)source.size());
	for(int i=0;i<(int)defs.size();i++)
		md5.Update((byte*)defs[i].c_str(),(int)defs[i].size()+1);
	md5.Final(hash);

	// load compiled binaries
	string cpath = FilePathGetPart((string("shaders/")+path).c_str(),true,true,false) + ".psc";
	if(!LoadShaderCache( cpath.c_str(), hash ) )
		CreateShaderCache( defs, hash );

	// shatter defs
	for(int i=0;i<(int)defs.size();i++)
	{
		macro_start.push_back((DWORD)macro_ptrs.size());
		ShatterDefsList(&defs[i][0],macro_ptrs);
	}

	// compile
	for(int i=0;i<(int)defs.size();i++)
		CompileVersion(i,false);

	// save compiled binaries
	SaveShaderCache( cpath.c_str() );
	final_save_path = cpath;

	return false;
}

bool DevShaderSet::BindVersionStr(const char *version)
{
	map<string,int>::iterator p;
	p = vmap.find(version);
	if( p==vmap.end() )
		return false;

	return BindVersion(p->second);
}

bool DevShaderSet::BindVersion(int version)
{
	if( version<0 || version>=(int)versions.size() )
		return false;

	ShaderSet *v = &versions[version];
	if( !v->vshader || !v->pshader )
		CompileVersion( version, true );

	Dev->SetVertexShader( v->vshader );
	Dev->SetPixelShader( v->pshader );

	return true;
}

void DevShaderSet::Unbind()
{
	Dev->SetVertexShader( NULL );
	Dev->SetPixelShader( NULL );
}

int	DevShaderSet::GetVersionIndex(const char *version)
{
	map<string,int>::iterator p;
	p = vmap.find(version);
	if( p==vmap.end() )
		return -1;

	return p->second;
}


void DevShaderSet::OnBeforeReset()
{
	ReleaseAll();
}

void DevShaderSet::OnAfterReset()
{
}

void DevShaderSet::ClearAll()
{
	ReleaseAll();

	source.clear();
	defs.clear();
	macro_ptrs.clear();
	macro_start.clear();
	original_path.clear();

	raw_file.clear();
	versions.clear();
	vmap.clear();
}

void DevShaderSet::ReleaseAll()
{
	for(int i=0;i<(int)versions.size();i++)
	{
		if(versions[i].vshader) versions[i].vshader->Release();
		if(versions[i].pshader) versions[i].pshader->Release();
		versions[i].vshader = NULL;
		versions[i].pshader = NULL;
	}
}

bool DevShaderSet::ScrambleLoad(const char *path,vector<char> &data)
{
	bool scrambled = false;
	if(!NFS.GetFileBytes(path,*(vector<byte>*)&data) || data.size()==0)
	{
		if(!_load_scrambled(format("shaders/%ss",path).c_str(),*(vector<byte>*)&data) || data.size()==0)
		{
			if(MessageBox(NULL,format("Can't find file %s!",path).c_str(),path,MB_OKCANCEL)==IDCANCEL)
				ExitProcess(0);
			return false;
		}
		else
			scrambled = true;
	}

	if(!scrambled)
		_save_scrambled(format("shaders/%ss",path).c_str(),*(vector<byte>*)&data);

	data.push_back(0);

	return true;
}

void DevShaderSet::ShatterDefsList(char *s,vector<D3DXMACRO> &macros)
{
	// parse version identifier
	const char *id = s;
	while(*s && *s!=':') s++;

	// parse macros
	while(*s)
	{
		D3DXMACRO m = { NULL, NULL };

		// terminate previous string
		*s++ = 0;
		
		// read name
		m.Name = s;
		while(*s && *s!='=') s++;
		if(!*s) break;
		*s++ = 0;

		// read definition
		m.Definition = s;
		while(*s && *s!=';') s++;

		macros.push_back(m);
	}

	// terminate macro list
	D3DXMACRO m = { NULL, NULL };
	macros.push_back(m);
}

void DevShaderSet::CreateShaderCache( vector<string> &defs, byte hash[16] )
{
	raw_file.clear();
	versions.clear();
	vmap.clear();

	int nv = (int)defs.size();
	versions.resize(nv);

	raw_file.assign( hash, hash+16 );
	raw_file.insert( raw_file.end(), (byte*)&nv, (byte*)(&nv+1) );
	if(nv>0)
		raw_file.insert( raw_file.end(), (byte*)&versions[0], (byte*)(&versions[0] + versions.size()) );

	for(int i=0;i<(int)versions.size();i++)
	{
		const char *s = defs[i].c_str();

		versions[i].name_ofs = (int)raw_file.size();
		while(*s && *s!=':')
			raw_file.push_back(*s++);
		raw_file.push_back(0);

		versions[i].vcode_ofs = 0;
		versions[i].pcode_ofs = 0;
		versions[i].vshader = NULL;
		versions[i].pshader = NULL;

		vmap[ (char *)&raw_file[versions[i].name_ofs] ] = i;
	}
}

bool DevShaderSet::LoadShaderCache( const char *path, byte hash[16] )
{
	raw_file.clear();
	versions.clear();
	vmap.clear();

	if(!_load_scrambled( path, raw_file ) || raw_file.size()<16+sizeof(int))
		return false;

	byte *raw = &raw_file[0];
	int nversions = *(int*)(raw+16);
	if( raw_file.size() < 16 + sizeof(int) + nversions*sizeof(ShaderSet) )
		return false;

	if( memcmp(raw,hash,16) != 0 )
		return false;

	ShaderSet *ss = (ShaderSet*)(raw+16+sizeof(int));
	versions.assign( ss, ss+nversions );

	for(int i=0;i<(int)versions.size();i++)
	{
		if(versions[i].name_ofs  >= raw_file.size()) versions[i].name_ofs  = 0xFFFFFFFF;
		if(versions[i].vcode_ofs >= raw_file.size()) versions[i].vcode_ofs = 0xFFFFFFFF;
		if(versions[i].pcode_ofs >= raw_file.size()) versions[i].pcode_ofs = 0xFFFFFFFF;
		versions[i].vshader = NULL;
		versions[i].pshader = NULL;

		if(versions[i].name_ofs != 0xFFFFFFFF)
			vmap[ (char *)&raw_file[versions[i].name_ofs] ] = i;
	}

	return true;
}

bool DevShaderSet::SaveShaderCache( const char *path )
{
	if(versions.size()>=1)
		memcpy( &raw_file[16+sizeof(int)], &versions[0], sizeof(ShaderSet)*versions.size() );

	if(!_save_scrambled( path, raw_file ))
		return false;

	return true;
}

void DevShaderSet::CompileVersion( int ver, bool create_shaders )
{
	if( ver<0 || ver>=(int)versions.size() || source.size()<=0 )
		return;

	ShaderSet *v = &versions[ver];
	char profile[20];

	for(int type=0;type<2;type++)
	{
		ID3DXBuffer *buffer = NULL, *errors = NULL;
		HRESULT res;
		DWORD *code_ofs = type ? &v->pcode_ofs : &v->vcode_ofs;
		const char *entry = type ? "pmain" : "vmain";
		sprintf_s(profile,20,"%s_%s",type?"ps":"vs",version);

		if(*code_ofs != 0)
			continue;

		res = D3DXCompileShader(
					&source[0], (DWORD)source.size(),
					&macro_ptrs[macro_start[ver]], NULL, entry, profile,
					D3DXSHADER_AVOID_FLOW_CONTROL, &buffer, &errors, NULL );
		if( SUCCEEDED(res) && buffer )
		{
			byte *bp = (byte*) buffer->GetBufferPointer();

			*code_ofs = (DWORD)raw_file.size();
			raw_file.insert( raw_file.end(), bp, bp + buffer->GetBufferSize() );
		}
		else
		{
			const char *err = "Unknown error!";
			if(errors)
				err = (char*)errors->GetBufferPointer();
			
			const char *def = "?";
			if(ver<(int)defs.size()) def = defs[ver].c_str();
			if(MessageBox(NULL,err,format("%s:%s:%cs",original_path.c_str(),def,"vp"[type]).c_str(),MB_OKCANCEL)==IDCANCEL)
				ExitProcess(0);

			*code_ofs = 0xFFFFFFFF;
		}

		if(buffer) buffer->Release();
		if(errors) errors->Release();
	}

	if( create_shaders )
	{
		if(v->vcode_ofs != 0xFFFFFFFF && v->pshader==NULL)
		{
			if(FAILED(Dev->CreateVertexShader((DWORD*)&raw_file[v->vcode_ofs],&v->vshader)))
			{
				const char *def = "?";
				if(ver<(int)defs.size()) def = defs[ver].c_str();
				if(MessageBox(NULL,"Internal error!",format("%s:%s:%vs",original_path.c_str(),def).c_str(),MB_OKCANCEL)==IDCANCEL)
					ExitProcess(0);
				v->vcode_ofs = 0xFFFFFFFF;
			}
		}

		if(v->pcode_ofs != 0xFFFFFFFF && v->pshader==NULL)
		{
			if(FAILED(Dev->CreatePixelShader((DWORD*)&raw_file[v->pcode_ofs],&v->pshader)))
			{
				const char *def = "?";
				if(ver<(int)defs.size()) def = defs[ver].c_str();
				if(MessageBox(NULL,"Internal error!",format("%s:%s:%ps",original_path.c_str(),def).c_str(),MB_OKCANCEL)==IDCANCEL)
					ExitProcess(0);
				v->pcode_ofs = 0xFFFFFFFF;
			}
		}
	}
}
