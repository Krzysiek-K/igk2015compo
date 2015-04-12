
#ifndef _B_TREEFILE_H
#define _B_TREEFILE_H


#include <string.h>

#include <vector>
#include <string>
#include <map>

#include "types.h"
#include "streams.h"



namespace base {


class TreeFileNode {
private:
	friend class TreeFileRef;
	friend class TreeFileBuilder;

	enum {	T_VOID = 0,
			T_INT = 1,
			T_FLOAT = 2,
			T_STRING = 3,
			T_RAW = 4,
	};
	enum { SHORT_DATA_MAX = 4 };


	struct NameRef {
		const char	*name;
		int			clone_id;

		bool operator <(const NameRef &r) const
		{
			int cmp = _stricmp(name,r.name);
			if(cmp!=0) return cmp<0;
			return clone_id < r.clone_id;
		}

		bool operator ==(const NameRef &r) const
		{
			int cmp = _stricmp(name,r.name);
			if(cmp!=0) return false;
			return clone_id == r.clone_id;
		}
	};

public:
	typedef std::map<NameRef,TreeFileNode *>::iterator	children_iterator;

private:
	const char		*name;
	int				clone_id;
	int				type;
	unsigned char	sdata[SHORT_DATA_MAX];
	unsigned char	*ldata;
	int				size;
	bool			owns_name;
	bool			owns_data;
	bool			invalid;

	std::map<NameRef,TreeFileNode *>	children;


//public:	// class to be used only internally

	TreeFileNode(const char *_name,int _id,bool copy_name=true);
	~TreeFileNode();

	children_iterator begin() { return children.begin(); }
	children_iterator end() { return children.end(); }

	TreeFileNode *GetChild(const char *cname,int cid,bool create=false,bool copy_name=true);

	bool Validate();

	void			SetData(const unsigned char *new_data,int new_size,int new_type,bool copy=true);
	unsigned char	*GetData() { return (size<=SHORT_DATA_MAX && owns_data) ? sdata : ldata; }
	int				GetSize() { return size; }

	bool GetInt(int &out);
	bool GetFloat(float &out);
	bool GetString(std::string &out);

	void SetInt(int value)					{ SetData((unsigned char*)&value,sizeof(int),T_INT); }
	void SetFloat(float value)				{ SetData((unsigned char*)&value,sizeof(float),T_FLOAT); }
	void SetString(const char *str)			{ SetData((unsigned char*)str,(int)strlen(str),T_STRING); }

};



class TreeFileRef {
public:
	class iterator {
	public:

		iterator() : empty(true) {}

		TreeFileRef operator *()		{	return empty ? TreeFileRef() : TreeFileRef(iter->second,false);	}
		void operator ++()				{	if(!empty) ++iter;	}

		bool operator ==(const iterator &it) { return (empty || it.empty) ? (empty==it.empty) : (iter==it.iter); }
		bool operator !=(const iterator &it) { return !operator ==(it); }

	private:
		friend class TreeFileRef;

		TreeFileNode::children_iterator iter;
		bool empty;

		iterator(const TreeFileNode::children_iterator &it) : empty(false), iter(it) {}
	};


	TreeFileRef();
	TreeFileRef(const TreeFileRef &f);
	~TreeFileRef();

	bool IsValid()			{ return (node!=NULL); }
	const char *GetName()	{ return node ? node->name : ""; }
	int GetId()				{ return node ? node->clone_id : 0; }

	int GetCloneArraySize(const char *name);

	iterator begin()		{ return (!node || is_writing) ? iterator() : iterator(node->begin()); }
	iterator end()			{ return (!node || is_writing) ? iterator() : iterator(node->end()); }



	void SerBool(const char *name,bool &v,bool def,int id=0)	{ SerializeBasic(name,&v,sizeof(v),false,def,id); }
	void SerByte(const char *name,byte &v,byte def,int id=0)	{ SerializeBasic(name,&v,sizeof(v),false,def,id); }
	void SerWord(const char *name,word &v,word def,int id=0)	{ SerializeBasic(name,&v,sizeof(v),false,def,id); }
	void SerDword(const char *name,dword &v,dword def,int id=0)	{ SerializeBasic(name,&v,sizeof(v),false,def,id); }
	void SerChar(const char *name,char &v,char def,int id=0)	{ SerializeBasic(name,&v,sizeof(v),true,def,id); }
	void SerShort(const char *name,short &v,short def,int id=0)	{ SerializeBasic(name,&v,sizeof(v),true,def,id); }
	void SerInt(const char *name,int &v,int def,int id=0)		{ SerializeBasic(name,&v,sizeof(v),true,def,id); }

	void SerializeBasic(const char *name,void *v,int size,bool sign,int def,int id=0);
	void SerFloat(const char *name,float &v,float def,int id=0);
	void SerPChar(const char *name,char *str,int len,const char *def,int id=0);
	void SerString(const char *name,std::string &out,const char *def,int id=0);

	void		Write_SetRaw(const char *name,const void *data,int size,int id=0);
	int			Read_GetRawSize(const char *name,int id=0);
	const void *Read_GetRawData(const char *name,int id=0);

	TreeFileRef SerChild(const char *name,int id=0);

	template<class _T>
	void SerVector(const char *name,std::vector<_T> &v)
	{
		if(IsReading())
		{
			v.clear();
			v.resize(GetCloneArraySize(name));
		}
		for(int i=0;i<v.size();i++)
			v[i].Serialize(SerChild(name,i));
	}

	template<class _T>
	void SerVectorPtr(const char *name,std::vector<_T*> &v)
	{
		if(IsReading())
		{
			for(int i=0;i<v.size();i++)
				if(v[i])
					delete v[i];
			v.clear();
			v.resize(GetCloneArraySize(name));
		}
		for(int i=0;i<v.size();i++)
		{
			if(IsReading())
				v[i] = new _T();
			v[i]->Serialize(SerChild(name,i));
		}
	}

	bool IsWriting() { return is_writing; }
	bool IsReading() { return !is_writing; }


private:
	friend class TreeFileBuilder;

	TreeFileNode	*node;
	bool			is_writing;


	TreeFileRef(TreeFileNode *n,bool is_wr);

	TreeFileNode *GetChild(const char *name,int id);

};


class TreeFileBuilder {

public:

	TreeFileBuilder();
	~TreeFileBuilder();

	TreeFileRef GetRoot(bool write);
	void Clear();

	bool LoadTreeTxt(const char *path,FileSystem &fs);
	bool SaveTreeTxt(const char *path);
	bool LoadTreeBin(InStream *in);
	bool SaveTreeBin(OutStream *out);

private:
    struct TextFileContext {
        int                                 auto_index;
        std::map<std::string,std::string>   defines;
        std::string                         base_path;
        std::map<std::string,int>           *last_index;
        FileSystem                          *fs;
    };

	TreeFileNode				*root;
	std::map<std::string,int>	dictionary_hits;
	std::vector<std::string>	dictionary;

	bool ReadNode_Binary(TreeFileNode *parent, InStream &in);
	bool WriteNode_Binary(TreeFileRef tf, OutStream &out);

    bool ReadNode_Text(TreeFileNode *parent,const char *&s,TextFileContext &ctx);
    bool WriteNode_Text(TreeFileNode *n,FILE *file,int indent,std::map<std::string,int> *last_index);

	void BuildDictionary(TreeFileRef tf,bool root_level = true);

	bool ReadDictionary_Binary(InStream &in);
	bool WriteDictionary_Binary(OutStream &out);
};





}




#endif
