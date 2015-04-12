
#include "dxfw.h"

using namespace std;
using namespace base;




// **************** DevTexture ****************


DevTexture::DevTexture()
{
	tex = NULL;
}

DevTexture::DevTexture(const char *_path)
{
	tex = NULL;
	preload_path = _path;
}

DevTexture::~DevTexture()
{
	if(tex)
		tex->Release();
}

void DevTexture::Unload()
{
	if(tex)
	{
		tex->Release();
		tex = NULL;
	}
}

bool DevTexture::Load(const char *path)
{
	Unload();

	static const char *EXT[] = {
		"",".png",".jpg",".tga",".bmp",".dds",".hdr",NULL
	};

	HRESULT res;
	string spath;
	for(int i=0;EXT[i];i++)
	{
		spath = format("%s%s",path,EXT[i]);

		res = D3DXCreateTextureFromFileEx(
			Dev.GetDevice(),
			spath.c_str(),
			D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			0, 0,
			D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
			D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, (IDirect3DTexture9**)&tex);

		if(SUCCEEDED(res) && tex)
		{
			path = spath.c_str();
			break;
		}
	}

	if(FAILED(res) || !tex)
		return false;

	IDirect3DTexture9 *ctex = (IDirect3DTexture9*)tex;
	D3DSURFACE_DESC cd, ad, dd;
	res = ctex->GetLevelDesc(0,&cd);
	if(FAILED(res) || (cd.Format!=D3DFMT_A8R8G8B8 && cd.Format!=D3DFMT_X8R8G8B8))
		return true;

	string path_a = FilePathGetPart(path,true,true,false) + "_a" + FilePathGetPart(path,false,false,true);
	FILE *fp = NULL;
	fopen_s(&fp,path_a.c_str(),"rb");
	if(!fp) return true;
	fclose(fp);

	IDirect3DTexture9 *atex = NULL;
	res = D3DXCreateTextureFromFileEx(
			Dev.GetDevice(),path_a.c_str(),D3DX_DEFAULT_NONPOW2,D3DX_DEFAULT_NONPOW2,0,0,D3DFMT_X8R8G8B8,D3DPOOL_MANAGED,
			D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, (IDirect3DTexture9**)&atex);

	if(FAILED(res) || !atex)
		return true;

	IDirect3DTexture9 *dtex = NULL;
	bool all_ok = false;
	do {
		if(ctex->GetLevelCount() != atex->GetLevelCount())
			break;

		res = Dev->CreateTexture(cd.Width,cd.Height,ctex->GetLevelCount(),cd.Usage,D3DFMT_A8R8G8B8,cd.Pool,&dtex,NULL);
		if(FAILED(res) || !dtex) break;

		if(dtex->GetLevelCount() != ctex->GetLevelCount())
			break;

		for(int i=0;i<(int)ctex->GetLevelCount();i++)
		{
			if(FAILED( ctex->GetLevelDesc(i,&cd) )) break;
			if(FAILED( atex->GetLevelDesc(i,&ad) )) break;
			if(FAILED( dtex->GetLevelDesc(i,&dd) )) break;

			if(cd.Format!=D3DFMT_A8R8G8B8 && cd.Format!=D3DFMT_X8R8G8B8) break;
			if(ad.Format!=D3DFMT_A8R8G8B8 && ad.Format!=D3DFMT_X8R8G8B8) break;
			if(dd.Format!=D3DFMT_A8R8G8B8) break;
			if(cd.Width  != ad.Width ) break;
			if(cd.Height != ad.Height) break;
			if(cd.Width  != dd.Width ) break;
			if(cd.Height != dd.Height) break;

			D3DLOCKED_RECT clr, alr, dlr;
			if(FAILED( atex->LockRect(i,&alr,NULL,D3DLOCK_READONLY) )) break;
			
			if(FAILED( ctex->LockRect(i,&clr,NULL,D3DLOCK_READONLY) ))
			{
				atex->UnlockRect(i);
				break;
			}

			if(FAILED( dtex->LockRect(i,&dlr,NULL,0) ))
			{
				atex->UnlockRect(i);
				ctex->UnlockRect(i);
				break;
			}

			for(int y=0;y<(int)cd.Height;y++)
			{
				DWORD *cline = (DWORD*)(((byte*)clr.pBits) + clr.Pitch*y);
				DWORD *aline = (DWORD*)(((byte*)alr.pBits) + alr.Pitch*y);
				DWORD *dline = (DWORD*)(((byte*)dlr.pBits) + dlr.Pitch*y);
				for(int x=0;x<(int)cd.Width;x++)
				{
					*dline = (*cline & 0x00FFFFFF) | ((*aline)<<24);
					cline++;
					aline++;
					dline++;
				}
			}

			ctex->UnlockRect(i);
			atex->UnlockRect(i);
			dtex->UnlockRect(i);
		}

		all_ok = true;

	} while(false);

	if(all_ok)
	{
		tex->Release();
		tex = dtex;
	}
	else
	{
		if(dtex) dtex->Release();
	}
	
	atex->Release();

	return true;
}

