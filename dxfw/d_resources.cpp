
#include "dxfw.h"

using namespace std;
using namespace base;




// **************** DevRenderTarget ****************


DevRenderTarget::DevRenderTarget()
{
	rt = NULL;
	format = D3DFMT_A8R8G8B8;
	width = 0;
	height = 0;
	denominator = 1;
	flags = 0;
	needs_fill = true;
}

DevRenderTarget::DevRenderTarget(int _format,int _width,int _height,int _denominator,int _flags)
{
	rt = NULL;
	format = _format;
	width = _width;
	height = _height;
	denominator = _denominator;
	flags = _flags;
	needs_fill = true;
}

DevRenderTarget::~DevRenderTarget()
{
	if(rt) rt->Release();
	rt = NULL;
}

IDirect3DSurface9 *DevRenderTarget::GetSurface()
{
	int w, h, m=1;
	GetCurrentSize(w,h);
	if(!rt)
	{
		needs_fill = true;

		DWORD n_usage = 0;
		if(!Device::IsFormatDepthStencil((D3DFORMAT)format))
			n_usage |= D3DUSAGE_RENDERTARGET;

		if(flags & (TEXF_MIPMAPS | TEXF_AUTO_MIPMAPS))
			m = 0;

		if(flags & TEXF_AUTO_MIPMAPS)
			n_usage |= D3DUSAGE_AUTOGENMIPMAP;

		HRESULT res = Dev->CreateTexture(w,h,m,n_usage,(D3DFORMAT)format,D3DPOOL_DEFAULT,&rt,NULL);
		if(FAILED(res) || !rt) return NULL;
	}

	IDirect3DSurface9 *surf = NULL;
	rt->GetSurfaceLevel(0,&surf);
	return surf;
}

void DevRenderTarget::SetParameters(int _format,int _width,int _height,int _denominator,int _flags)
{
	if(format==_format && width==_width && height==_height && denominator==_denominator && flags==_flags)
		return;

	if(rt) rt->Release();
	rt = NULL;

	format = _format;
	width = _width;
	height = _height;
	denominator = _denominator;
	flags = _flags;
	needs_fill = true;
}

void DevRenderTarget::GetCurrentSize(int &sx,int &sy)
{
	if(denominator>0)
	{
		int dsx, dsy;
		Dev.GetScreenSize(dsx,dsy);
		sx = (dsx + (denominator-1))/denominator;
		sy = (dsy + (denominator-1))/denominator;
	}
	else
	{
		sx = width;
		sy = height;
	}
}

vec2 DevRenderTarget::GetCurrentSizeV()
{
	int sx, sy;
	GetCurrentSize(sx,sy);
	return vec2((float)sx,(float)sy);
}

bool DevRenderTarget::Save(const char *path)
{
	IDirect3DSurface9 *surf = GetSurface();
	if(!surf) return false;

	D3DXSaveSurfaceToFile(path,D3DXIFF_PNG,surf,NULL,NULL);
	surf->Release();

	return true;
}


bool DevRenderTarget::GetPixels(int &w,int &h,vector<DWORD> &data)
{
	w = 0;
	h = 0;
	data.clear();

	int bpp = Device::GetSurfaceSize((D3DFORMAT)format,1,1);	// bytes per pixel
	if(!bpp || bpp%4!=0)
		return false;

	IDirect3DSurface9 *rts = GetSurface();
	if(!rts) return false;


    IDirect3DSurface9 *off = NULL;
    D3DLOCKED_RECT srcr;
	D3DSURFACE_DESC desc;
    bool ok = false;

    do {
		if(FAILED(rts->GetDesc(&desc))) break;
		w = desc.Width;
		h = desc.Height;
		data.resize(w*h*(bpp/4));

        if(FAILED(Dev->CreateOffscreenPlainSurface(w,h,(D3DFORMAT)format,D3DPOOL_SYSTEMMEM,&off,NULL))) break;
        if(FAILED(Dev->GetRenderTargetData(rts,off))) break;
        if(FAILED(off->LockRect(&srcr,NULL,0))) break;

        for(int y=0;y<h;y++)
        {
            DWORD *s = (DWORD*)( (byte*)srcr.pBits + srcr.Pitch*y );
            memcpy(&data[y*w*(bpp/4)],s,w*bpp);
        }

        off->UnlockRect();
        ok = true;
    } while(0);

    if(rts) rts->Release();
    if(off) off->Release();

	if(!ok)
	{
		w = 0;
		h = 0;
		data.clear();
	}

    return ok;
}



void DevRenderTarget::OnBeforeReset()
{
	if(rt) rt->Release();
	rt = NULL;
	needs_fill = true;
}

void DevRenderTarget::OnAfterReset()
{
	needs_fill = true;
}


// **************** DevCubeRenderTarget ****************


