
#ifndef _CFG_PARSER_H
#define _CFG_PARSER_H


#include <vector>
#include <string>
#include <map>



namespace base {



class CfgPreprocessor {
public:
	CfgPreprocessor(const char *path);
	~CfgPreprocessor();

	char GetChar();

private:
	struct FilePos {
		FILE		*fp;
		std::string	buffer;
		int			bpos;
		int			line;
		std::string	filename;
	};

	std::vector<FilePos>	files;
	FilePos					*current;
	std::string				base_dir;
	const char				*read_pos;
	int						read_size;


	char GetChar(int offs=0);
	void Advance(int cnt=1);
	bool ReadFile();
	void Include(const char *path);
};



class CfgParser {

public:
	CfgParser() { auto_index = 0x40000000; };

	bool Load(const char *path,base::TreeFileRef &f,FileSystem &fs);

private:
	int										auto_index;
	std::map<std::string,std::string>		defines;
	std::string								base_dir;
	FileSystem								*filesystem;


	bool ReadName(const char *&p, std::string &name, int &index);
	bool ReadValue(const char *&p, std::string &value);
	void ParseBlock(const char *&p, TreeFileRef &tree);


	static bool IsAlnum		(char ch);
	static void SkipWhites	(const char *& p);
	static void SkipWhitesNL(const char *& p);
	static void SkipError	(const char *& p);
};




inline bool CfgParser::IsAlnum(char ch) {
	if((ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || (ch>='0' && ch <='9') 
		|| ch=='_'
		|| ch=='"'
		|| ch=='-'
		) return true;
	else  return false;
		
}

inline void CfgParser::SkipWhitesNL(const char *& p) {
	while(*p==' ' || *p=='\t' || *p=='\n' || *p=='\r')
		p++;
}

inline void CfgParser::SkipWhites(const char *& p) {
	while(*p==' ' || *p=='\t' || *p=='\r')
		p++;
}

inline void CfgParser::SkipError(const char *& p) {
	while(*p && *p!='\n' && *p!=';')
		p++;
	if(*p==';') p++;
}


}



#endif
