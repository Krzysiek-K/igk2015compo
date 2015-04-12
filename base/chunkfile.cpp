
#include "base.h"


namespace base {


ChunkFileBuilder::ChunkFileBuilder(DWORD magic,DWORD version)
{
	file_data.insert(file_data.end(),(byte*)&magic,((byte*)&magic)+4);
	file_data.insert(file_data.end(),(byte*)&version,((byte*)&version)+4);

	version = 0xFFFFFFFF;
	file_data.insert(file_data.end(),(byte*)&version,((byte*)&version)+4);	// dir offset
}

bool ChunkFileBuilder::Save(const char *path)
{
	FILE *fp = fopen(path,"wb");
	if(!fp) return false;

	((DWORD*)&file_data[0])[2] = file_data.size();

	fwrite(&file_data[0],1,file_data.size(),fp);
	if(dir_data.size()>0)
		fwrite(&dir_data[0],1,dir_data.size(),fp);
	int tmp = 0;
	fwrite(&tmp,1,1,fp);

	fclose(fp);
	return true;
}

void ChunkFileBuilder::AddChunk(const char *name,const byte *data,int size,int struct_size)
{
	dir_data.push_back(1);
	dir_data.insert(dir_data.end(),name,name+(strlen(name)+1));

	int tmp = file_data.size();
	dir_data.insert(dir_data.end(),(byte*)&tmp,((byte*)&tmp)+4);
	dir_data.insert(dir_data.end(),(byte*)&size,((byte*)&size)+4);
	if(struct_size>0)
	{
		dir_data.push_back(1);
		dir_data.insert(dir_data.end(),(byte*)&struct_size,((byte*)&struct_size)+4);
	}
	dir_data.push_back(0);

	file_data.insert(file_data.end(),data,data+size);
}


bool ChunkFileVerify(DWORD magic,DWORD version,const byte *data,int size)
{
	if(size<12) return false;
	if(((DWORD*)data)[0]!=magic) return false;
	if(((DWORD*)data)[1]!=version) return false;

	int dir_base = ((DWORD*)data)[2];
	if(dir_base<0 || dir_base>=size-1) return false;

	const byte *data_end = data + size;
	const byte *dir = data + dir_base;
	while(true)
	{
		if(dir>=data_end) return false;
		if(*dir==0) break;
		if(*dir!=1) return false;

		if(++dir>=data_end) return false;

		while(*dir)
			if(++dir>=data_end) return false;
		if(++dir>=data_end) return false;
		if(dir+8>=data_end) return false;
		int c_base = *(DWORD*)dir;
		dir += 4;
		int c_size = *(DWORD*)dir;
		dir += 4;

		if(c_base<0 || c_base+c_size>size || size<0)
			return false;

		if(*dir==1) dir += 5;
		if(*dir!=0) return false;
		if(++dir>=data_end) return false;
	}

	return true;
}

const byte *ChunkFileGetChunk(const char *name,int *out_size,const byte *data,int *out_struct_size)
{
	const byte *dir = data + ((DWORD*)data)[2];

	if(out_struct_size) *out_struct_size = 0;
	
	while(true)
	{
		if(*dir!=1) break;
		dir++;

		const byte *n = (const byte *)name;
		while(*dir && *dir==*n)
			dir++, n++;

		if(*dir==0 && *n==0)
		{
			// found
			dir++;
			const byte *ptr = data + *(DWORD*)dir;
			if(out_size) *out_size = ((DWORD*)dir)[1];
			dir += 8;
			if(out_struct_size && *dir==1)
			{
				dir++;
				*out_struct_size = *(DWORD*)dir;
			}
			return ptr;
		}

		while(*dir)
			dir++;
		dir++;			// terminating zero
		dir += 8;		// offs/size
		if(*dir==1)		// optional struct size
		{
			dir++;
			dir+=4;
		}
		dir++;			// reserved zero
	}

	if(out_size)
		*out_size = 0;

	return NULL;
}



}