DevCubeRenderTarget::DevCubeRenderTarget()
{
	rt = NULL;
	format = D3DFMT_A8R8G8B8;
	size = 0;
	flags = 0;
	needs_fill = true;
}

DevCubeRenderTarget::DevCubeRenderTarget(int _format,int _size,int _flags)
{
	rt = NULL;
	format = _format;
	size = _size;
	flags = _flags;
	needs_fill = true;
}

DevCubeRenderTarget::~DevCubeRenderTarget()
{
	if(rt) rt->Release();
	rt = NULL;
}

IDirect3DSurface9 *DevCubeRenderTarget::GetSurface(int face)
{
	return GetSurfaceLevel(face,0);
}

IDirect3DSurface9 *DevCubeRenderTarget::GetSurfaceLevel(int face,int level)
{
	if(!rt)
	{
		needs_fill = true;

		DWORD n_usage = 0;
		if( !Device::IsFormatDepthStencil((D3DFORMAT)format) )
			n_usage |= D3DUSAGE_RENDERTARGET;

		int mips = 1;
		if( flags & (TEXF_MIPMAPS | TEXF_AUTO_MIPMAPS) )
			mips = 0;

		if( flags & TEXF_AUTO_MIPMAPS )
			n_usage |= D3DUSAGE_AUTOGENMIPMAP;

		HRESULT res = Dev->CreateCubeTexture(size,mips,n_usage,(D3DFORMAT)format,D3DPOOL_DEFAULT,&rt,NULL);
		if(FAILED(res) || !rt) return NULL;
	}

	IDirect3DSurface9 *surf = NULL;
	rt->GetCubeMapSurface((D3DCUBEMAP_FACES)face,level,&surf);
	return surf;
}


void DevCubeRenderTarget::SetParameters(int _format,int _size,int _flags)
{
	if(format==_format && size==_size && flags==_flags)
		return;

	if(rt) rt->Release();
	rt = NULL;

	format = _format;
	size = _size;
	flags = _flags;
	needs_fill = true;
}

void DevCubeRenderTarget::OnBeforeReset()
{
	if(rt) rt->Release();
	rt = NULL;
	needs_fill = true;
}

void DevCubeRenderTarget::OnAfterReset()
{
	needs_fill = true;
}



// **************** DevDepthStencilSurface ****************


DevDepthStencilSurface::DevDepthStencilSurface(int _format,int _width,int _height,int _denominator)
{
	surf = NULL;
	format = _format;
	width = _width;
	height = _height;
	denominator = _denominator;
}

DevDepthStencilSurface::~DevDepthStencilSurface()
{
	if(surf) surf->Release();
	surf = NULL;
}

IDirect3DSurface9 *DevDepthStencilSurface::GetSurface()
{
	int w, h;
	GetCurrentSize(w,h);
	if(!surf)
	{
		HRESULT res = Dev->CreateDepthStencilSurface(w,h,(D3DFORMAT)format,D3DMULTISAMPLE_NONE,0,false,&surf,NULL);
		if(FAILED(res) || !surf)
		{
//			MessageBox(NULL,base::format("Failed to create secondary z/stencil surface (code %08x)",res).c_str(),"Error!",MB_OK);
			return NULL;
		}
	}
	return surf;
}

void DevDepthStencilSurface::GetCurrentSize(int &sx,int &sy)
{
	if(denominator>0)
	{
		int dsx, dsy;
		Dev.GetScreenSize(dsx,dsy);
		sx = (dsx + (denominator-1))/denominator;
		sy = (dsy + (denominator-1))/denominator;
	}
	else
	{
		sx = width;
		sy = height;
	}
}

void DevDepthStencilSurface::OnBeforeReset()
{
	if(surf) surf->Release();
	surf = NULL;
}

void DevDepthStencilSurface::OnAfterReset()
{
}




// **************** DevFont ****************

DevFont::DevFont()
{
	font = NULL;
	do_preload = false;
}

DevFont::DevFont(const char *facename,int height,int width,bool bold,bool italic)
{
	font = NULL;
	do_preload = false;
	if(Dev.GetIsReady())
		Create(facename,height,width,bold,italic);
	else
	{
		do_preload = true;
		p_facename = facename;
		p_height = height;
		p_width = width;
		p_bold = bold;
		p_italic = italic;
	}
}

DevFont::~DevFont()
{
	if(font)
		font->Release();
}

bool DevFont::Create(const char *facename,int height,int width,bool bold,bool italic)
{
	if(font)
		font->Release();
	font = NULL;
	do_preload = false;

	HRESULT res = D3DXCreateFont(Dev.GetDevice(),height,width,bold?700:400,3,italic,
									DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,ANTIALIASED_QUALITY,
									FF_DONTCARE,facename,&font);
	return SUCCEEDED(res);
}

void DevFont::OnBeforeReset()
{
	if(font)
		font->OnLostDevice();
}

void DevFont::OnAfterReset()
{
	if(font)
		font->OnResetDevice();
}