bool DevTexture::LoadF(const char *path,D3DFORMAT fmt)
{
	Unload();

	static const char *EXT[] = {
		"",".png",".jpg",".tga",".bmp",".dds",".hdr",NULL
	};

	HRESULT res;
	string spath;
	for(int i=0;EXT[i];i++)
	{
		spath = format("%s%s",path,EXT[i]);

		res = D3DXCreateTextureFromFileEx(
			Dev.GetDevice(),
			spath.c_str(),
			D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
			0, 0,
			fmt, D3DPOOL_MANAGED,
			D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, (IDirect3DTexture9**)&tex);

		if(SUCCEEDED(res) && tex)
		{
			path = spath.c_str();
			break;
		}
	}

	if(FAILED(res) || !tex)
		return false;

	return true;
}

bool DevTexture::LoadCube(const char *path)
{
	Unload();
	HRESULT res = D3DXCreateCubeTextureFromFile(Dev.GetDevice(),path,(IDirect3DCubeTexture9**)&tex);
	if(!FAILED(res) && tex)
		return true;

	string path1 = FilePathGetPart(path,true,true,false);
	string path2 = FilePathGetPart(path,false,false,true);
	DevTexture t2;

	static const char *SIDES[6] = {
		"_xp", "_xm", "_yp", "_ym", "_zp", "_zm"
	};

	int size = 0;
	for(int i=0;i<6;i++)
	{
		bool ok = false;
		do {
		
			if(!t2.Load((path1 + SIDES[i] + path2).c_str()) || !t2.tex)
				break;

			D3DSURFACE_DESC desc;
			if(FAILED(((IDirect3DTexture9*)t2.tex)->GetLevelDesc(0,&desc)))
				break;

			if(desc.Format!=D3DFMT_A8R8G8B8 && desc.Format!=D3DFMT_X8R8G8B8)
				break;

			if(!size)
			{
				size = desc.Width;
				if(FAILED(Dev->CreateCubeTexture(size,0,D3DUSAGE_AUTOGENMIPMAP,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,
												 (IDirect3DCubeTexture9**)&tex,NULL)) || !tex)
					 break;
			}

			if(desc.Width!=size || desc.Height!=size)
				break;

			D3DLOCKED_RECT tlr, clr;
			if(FAILED(((IDirect3DTexture9*)t2.tex)->LockRect(0,&tlr,NULL,0)))
				break;

			if(FAILED(((IDirect3DCubeTexture9*)tex)->LockRect((D3DCUBEMAP_FACES)i,0,&clr,NULL,0)))
				break;

			for(int y=0;y<size;y++)
			{
				DWORD *tline = (DWORD*)(((byte*)tlr.pBits) + tlr.Pitch*y);
				DWORD *cline = (DWORD*)(((byte*)clr.pBits) + clr.Pitch*y);
				memcpy(cline,tline,4*size);
			}

			ok = true;
		} while(0);

		if(t2.tex) ((IDirect3DTexture9*)t2.tex)->UnlockRect(0);
		if(tex) ((IDirect3DCubeTexture9*)tex)->UnlockRect((D3DCUBEMAP_FACES)i,0);

		if(!ok)
		{
			Unload();
			return false;
		}
	}

	return true;
}

bool DevTexture::LoadRaw2D(int format,int w,int h,const void *data,bool use_mipmaps)
{
	IDirect3DTexture9 *ttex = NULL;
	
	Unload();

	int bpp = Device::GetSurfaceSize((D3DFORMAT)format,1,1);
	if(bpp<=0)
	{
		assert(!"LoadRaw2D: unsupported format specified");
		return false;
	}

	if(FAILED(Dev->CreateTexture(w,h,use_mipmaps?1:0,use_mipmaps?D3DUSAGE_AUTOGENMIPMAP:0,
										(D3DFORMAT)format,D3DPOOL_MANAGED,&ttex,NULL)))
		return false;

	if(!ttex)
		return false;

	D3DLOCKED_RECT lr;

	bool done = false;
	do
	{
		if(FAILED(ttex->LockRect(0,&lr,NULL,0)))
			break;

		for(int y=0;y<h;y++)
		{
			byte *dst = ((byte*)lr.pBits)+lr.Pitch*y;
			memcpy(dst,((byte*)data)+y*w*bpp,bpp*w);
		}

		ttex->UnlockRect(0);
		done = true;
	} while(0);

	if(!done)
	{
		ttex->Release();
		return false;
	}

	ttex->GenerateMipSubLevels();
	tex = ttex;

	return true;
}

bool DevTexture::CreateEmpty2D(int format,int w,int h,bool create_mipmaps,bool autogen_mipmaps)
{
	IDirect3DTexture9 *ttex = NULL;

	Unload();

	if(FAILED(Dev->CreateTexture(w,h,create_mipmaps?0:1,autogen_mipmaps?D3DUSAGE_AUTOGENMIPMAP:0,
									(D3DFORMAT)format,D3DPOOL_MANAGED,&ttex,NULL)))
		return false;

	if(!ttex)
		return false;

	tex = ttex;

	return true;
}

