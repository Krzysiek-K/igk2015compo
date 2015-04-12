
#include "base.h"


using namespace std;



namespace base
{



CfgPreprocessor::CfgPreprocessor(const char *path)
{
	const char *p = path;
	while(*p) p++;
	while(p>path && *p!='/' && *p!='\\') p--;
	if(*p=='/' || *p=='\\') p++;
	base_dir.assign(path,p-path);

	files.clear();
	current = NULL;
	read_pos = NULL;
	read_size = 0;

	Include(p);
}

CfgPreprocessor::~CfgPreprocessor()
{
}

char CfgPreprocessor::GetChar()
{
	if(!current) return 0;

	char ch0 = GetChar(0);
	char ch1 = GetChar(1);

	int comment = 0;
	if(ch0=='/' && ch1=='/') comment = 1;
	if(ch0=='/' && ch1=='*') comment = 2;
	while(comment)
	{
		Advance();
		ch0 = GetChar(0);
		ch1 = GetChar(1);
		if(ch0==0) break;
		if(comment==1 && ch0=='\n') break;
		if(comment==2 && ch0=='*' && ch1=='/') break;
	}

	if(ch0=='\n' && ch1=='#')
	{
	}

	Advance();
	return ch0;
}

char CfgPreprocessor::GetChar(int offs)
{
	bool cnt = true;
	while(true)
	{
		if(offs<read_size) return read_pos[offs];
		if(!cnt) break;
		cnt = ReadFile();
	}
	return 0;
}

void CfgPreprocessor::Advance(int cnt)
{
	while(cnt>0)
	{
		int adv = cnt;
		if(adv>read_size) adv = read_size;
		read_size -= adv;
		cnt -= adv;
		while(adv-->0)
		{
			if(*read_pos=='\n') current->line++;
			read_pos++;
		}

		if(read_size<=0) ReadFile();
		if(read_size<=0) break;
	}
}

bool CfgPreprocessor::ReadFile()
{
	if(!current->fp)
		return false;

	if(read_pos !=NULL)
		if( read_pos - &current->buffer[0] >= 4096)
		{
			current->buffer.erase(0,current->bpos);
			read_pos = &current->buffer[0];
			read_size = current->buffer.size();
		}

	int bpos = 0;
	if(read_pos != NULL)
		bpos = read_pos - &current->buffer[0];

	int bsize = current->buffer.size();
	current->buffer.resize(bsize+1024);
	int len = fread(&current->buffer[bsize],1,1024,current->fp);
	current->buffer.resize(bsize+len);

	read_pos = &current->buffer[0] + bpos;

	if(len<1024)
	{
		fclose(current->fp);
		current->fp = NULL;
		return false;
	}

	return true;
}

void CfgPreprocessor::Include(const char *path)
{
	string fpath = base_dir + path;

	FilePos fp;
	fp.filename = path;
	fp.fp = fopen(path,"rt");
	fp.bpos = 0;
	fp.line = 1;

	if(!fp.fp)
		return;

	files.push_back(fp);
	current = &files[files.size()-1];
}





bool CfgParser::Load(const char *path,TreeFileRef &f,FileSystem &fs)
{
	filesystem = &fs;

	{
		const char *p = path;
		while(*p) p++;
		while(p>path && *p!='/' && *p!='\\') p--;
		if(*p=='/' || *p=='\\') p++;
		base_dir.assign(path,p-path);
	}

	vector<unsigned char> data;
	if(!filesystem->GetFileBytes(path,data))
		return false;

	const char *p = (char*)&data[0];
	ParseBlock(p,f);

	return true;
}

bool CfgParser::ReadName(const char *&p, string &name, int &index)
{
	SkipWhitesNL(p);

	const char *n_begin, *n_end;
	int _index = 0;

	n_begin = p;
	while(IsAlnum(*p) || *p == '_' || *p=='@' || *p=='$')
		p++;
	n_end = p;

	SkipWhites(p);
	if(*p=='[')
	{
		p++;
		SkipWhites(p);

        _index = ParseInt(p);
		SkipWhites(p);
		if(*p!=']') return false;
		p++;
		SkipWhites(p);
	}
	else if(*p=='*')
	{
		p++;
		_index = auto_index++;
		SkipWhites(p);
	}

	if(*p==':' || *p=='=')
		p++;
	else
	{
		SkipWhitesNL(p);
		if(*p!='{') return false;
	}

	name.assign(n_begin, n_end-n_begin);
	index = _index;

	return true;
}

bool CfgParser::ReadValue(const char *&p, string &value)
{
	SkipWhites(p);

	bool quoted = (*p=='"');
	if(quoted) p++;

	value.clear();
	while(1)
	{
		if(quoted)
		{
			if(*p=='\n') return false;
			if(*p=='"')
			{
				p++;
				break;
			}
		}
		else
			if(*p==' ' || *p=='\t' || *p=='\r' || *p=='\n' || *p==';')
				break;

		if(*p=='\r' || *p=='\n')
			return false;

		if(*p!='\\')
			value.push_back(*p++);
		else
		{
			p++;
			if(*p=='\r') p++; // CRLF version

			if(*p=='\n') { } // do nothing 
			else if(*p=='n') value.push_back('\n');
			else if(*p=='t') value.push_back('\t');
			else if(*p=='"') value.push_back('"');
			else return false;
		}
	}

	SkipWhites(p);
	if(*p!=';') return false;
	p++;

	return true;
}

void CfgParser::ParseBlock(const char *&p,TreeFileRef &tree)
{
	string name, value;
	int index;
	while(1)
	{
		SkipWhitesNL(p);
		if(*p=='\0' || *p=='}')
			break;

		if(!ReadName(p,name,index))
		{
			SkipError(p);
			continue;
		}

		if(*p=='{')
		{
			p++;
			ParseBlock(p,tree.SerChild(name.c_str(),index));
		}
		else
		{
			if(!ReadValue(p,value))
				SkipError(p);
			else
			{
				map<string,string>::iterator f = defines.find(value);
				if(f!=defines.end())
					value = f->second;

				if(name.c_str()[0]=='@')
					defines[name.c_str()+1] = value;
				else if(name.c_str()[0]=='$')
				{
					if(name == "$include")
					{
						CfgParser included;
						string path = base_dir + value;
						included.auto_index = auto_index;
						included.defines = defines;
						included.Load(path.c_str(),tree,*filesystem);
						defines = included.defines;
						auto_index = included.auto_index;
					}
				}
				else
				{
					tree.SerString(name.c_str(),value,"",index);
				}
			}
		}
	}

	if(*p=='}')
		p++;
}


}
