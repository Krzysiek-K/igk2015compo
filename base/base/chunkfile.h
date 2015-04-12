
#ifndef _BASE_CHUNKFILE_H
#define _BASE_CHUNKFILE_H



namespace base {


/*

File format:
-  4 - magic
-  4 - version
-  4 - dir data offset

Directory (anywhere in the file):
-  1 - type ( 0 - dir end, 1 - chunk )
-  n - name (null-terminated)
-  4 - offset in file
-  4 - size
-  1 - extension (0 - old format, 1 - new format)
-  4 - struct size (only if extension == 1)

*/


class ChunkFileBuilder {
public:
	ChunkFileBuilder(DWORD magic=13,DWORD version=13);
	~ChunkFileBuilder() { }

	bool Save(const char *path);
	void AddChunk(const char *name,const byte *data,int size,int struct_size=0);
	
	template<class T>	void add(const char *name,const T &data)				{ AddChunk(name,&data,sizeof(T),sizeof(T)); }
	template<class T>	void add(const char *name,const T *data,int count=1)	{ AddChunk(name,data,count*sizeof(T),sizeof(T)); }
	
	template<class T>	void add(const char *name,const std::vector<T> &data)
	{ if(data.size()>0) AddChunk(name,(const byte*)&data[0],data.size()*sizeof(T),sizeof(T)); }

private:
	std::vector<byte>	file_data;
	std::vector<byte>	dir_data;
};


bool ChunkFileVerify(DWORD magic,DWORD version,const byte *data,int size);
const byte *ChunkFileGetChunk(const char *name,int *out_size,const byte *data,int *out_struct_size=0);



class ChunkFile {
public:
	ChunkFile(const char *path,DWORD magic=13,DWORD version=13)
	{
		if(!NFS.GetFileBytes(path,file) || file.size()<=0) { file.clear(); return; }
		if(!ChunkFileVerify(magic,version,&file[0],file.size())) { file.clear(); return; }
	}
	~ChunkFile() { }
	
	template<class T>
	void read(const char *name,T &data)
	{
		int size=0, ssize=0;
		const byte *src = file.size()>0 ? ChunkFileGetChunk(name,&size,&file[0],&ssize) : 0;
		memset(&data,0,sizeof(data));
		if(src)
		{
			if(sizeof(data)<size) size=sizeof(data);
			if(ssize && ssize<size) size=ssize;
			memcpy(&data,src,size);
		}
	}

	template<class T>
	void read(const char *name,std::vector<T> &data)
	{
		int size=0, ssize=0;
		const byte *src = file.size()>0 ? ChunkFileGetChunk(name,&size,&file[0],&ssize) : 0;
		data.clear();
		if(src && size>0)
		{
			if(!ssize) ssize=sizeof(T);
			data.resize(size/ssize);

			int csize = ssize;
			if(csize>sizeof(T)) csize = sizeof(T);

			for(int i=0;i<(int)data.size();i++)
			{
				T &t = data[i];
				memset(&t,0,sizeof(T));
				memcpy(&t,src+i*ssize,csize);
			}
		}
	}

private:
	std::vector<byte>	file;
};



}

#endif