bool DevTexture::BuildLookup2D(int w,int h,DWORD (*fn)(float,float),bool use_mipmaps)
{
	IDirect3DTexture9 *ttex = NULL;

	Unload();

	if(FAILED(Dev->CreateTexture(w,h,use_mipmaps?0:1,use_mipmaps?D3DUSAGE_AUTOGENMIPMAP:0,
										D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&ttex,NULL)))
		return false;

	if(!ttex)
		return false;

	D3DLOCKED_RECT lr;
	float u,dx,dy;
	dx = 1.f/float(w);
	dy = 1.f/float(h);

	bool done = false;
	do
	{
		if(FAILED(ttex->LockRect(0,&lr,NULL,0)))
			break;

		for(int y=0;y<h;y++)
		{
			DWORD *data = (DWORD*)(((char*)lr.pBits)+lr.Pitch*y);
			float v = (0.5f+y)*dy;
			for(u=dx*0.5f;u<1.f;u+=dx)
				*data++ = fn(u,v);
		}

		ttex->UnlockRect(0);
		done = true;
	} while(0);

	if(!done)
	{
		ttex->Release();
		return false;
	}

	ttex->GenerateMipSubLevels();
	tex = ttex;

	return true;
}

/*
bool DevTexture::BuildLookup3D(int sx,int sy,int sz,DWORD (*fn)(float,float,float),bool use_mapmaps)
{
	Unload();

	// TODO: implement
	return false;
}
*/

bool DevTexture::BuildLookupCube(int size,DWORD (*fn)(float,float,float),bool use_mipmaps)
{
	IDirect3DCubeTexture9 *ctex = NULL;

	Unload();

	if(FAILED(Dev->CreateCubeTexture(size,use_mipmaps?0:1,use_mipmaps?D3DUSAGE_AUTOGENMIPMAP:0,
										D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&ctex,NULL)))
		return false;

	if(!ctex)
		return false;

	D3DLOCKED_RECT lr;
	int face;
	float u,v,dt;
	float x,y,z;

	dt = 2.f/float(size);

	for(face=0;face<6;face++)
	{
		if(FAILED(ctex->LockRect((D3DCUBEMAP_FACES)face,0,&lr,NULL,0)))
			break;

		DWORD *data = (DWORD *)lr.pBits;

		for(v=-1.f+dt*0.5f;v<1.f;v+=dt)
			for(u=-1.f+dt*0.5f;u<1.f;u+=dt)
			{
				switch(face)
				{
					case 0: x= 1; y=-v; z=-u; break;
					case 1: x=-1; y=-v; z= u; break;
					case 2: x= u; y= 1; z= v; break;
					case 3: x= u; y=-1; z=-v; break;
					case 4: x= u; y=-v; z= 1; break;
					case 5: x=-u; y=-v; z=-1; break;
				}

				*data++ = fn(x,y,z);
			}

		ctex->UnlockRect((D3DCUBEMAP_FACES)face,0);
	}

	if(face<6)
	{
		ctex->Release();
		return false;
	}

	ctex->GenerateMipSubLevels();
	tex = ctex;

	return true;
}

bool DevTexture::GetRawData(int &width,int &height,vector<DWORD> &data)
{
	width = 0;
	height = 0;
	data.clear();
	if(!tex) return false;
	if(tex->GetType() != D3DRTYPE_TEXTURE)
		return false;

	IDirect3DTexture9 *tex2d = (IDirect3DTexture9 *)tex;
	
	D3DSURFACE_DESC desc;
	if(FAILED(tex2d->GetLevelDesc(0,&desc)))
		return false;

	if(desc.Format!=D3DFMT_A8R8G8B8 && desc.Format!=D3DFMT_X8R8G8B8)
		return false;

	D3DLOCKED_RECT lr;
	if(FAILED(tex2d->LockRect(0,&lr,NULL,0)))
		return false;

	data.resize(desc.Width*desc.Height);
	for(int y=0;y<(int)desc.Height;y++)
	{
		DWORD *src = (DWORD*)(((char*)lr.pBits)+lr.Pitch*y);
		memcpy(&data[y*desc.Width],src,desc.Width*4);
	}

	tex2d->UnlockRect(0);

	width = desc.Width;
	height = desc.Height;

	return true;
}

vec2 DevTexture::GetSize2D()
{
	if(!tex) return vec2(0,0);
	if(tex->GetType() != D3DRTYPE_TEXTURE)
		return vec2(0,0);

	IDirect3DTexture9 *tex2d = (IDirect3DTexture9 *)tex;
	
	D3DSURFACE_DESC desc;
	if(FAILED(tex2d->GetLevelDesc(0,&desc)))
		return vec2(0,0);

	return vec2(float(desc.Width),float(desc.Height));
}

void DevTexture::OnBeforeReset()
{
	// nothing to do here
}

void DevTexture::OnAfterReset()
{
	// nothing to do here
}

void DevTexture::OnPreload()
{
	if(preload_path.size()>0)
		Load(preload_path.c_str());
}
