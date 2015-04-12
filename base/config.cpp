
#include "base.h"


using namespace std;


namespace base
{



Config::Config(const char *_path,bool _autosave)
{
    autosave = _autosave;
    path = _path ? _path : "";
    if(_path)
        Load(_path);
}

Config::~Config()
{
    if(autosave)
        Save(path.c_str());
}

bool Config::Load(const char *p,bool set_as_autosave)
{
	if(set_as_autosave)
	{
	    autosave = true;
		path = p;
	}
	return tree.LoadTreeTxt(p,NFS);
}

bool Config::Save(const char *p)
{
    return tree.SaveTreeTxt(p);
}

TreeFileRef	Config::GetNode(const char *name,int id,bool write)
{
	TreeFileRef n = tree.GetRoot(write);
	string tmp, ptmp;

	if(id!=0)
	{
		ptmp = format("%s[%d]",name,id);
		name = ptmp.c_str();
	}

	while(n.IsValid() && *name)
	{
		const char *e = name;
		while(*e && *e!='/') e++;
		tmp.assign(name,e);

		n = n.SerChild(tmp.c_str());

		name = e;
		if(*name=='/')
			name++;
	}

	return n;
}

int Config::GetInt(const char *name,int def,int id)
{
	TreeFileRef n = GetNode(name,id,false);
	n.SerInt(NULL,def,def);
	return def;
}

float Config::GetFloat(const char *name,float def,int id)
{
	TreeFileRef n = GetNode(name,id,false);
	n.SerFloat(NULL,def,def);
	return def;
}

string Config::GetString(const char *name,const char *def,int id)
{
	TreeFileRef n = GetNode(name,id,false);
	string out;
	n.SerString(NULL,out,def);
	return out;
}

void Config::SetInt(const char *name,int v,int id)
{
	TreeFileRef n = GetNode(name,id,true);
	n.SerInt(NULL,v,v);
}

void Config::SetFloat(const char *name,float v,int id)
{
	TreeFileRef n = GetNode(name,id,true);
	n.SerFloat(NULL,v,v);
}

void Config::SetString(const char *name,const char *v,int id)
{
	TreeFileRef n = GetNode(name,id,true);
    string s = v;
	n.SerString(NULL,s,v);
}



}
