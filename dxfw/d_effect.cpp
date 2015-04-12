
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


// **************** DevEffect ****************


DevEffect::DevEffect(const char *path,DevEffect::DefGenerator *dg,int _flags) : fx(NULL), pool(NULL)
{
	do_preload = false;
	pass = -2;
	def_generator = dg;
	flags = _flags;
	file_time = 0;

	compile_flags = D3DXSHADER_AVOID_FLOW_CONTROL;
	if( flags & FXF_FLOW_CONTROL_MEDIUM )	compile_flags &= ~D3DXSHADER_AVOID_FLOW_CONTROL;
	if( flags & FXF_FLOW_CONTROL_HIGH	)	compile_flags |= D3DXSHADER_PREFER_FLOW_CONTROL;
	if( flags & FXF_PARTIAL_PRECISION	)	compile_flags |= D3DXSHADER_PARTIALPRECISION;
		 if( flags & FXF_OPTIMIZATION_3	)	compile_flags |= D3DXSHADER_OPTIMIZATION_LEVEL3;
	else if( flags & FXF_OPTIMIZATION_0	)	compile_flags |= D3DXSHADER_OPTIMIZATION_LEVEL0;
	else if( flags & FXF_OPTIMIZATION_1	)	compile_flags |= D3DXSHADER_OPTIMIZATION_LEVEL1;
	else if( flags & FXF_OPTIMIZATION_2	)	compile_flags |= D3DXSHADER_OPTIMIZATION_LEVEL2;
    if( flags & FXF_LEGACY              )	compile_flags |= D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;



	if(Dev.GetIsReady())	Load(path,dg);
	else					do_preload = true, p_path = path;
}

bool DevEffect::Load(const char *path,DevEffect::DefGenerator *dg)
{
	// clear effect
	ClearEffect();
	file_time = GetFileTime(path);
	p_path = string(path).c_str();

	// load file
	vector<char> data;
	if(!ScrambleLoad(path,data))
		return false;

	// compute partial hash
	MD5Hash md5;
	md5.Update((byte*)&data[0],(int)data.size());

	// build skip list
	string skip_list;
	BuildSkipList(&data[0],skip_list);

	// build defines
	vector<string> defs;
	if(dg)	dg(defs);
	else	defs.push_back(":");

	// create pool
	if(defs.size()>=2)
	{
		if(FAILED(D3DXCreateEffectPool(&pool)))
		{
			return false;
		}
	}

	// compile
	for(int i=0;i<(int)defs.size();i++)
	{
		vector<D3DXMACRO> macros;
		vector<byte> bin;
		MD5Hash hash = md5;
		byte bhash[16];

		// build final hash
		defs[i].push_back(0);
		hash.Update((byte*)&defs[i][0],(int)defs.size());
		hash.Final(bhash);

		// get macros
		ShatterDefsList(&defs[i][0],macros);

		// build binary path
		string bpath = FilePathGetPart((string("shaders/")+path).c_str(),true,true,false);
		if(defs[i].c_str()[0])
		{
			bpath += "-";
			bpath += defs[i].c_str();
		}
		bpath += ".fxx";

		// precompile effect
		if(!GetPrecompiledBinary(data,defs[i].c_str(),path,bpath.c_str(),macros,bhash,bin))
			return false;

		if(!CompileVersion(defs[i].c_str(),path,bin,macros,skip_list.c_str()))
			return false;
	}

	if(fx_list.size()<=0)
		return false;

	fx = fx_list[0];

	// load textures
	D3DXHANDLE h, ha;
	const char *str;
	int id = 0;
	while( (h = fx->GetParameter(NULL,id)) )
	{
		ha = fx->GetAnnotationByName(h,"Load");
		if(ha)
			if(SUCCEEDED(fx->GetString(ha,&str)))
			{
				DevTexture *t = new DevTexture();
				t->Load(str);
				fx->SetTexture(h,t->GetTexture());
				textures.push_back(t);
			}
		id++;
	}

	return true;
}

