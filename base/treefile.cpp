
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include "base.h"


using namespace std;



namespace base {


// ---------------- TreeFileNode ----------------


TreeFileNode::TreeFileNode(const char *_name,int _id,bool copy_name)
{
	owns_name = copy_name;
	if(copy_name)
	{
		int len = strlen(_name);
		name = new char[len+1];
		memcpy((char*)name,_name,len+1);
	}
	else
		name = _name;

	clone_id = _id;
	invalid = false;

	type = T_VOID;
	ldata = NULL;
	size = 0;
	owns_data = false;
}

TreeFileNode::~TreeFileNode()
{
	map<NameRef,TreeFileNode *>::iterator p,prev;

	p = children.begin();
	while(p!=children.end())
	{
		TreeFileNode *n = p->second;
		prev = p;
		++p;

		children.erase(prev);
		delete n;
	}

	if(name && owns_name) delete name;
	if(ldata && owns_data) delete ldata;
}

TreeFileNode *TreeFileNode::GetChild(const char *cname,int cid,bool create,bool copy_name)
{
	NameRef key;
	key.name = cname;
	key.clone_id = cid;

	map<NameRef,TreeFileNode *>::iterator child = children.find(key);

	if(child != children.end())
		return child->second;

	if(!create)
		return NULL;

	TreeFileNode *n = new TreeFileNode(cname,cid,copy_name);
	key.name = n->name;

	children[key] = n;

	return n;
}

bool TreeFileNode::Validate()
{
	if(invalid) return false;

	// call recursively
	map<NameRef,TreeFileNode *>::iterator p;

	for(p = children.begin();p!=children.end();++p)
		if(!p->second->Validate())
			return false;

	return true;
}

void TreeFileNode::SetData(const byte *new_data,int new_size,int new_type,bool copy)
{
	if(ldata && owns_data) delete ldata;
	ldata = NULL;
	owns_data = false;
    size = new_size;
    type = new_type;

	if(!copy)
	{
		ldata = (byte*)new_data;
		return;
	}

    byte *ptr = NULL;

    if(new_size<=SHORT_DATA_MAX)
        ptr = sdata;
    else
        ptr = ldata = (new_size>0) ? new byte[new_size] : NULL;

    owns_data = true;
    if(new_data)    memcpy(ptr,new_data,new_size);
    else            memset(ptr,0,new_size);
}

bool TreeFileNode::GetInt(int &out)
{
	if(type==T_INT)
	{
		out = *(int*)GetData();
		return true;
	}

	if(type==T_STRING)
	{
        string tmp((char*)GetData(),GetSize());
        const char *s = tmp.c_str();
        out = ParseInt(s);
        ParseWhitespace(s);
        return (*s==0);
	}

	return false;
}

bool TreeFileNode::GetFloat(float &out)
{
	if(type==T_FLOAT)
	{
		out = *(float*)GetData();
		return true;
	}

	if(type==T_STRING)
	{
		string tmp((char*)GetData(),GetSize());
        const char *s = tmp.c_str();
        out = ParseFloat(s);
        ParseWhitespace(s);
		return (*s==0);
	}

	return false;
}

bool TreeFileNode::GetString(std::string &out)
{
	if(type==T_STRING)
	{
		out.assign((char*)GetData(),GetSize());
		return true;
	}

	if(type==T_INT)
	{
        out.clear();
        AppendInt(out,*(int*)GetData());
		return true;
	}

	if(type==T_FLOAT)
	{
        out.clear();
        AppendFloat(out,*(float*)GetData());
		return true;
	}

    return false;
}





// ---------------- TreeFileRef ----------------


TreeFileRef::TreeFileRef()
{
	node = NULL;
	is_writing = false;
}

TreeFileRef::TreeFileRef(TreeFileNode *n,bool is_wr)
{
	node = n;
	is_writing = is_wr;
}

TreeFileRef::TreeFileRef(const TreeFileRef &f)
{
	node = f.node;
	is_writing = f.is_writing;
}

TreeFileRef::~TreeFileRef()
{
}

int TreeFileRef::GetCloneArraySize(const char *name)
{
	if(!node) return 0;

	int id = 0;
	while(1)
	{
		if(!node->GetChild(name,id,false))
			break;
		id++;
	}
	return id;
}

void TreeFileRef::SerializeBasic(const char *name,void *v,int size,bool sign,int def,int id)
{
	if(!is_writing)
		memcpy(v,&def,size);

	TreeFileNode *n = GetChild(name,id);
	if(!n) return;

	int i,m;

	if(is_writing)
	{
		i=0;
		memcpy(&i,v,size);

		if(sign)
		{
			m=0xFFFFFFFF<<(size*8-1);
			if(i&m)
				i|=m;
		}
		n->SetInt(i);
	}
	else
	{
		if(n->GetInt(i))
			memcpy(v,&i,size);
	}
}

void TreeFileRef::SerFloat(const char *name,float &v,float def,int id)
{
	if(!is_writing)
		v = def;

	TreeFileNode *n = GetChild(name,id);
	if(!n) return;

	if(is_writing)
		n->SetFloat(v);
	else
		n->GetFloat(v);
}

void TreeFileRef::SerPChar(const char *name,char *str,int len,const char *def,int id)
{
	TreeFileNode *n = GetChild(name,id);
	if(!n)
	{
		if(!is_writing)
		{
			strncpy(str,def,len-1);
			str[len-1]=0;
		}
		return;
	}

	if(is_writing)
		n->SetString(str);
	else
	{
		string s;
		const char *src = def;
		if(n->GetString(s))
			src = s.c_str();
		strncpy(str,src,len-1);
		str[len-1]=0;
	}
}

void TreeFileRef::SerString(const char *name,std::string &out,const char *def,int id)
{
	if(is_writing)
	{
		SerPChar(name,(char*)out.c_str(),out.length()+1,def,id);
		return;
	}

	TreeFileNode *n = GetChild(name,id);
	if(!n)
	{
		out = def;
		return;
	}

	if(!n->GetString(out))
		out = def;
}

void TreeFileRef::Write_SetRaw(const char *name,const void *data,int size,int id)
{
	if(!is_writing) return;
	if(size<0) size = 0;

	TreeFileNode *n = GetChild(name,id);
	if(!n) return;

	n->SetData((const unsigned char*)data,size,TreeFileNode::T_RAW);
}

int TreeFileRef::Read_GetRawSize(const char *name,int id)
{
	if(is_writing) return 0;

	TreeFileNode *n = GetChild(name,id);
	if(!n || n->type!=TreeFileNode::T_RAW) return 0;

	return n->GetSize();
}

const void *TreeFileRef::Read_GetRawData(const char *name,int id)
{
	if(is_writing) return 0;

	TreeFileNode *n = GetChild(name,id);
	if(!n || n->type!=TreeFileNode::T_RAW) return NULL;

	return n->GetData();
}



TreeFileRef TreeFileRef::SerChild(const char *name,int id)
{
	return TreeFileRef(GetChild(name,id),is_writing);
}

TreeFileNode *TreeFileRef::GetChild(const char *name,int id)
{
	if(!node)	return NULL;
	if(!name)	return node;

	const char *s = name;
	while(*s && *s!='[') s++;
	if(*s=='[')
	{
		s++;
		id += ParseInt(s);
	}

	return node->GetChild(name,id,is_writing);
}



// ---------------- TreeFileBuilder ----------------

TreeFileBuilder::TreeFileBuilder()
{
	root = NULL;
}

TreeFileBuilder::~TreeFileBuilder()
{
	Clear();
}


TreeFileRef TreeFileBuilder::GetRoot(bool write)
{
	if(write && root==NULL)
	{
		Clear();
		root = new TreeFileNode("",0);
	}

	return TreeFileRef(root,write);
}

void TreeFileBuilder::Clear()
{
	if(root)
		delete root;
	root = NULL;
}

bool TreeFileBuilder::LoadTreeTxt(const char *path,FileSystem &fs)
{
    Clear();
    TreeFileRef tf = GetRoot(true);

    if(!tf.IsValid()) return false;

    vector<byte> data;
    if(!fs.GetFileBytes(path,data))
        return false;

    data.push_back(0);

    const char *s = (const char*)&data[0];
    map<string,int> last_index;
    TextFileContext ctx;
    ctx.auto_index = 0x40000000;
    ctx.base_path = FilePathGetPart(path,true,false,false);
    ctx.fs = &fs;
    ctx.last_index = &last_index;

    while( *s )
    {
        const char *p = s;
        ReadNode_Text(tf.node,s,ctx);
        if(s<=p) break;
    }

    if(!root->Validate())
        return false;

    return true;
}

bool TreeFileBuilder::SaveTreeTxt(const char *path)
{
	FILE *file = fopen(path, "wt");

	if(!file) return false;
	
	TreeFileNode *n = root;
	map<TreeFileNode::NameRef,TreeFileNode *>::iterator p;
    map<string,int> li;
	for(p = n->children.begin();p!=n->children.end();++p)
		WriteNode_Text(p->second,file,0,&li);
	fclose(file);

	return true;
}

bool TreeFileBuilder::LoadTreeBin(InStream *in)
{
    Clear();
	TreeFileRef tf = GetRoot(true);

	dictionary_hits.clear();
	dictionary.clear();

	bool ok = true;
	if(ok) ok = tf.IsValid();
	if(ok) ok = ReadDictionary_Binary(*in);
	if(ok)
	{
		int cnt = 0;
		in->Read(&cnt,4);
		for(int i=0;ok && i<cnt;i++)
			ok = ReadNode_Binary(tf.node,*in);
	}
	if(ok) ok = root->Validate();

	dictionary_hits.clear();
	dictionary.clear();

	return ok;
}

bool TreeFileBuilder::SaveTreeBin(OutStream *out)
{
	// start
	TreeFileRef tf = GetRoot(false);
	if(!tf.IsValid()) return false;

	// build dictionary
	BuildDictionary(tf);

	// write data
	bool ok = true;
	if(ok) ok = WriteDictionary_Binary(*out);
	if(ok)
	{
		int cnt = tf.node->children.size();
		out->Write(&cnt,4);
		for(TreeFileRef::iterator p = tf.begin();ok && p!=tf.end();++p)
			ok = WriteNode_Binary(*p,*out);
	}

	// cleanup
	dictionary_hits.clear();
	dictionary.clear();

	return ok;
}

bool TreeFileBuilder::ReadNode_Binary(TreeFileNode *parent, InStream &in)
{
	int flags = 0;
	in.Read(&flags,2);

	if((flags&0xF000)!=0xB000)
		return false;

	// flags:	1011ccssiinntttt
	//	c - children info	(00: no children,	01: unsigned char, 10: unsigned short, 11: int)
	//	s - size info		(00: size = 0 or 4,	01: unsigned char, 10: unsigned short, 11: int)
	//	i - clone id info	(00: clone_id = 0,	01: unsigned char, 10: unsigned short, 11: int)
	//	n - name id info	(00: reserved,		01: unsigned char, 10: unsigned short, 11: int)
	//	t - type id

	static const int WIDTH[4] = { 0, 1, 2, 4 };
	int type = flags&0x000F;
	int n_children = 0;
	int data_size = (type==TreeFileNode::T_VOID) ? 0 : 4;
	int clone_id = 0;
	int name_id = 0;

	if(type>4) return false;
	if((flags&0x0030)==0) return false;

	in.Read(&name_id	,WIDTH[(flags>> 4)&3]);
	in.Read(&clone_id	,WIDTH[(flags>> 6)&3]);
	in.Read(&data_size	,WIDTH[(flags>> 8)&3]);
	in.Read(&n_children	,WIDTH[(flags>>10)&3]);

	if(name_id<0 || name_id>=(int)dictionary.size())
		return false;

	TreeFileNode *node = parent->GetChild(dictionary[name_id].c_str(),clone_id,true);
	node->SetData(NULL,data_size,type);
	if(node->GetSize()!=data_size)
		return false;

	in.Read(node->GetData(),node->GetSize());

	for(int i=0;i<n_children;i++)
		if(!ReadNode_Binary(node,in))
			return false;
	
	return !in.WasError();
}

bool TreeFileBuilder::WriteNode_Binary(TreeFileRef tf, OutStream &out)
{
	// flags:	1011ccssiinntttt
	//	c - children info	(00: no children,	01: unsigned char, 10: unsigned short, 11: int)
	//	s - size info		(00: size = 0 or 4,	01: unsigned char, 10: unsigned short, 11: int)
	//	i - clone id info	(00: clone_id = 0,	01: unsigned char, 10: unsigned short, 11: int)
	//	n - name id info	(00: reserved,		01: unsigned char, 10: unsigned short, 11: int)
	//	t - type id

	int flags = 0xB000;
	int type = tf.node->type;
	int n_children = tf.node->children.size();
	int data_size = tf.node->GetSize();
	int clone_id = tf.GetId();
	int name_id = dictionary_hits[tf.GetName()];
	int wc,ws,wi,wn;

	flags |= type;

	if(name_id<0x100)			flags |= 0x0010, wn = 1;
	else if(name_id<0x10000)	flags |= 0x0020, wn = 2;
	else						flags |= 0x0030, wn = 4;

	if(clone_id==0)				flags |= 0x0000, wi = 0;
	else if(clone_id<0x100)		flags |= 0x0040, wi = 1;
	else if(clone_id<0x10000)	flags |= 0x0080, wi = 2;
	else						flags |= 0x00C0, wi = 4;

	int ds = (type==TreeFileNode::T_VOID) ? 0 : 4;
	if(data_size==ds)			flags |= 0x0000, ws = 0;
	else if(data_size<0x100)	flags |= 0x0100, ws = 1;
	else if(data_size<0x10000)	flags |= 0x0200, ws = 2;
	else						flags |= 0x0300, ws = 4;

	if(n_children==0)			flags |= 0x0000, wc = 0;
	else if(n_children<0x100)	flags |= 0x0400, wc = 1;
	else if(n_children<0x10000)	flags |= 0x0800, wc = 2;
	else						flags |= 0x0C00, wc = 4;

	out.Write(&flags,2);
	out.Write(&name_id		,wn);
	out.Write(&clone_id		,wi);
	out.Write(&data_size	,ws);
	out.Write(&n_children	,wc);

	out.Write(tf.node->GetData(),data_size);

	for(TreeFileRef::iterator p = tf.begin();p!=tf.end();++p)
		if(!WriteNode_Binary(*p,out))
			return false;

	return !out.WasError();
}


#define TEXT_SPECIALS       ":;*@$[]{}"


bool TreeFileBuilder::ReadNode_Text(TreeFileNode *parent,const char *&s,TextFileContext &ctx)
{
    string name, value;
    int index = 0;

    ParseWhitespace(s);
    if(!*s || *s=='}')
        return false;

    if(*s=='@')
    {
        s++;
        ParseStringT(s,name,TEXT_SPECIALS);
        ParseWhitespace(s);
        if(*s==':') s++;
        ParseStringT(s,value,TEXT_SPECIALS);
        ParseWhitespace(s);
        if(*s==';') s++;
        ctx.defines[name] = value;
        return true;
    }

    if(*s=='$')
    {
        s++;
        ParseStringT(s,name,TEXT_SPECIALS);
        ParseWhitespace(s);
        if(*s==':') s++;
        ParseStringT(s,value,";");
        ParseWhitespace(s);
        if(*s==';') s++;
        if(name=="include")
        {
            string path = ctx.base_path + value;

            vector<byte> data;
            if(ctx.fs->GetFileBytes(path.c_str(),data))
            {
                data.push_back(0);

                const char *q = (const char*)&data[0];
                while( *q )
                {
                    const char *p = q;
                    ReadNode_Text(parent,q,ctx);
                    if(q<=p) break;
                }
            }
        }
        return true;
    }
    
    ParseStringT(s,name,TEXT_SPECIALS);

    ParseWhitespace(s);
    if(*s=='[')
    {
        s++;
        ParseWhitespace(s);
        if(*s==']')
        {
            index = ++(*ctx.last_index)[name];
            s++;
        }
        else
        {
            (*ctx.last_index)[name] = index = ParseInt(s);
            ParseWhitespace(s);
            if(*s==']') s++;
        }
    }
    else if(*s=='*')
    {
        s++;
        index = ctx.auto_index++;
    }

    TreeFileNode *node = parent ? parent->GetChild(name.c_str(),index,true) : NULL;

    if(*s==':')
    {
        s++;
        ParseWhitespace(s);
        if(*s=='$')
        {
            s++;

            vector<byte> buff;
            ParseHexBuffer(s,buff);
            if(buff.size()>0)   node->SetData(&buff[0],buff.size(),TreeFileNode::T_RAW);
            else                node->SetData(NULL,0,TreeFileNode::T_RAW);
        }
        else
        {
            ParseStringT(s,value,";{}");
            if(ctx.defines.find(value)!=ctx.defines.end())
                value = ctx.defines[value];
            if(node) node->SetString(value.c_str());
        }
    }
    ParseWhitespace(s);
    if(*s=='{')
    {
        s++;

        map<string,int> last_index, *_li;
        _li = ctx.last_index;
        ctx.last_index = &last_index;

        while(ReadNode_Text(node,s,ctx)) {}
        ParseWhitespace(s);
        if(*s=='}') s++;

        ctx.last_index = _li;
    }

    ParseWhitespace(s);
    if(*s==';') s++;

    return true;
}


#define WRITE_STR(str) fwrite(str.c_str(), 1, str.length(), file)

bool TreeFileBuilder::WriteNode_Text(TreeFileNode *n,FILE *file,int indent,std::map<std::string,int> *last_index)
{
	string out;
	
    for(int i=0; i<indent; i++)
        out.push_back('\t');

    AppendString(out,n->name,TEXT_SPECIALS);

    if(n->clone_id != 0) 
    {
        out.push_back('[');
        if(n->clone_id!=(*last_index)[n->name]+1)
            AppendInt(out,n->clone_id);
        out.push_back(']');
    }
    (*last_index)[n->name] = n->clone_id;

    string value;
    if(n->type==TreeFileNode::T_RAW)
    {
        out += ": $";
        AppendHexBuffer(out,n->GetData(),n->GetSize());
    }
    else if(n->GetString(value))
    {
        out += ": ";
        AppendString(out,value.c_str(),TEXT_SPECIALS);
    }

	if(n->children.size()>0)
	{
        out += " {\n";
		WRITE_STR(out);

		// call recursively
        map<string,int> li;

		map<TreeFileNode::NameRef,TreeFileNode *>::iterator p;
		for(p = n->children.begin();p!=n->children.end();++p)
			WriteNode_Text(p->second,file,indent+1,&li);

        out.clear();
        for(int i=0; i<indent; i++)
            out.push_back('\t');
		out += "}\n";
	}
    else
        out += ";\n";

    WRITE_STR(out);
	
	return true;
}
#undef WRITE_STR




void TreeFileBuilder::BuildDictionary(TreeFileRef tf,bool root_level)
{
	if(root_level)
	{
		dictionary_hits.clear();
		dictionary.clear();
	}

	for(TreeFileRef::iterator p = tf.begin();p!=tf.end();++p)
	{
		dictionary_hits[(*p).GetName()]++;
		BuildDictionary( TreeFileRef( (*p).node, false ), false );
	}

	if(root_level)
	{
		for(map<string,int>::iterator p=dictionary_hits.begin();p!=dictionary_hits.end();++p)
			dictionary.push_back(p->first);

		class FreqSorter {
		public:
			map<string,int>	*freq;
			bool operator ()(const string &a,const string &b) const
			{ return (*freq)[a] > (*freq)[b]; }
		} fs;
		fs.freq = &dictionary_hits;
		sort(dictionary.begin(),dictionary.end(),fs);

		for(int i=0;i<(int)dictionary.size();i++)
			dictionary_hits[dictionary[i]] = i;
	}
}

bool TreeFileBuilder::ReadDictionary_Binary(InStream &in)
{
	dictionary.clear();

	int magic = 0;
	in.Read(&magic,4);
	if(magic!=0xA0A2A1A3) return false;

	string buffer;
	int cnt = 0;
	in.Read(&cnt,4);
	for(int i=0;i<cnt;i++)
	{
		int len = 0;
		in.Read(&len,2);
		if((int)buffer.size()<len)
			buffer.resize(len);
		in.Read(&buffer[0],len);
		dictionary.push_back(string(buffer.data(),len));
	}

	return !in.WasError();
}

bool TreeFileBuilder::WriteDictionary_Binary(OutStream &out)
{
	int magic = 0xA0A2A1A3;
	out.Write(&magic,4);

	int cnt = dictionary.size();
	out.Write(&cnt,4);
	for(int i=0;i<cnt;i++)
	{
		int len = dictionary[i].size();
		if(len>0xFFFF) return false;
		out.Write(&len,2);
		out.Write(dictionary[i].c_str(),len);
	}

	return !out.WasError();
}



}
