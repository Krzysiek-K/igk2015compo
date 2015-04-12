
#ifndef _BASE_CONFIG_H
#define _BASE_CONFIG_H


#include <map>



namespace base
{


class Config {
public:

	Config(const char *_path=NULL,bool _autosave=false);
	~Config();

    bool Load(const char *p,bool set_as_autosave=false);
    bool Save(const char *p);

	TreeFileRef	GetNode(const char *name,int id,bool write);
	int			GetInt(const char *name,int def,int id=0);
	float		GetFloat(const char *name,float def,int id=0);
	std::string	GetString(const char *name,const char *def,int id=0);

	TreeFileRef	GetNodeWrite(const char *name,int id=0);
	void SetInt(const char *name,int v,int id=0);
	void SetFloat(const char *name,float v,int id=0);
    void SetString(const char *name,const char *v,int id=0);


private:
	TreeFileBuilder		tree;
    std::string         path;
    bool                autosave;

};



}




#endif