const char *DevEffect::GetTechniqueName(int id)
{
	D3DXHANDLE h;
	D3DXTECHNIQUE_DESC desc;
	if(!fx) return NULL;
	h = fx->GetTechnique(id);
	if(!h) return NULL;
	if(SUCCEEDED(fx->GetTechniqueDesc(h,&desc)))
		return desc.Name;
	return NULL;
}

const char *DevEffect::GetTechniqueInfo(const char *name,const char *param)
{
	D3DXHANDLE h, ha;
	const char *str;
	if(!fx) return NULL;
	h = fx->GetTechniqueByName(name);
	if(!h) return "";
	ha = fx->GetAnnotationByName(h,param);
	if(!ha) return "";
	if(SUCCEEDED(fx->GetString(ha,&str)))
		return str;
	return "";
}

bool DevEffect::SelectVersionStr(const char *version)
{
	map<string,int>::iterator p = version_index.find(version);
	if(p==version_index.end()) { fx = NULL; return false; }
	fx = fx_list[p->second];
	return true;
}

bool DevEffect::SelectVersion(int version)
{
	if(version<0 || version>=(int)fx_list.size()) { fx = NULL; return false; }
	fx = fx_list[version];
	return true;
}

int DevEffect::GetVersionIndex(const char *version)
{
	map<string,int>::iterator p = version_index.find(version);
	if(p==version_index.end()) return -1;
	return p->second;
}


bool DevEffect::StartTechnique(const char *name)
{
	if(!fx)
		return false;

	if(FAILED(fx->SetTechnique(name)))
	{
		pass = -2;
		return false;
	}
	pass = -1;

	return true;
}

bool DevEffect::StartPass()
{
	if(!fx || pass==-2)
		return false;

	if(pass<0)	fx->Begin((UINT*)&n_passes,0);
	else		fx->EndPass();
	pass++;

	if(pass>=n_passes)
	{
		fx->End();
		pass = -2;
		return false;
	}

	fx->BeginPass(pass);

	return true;
}

void DevEffect::OnBeforeReset()
{
	for(int i=0;i<(int)fx_list.size();i++)
		fx_list[i]->OnLostDevice();
}

void DevEffect::OnAfterReset()
{
	for(int i=0;i<(int)fx_list.size();i++)
		fx_list[i]->OnResetDevice();
}


void DevEffect::ClearEffect()
{
	for(int i=0;i<(int)fx_list.size();i++)
		fx_list[i]->Release();
	fx_list.clear();
	fx = NULL;
	if(pool)
	{
		pool->Release();
		pool = NULL;
	}
	version_index.clear();
	do_preload = false;
	pass = -2;
	last_error.clear();
}

bool DevEffect::ScrambleLoad(const char *path,vector<char> &data)
{
	bool scrambled = false;
	if(!NFS.GetFileBytes(path,*(vector<byte>*)&data) || data.size()==0)
	{
		if(!_load_scrambled(format("shaders/%ss",path).c_str(),*(vector<byte>*)&data) || data.size()==0)
		{
			last_error = format("Can't find file %s!",path);
			if(!(flags&FXF_NO_ERROR_POPUPS))
				if(MessageBox(NULL,last_error.c_str(),path,MB_OKCANCEL)==IDCANCEL)
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

void DevEffect::BuildSkipList(const char *d,string &skip_list)
{
	while(*d)
	{
		if(d[0]=='/' && d[1]=='*' && d[2]=='$' && d[3]=='*' && d[4]=='/' && d[5]==' ')
		{
			const char *p = d+6;
			if(skip_list.size()>0) skip_list.push_back(';');
			while((*p>='a' && *p<='z') || (*p>='A' && *p<='Z') || (*p>='0' && *p<='9') || *p=='_')
				skip_list.push_back(*p++);
		}
		d++;
	}
}

void DevEffect::ShatterDefsList(char *s,vector<D3DXMACRO> &macros)
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

bool DevEffect::GetPrecompiledBinary(
		vector<char> &data, const char *id, const char *path, const char *bpath,
		vector<D3DXMACRO> &macros, byte hash[16], vector<byte> &bin)
{
	ID3DXEffectCompiler *fxc = NULL;
	ID3DXBuffer *binbuff = NULL;
	ID3DXBuffer *errors = NULL;


	if(_load_scrambled(bpath,*(vector<byte>*)&bin) && bin.size()>16)
	{
		if(memcmp(&bin[0],hash,16)==0)
		{
			bin.erase(bin.begin(),bin.begin()+16);
			return true;
		}
	}

	for(int pass=0;pass<2;pass++)
	{
		HRESULT r;
		bool error;
		
		if( pass == 0 )
		{
			r = D3DXCreateEffectCompiler(&data[0],(DWORD)data.size(),&macros[0],NULL,
					compile_flags,&fxc,&errors);
			error = !fxc;
		}
		else
		{
			r = fxc->CompileEffect(compile_flags,&binbuff,&errors);
			error = !binbuff;
		}

		if(FAILED(r) || errors || error)
		{
			if(errors)
			{
				last_error = format("%s:%s: %s",path,id,(char*)errors->GetBufferPointer());
				if(!(flags&FXF_NO_ERROR_POPUPS))
					if(MessageBox(NULL,last_error.c_str(),format("%s:%s",path,id).c_str(),MB_OKCANCEL)==IDCANCEL)
						ExitProcess(0);
				errors->Release();
				if(fxc) fxc->Release();
				if(binbuff) binbuff->Release();
				return false;
			}
			else
			{
				last_error = format("%s:%s: Unknown error!",path,id);
				if(!(flags&FXF_NO_ERROR_POPUPS))
					if(MessageBox(NULL,last_error.c_str(),format("%s:%s",path,id).c_str(),MB_OKCANCEL)==IDCANCEL)
						ExitProcess(0);
				if(fxc) fxc->Release();
				if(binbuff) binbuff->Release();
				return false;
			}
		}
	}

	bin.clear();
	bin.insert(bin.end(),hash,hash+16);
	bin.insert(bin.end(),(byte*)binbuff->GetBufferPointer(),((byte*)binbuff->GetBufferPointer())+binbuff->GetBufferSize());

	fxc->Release();
	binbuff->Release();

	_save_scrambled(bpath,bin);

	bin.erase(bin.begin(),bin.begin()+16);

	return true;
}

bool DevEffect::CompileVersion(
			const char *id, const char *path, vector<byte> &bin,
			vector<D3DXMACRO> &macros, const char *skip_list )
{
	ID3DXEffect *fx = NULL;
	ID3DXBuffer *errors = NULL;

	HRESULT r = D3DXCreateEffectEx(Dev.GetDevice(),&bin[0],(DWORD)bin.size(),&macros[0],NULL,skip_list,
		compile_flags,pool,&fx,&errors);

	if(FAILED(r) || errors)
	{
		if(errors)
		{
			last_error = format("%s:%s: %s",path,id,(char*)errors->GetBufferPointer());
			if(!(flags&FXF_NO_ERROR_POPUPS))
				if(MessageBox(NULL,last_error.c_str(),format("%s:%s",path,id).c_str(),MB_OKCANCEL)==IDCANCEL)
					ExitProcess(0);
			errors->Release();
			return false;
		}
		else
		{
			last_error = format("%s:%s: Unknown error!",path,id);
			if(!(flags&FXF_NO_ERROR_POPUPS))
				if(MessageBox(NULL,last_error.c_str(),format("%s:%s",path,id).c_str(),MB_OKCANCEL)==IDCANCEL)
					ExitProcess(0);
			return false;
		}
	}

	version_index[id] = (int)fx_list.size();
	fx_list.push_back(fx);

	return true;
}
