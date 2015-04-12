
#include "dxfw.h"

using namespace std;
using namespace base;



static void _conv_A8R8G8B8_u(DWORD rgba,void *out)				{ *(DWORD*)out = rgba; }
static void _conv_R5G6B5_u(DWORD rgba,void *out)				{ *(word*)out = ((rgba>>8)&0xF800) | ((rgba>>5)&0x07E0) | ((rgba>>3)&0x001F); }
static void _conv_A1R5G5B5_u(DWORD rgba,void *out)				{ *(word*)out = ((rgba>>31)&0x8000) | ((rgba>>9)&0x7C00) | ((rgba>>6)&0x03E0) | ((rgba>>3)&0x001F); }
static void _conv_X1R5G5B5_u(DWORD rgba,void *out)				{ *(word*)out = ((rgba>>9)&0x7C00) | ((rgba>>6)&0x03E0) | ((rgba>>3)&0x001F); }
static void _conv_A4R4G4B4_u(DWORD rgba,void *out)				{ *(word*)out = ((rgba>>16)&0xF000) | ((rgba>>12)&0x0F00) | ((rgba>>8)&0x00F0) | ((rgba>>4)&0x000F); }
static void _conv_X4R4G4B4_u(DWORD rgba,void *out)				{ *(word*)out = ((rgba>>12)&0x0F00) | ((rgba>>8)&0x00F0) | ((rgba>>4)&0x000F); }
static void _conv_A8R3G3B2_u(DWORD rgba,void *out)				{ *(word*)out = ((rgba>>16)&0xFF00) | ((rgba>>16)&0x00E0) | ((rgba>>11)&0x001C) | ((rgba>>6)&0x0003); }
static void _conv_R3G3B2_u(DWORD rgba,void *out)				{ *(byte*)out = ((rgba>>16)&0xE0) | ((rgba>>11)&0x1C) | ((rgba>>6)&0x03); }
static void _conv_A8_u(DWORD rgba,void *out)					{ *(byte*)out = rgba>>24; }
static void _conv_L8_u(DWORD rgba,void *out)					{ *(byte*)out = byte(rgba); }
static void _conv_A8L8_u(DWORD rgba,void *out)					{ *(word*)out = (rgba>>16) | (rgba&0xFF); }

static void _conv_A8R8G8B8_f(const float *rgba,void *out)		{ *(DWORD*)out = ((vec3*)rgba)->make_rgba(rgba[3]); }
static void _conv_X8R8G8B8_f(const float *rgba,void *out)		{ *(DWORD*)out = ((vec3*)rgba)->make_rgb(); }
static void _conv_R5G6B5_f(const float *rgba,void *out)			{ DWORD tmp = ((vec3*)rgba)->make_rgb(); 
																  *(word*)out = ((tmp>>8)&0xF800) | ((tmp>>5)&0x07E0) | ((tmp>>3)&0x001F);
																}
static void _conv_A8_f(const float *rgba,void *out)				{ float tmp = rgba[3]*(255.f/256.f)+1.0f; *(byte*)out = byte((*(DWORD*)&tmp)>>15); }
static void _conv_L8_f(const float *rgba,void *out)				{ float tmp = rgba[0]*(255.f/256.f)+1.0f; *(byte*)out = byte((*(DWORD*)&tmp)>>15); }
static void _conv_L16_f(const float *rgba,void *out)			{ float tmp = rgba[0]*(65535.f/65536.f)+1.0f; *(word*)out = word((*(DWORD*)&tmp)>>7); }
static void _conv_A8L8_f(const float *rgba,void *out)			{ float l = rgba[0]*(255.f/256.f)+1.0f; 
																  float a = rgba[3]*(255.f/256.f)+1.0f;
																  *(word*)out = (((*(DWORD*)&a)>>7)&0xFF00) | (((*(DWORD*)&l)>>15)&0x00FF);
																}
static void _conv_G16R16_f(const float *rgba,void *out)			{ float r = rgba[0]*(65535.f/65536.f)+1.0f;
																  float g = rgba[1]*(65535.f/65536.f)+1.0f;
																  *(DWORD*)out = ((*(DWORD*)&g)<<9); *(word*)out = word((*(DWORD*)&r)>>7);
																}
static void _conv_A16B16G16R16_f(const float *rgba,void *out)	{ float r = rgba[0]*(65535.f/65536.f)+1.0f;
																  float g = rgba[1]*(65535.f/65536.f)+1.0f;
																  float b = rgba[2]*(65535.f/65536.f)+1.0f;
																  float a = rgba[3]*(65535.f/65536.f)+1.0f;
																  *(DWORD*)out = ((*(DWORD*)&g)<<9); *(word*)out = word((*(DWORD*)&r)>>7);
																  ((DWORD*)out)[1] = ((*(DWORD*)&a)<<9); ((word*)out)[2] = word((*(DWORD*)&b)>>7);
																}
static void _conv_R32F_f(const float *rgba,void *out)			{ *(float*)out = rgba[0]; }
static void _conv_G32R32F_f(const float *rgba,void *out)		{ *(int64*)out = *(int64*)rgba; }
static void _conv_A32B32G32R32F_f(const float *rgba,void *out)	{ *(int64*)out = *(int64*)rgba; ((int64*)out)[1] = ((int64*)rgba)[1]; }




struct FormatDef {
	D3DFORMAT	format;
	char		*descr;
	int			bpp;
	void		(*fn_convert_u)(DWORD rgba,void *out);
	void		(*fn_convert_f)(const float *rgba,void *out);
};


#define FMT(name)	D3DFMT_ ## name, #name

static const FormatDef _FORMATS[] = {
	// BackBuffer, Display and Unsigned
	{ FMT( A2R10G10B10	), 4	,NULL	, NULL	},
	{ FMT( A8R8G8B8		), 4	,_conv_A8R8G8B8_u	,_conv_A8R8G8B8_f		},
	{ FMT( X8R8G8B8		), 4	,_conv_A8R8G8B8_u	,_conv_X8R8G8B8_f		},
	{ FMT( A1R5G5B5		), 2	,_conv_A1R5G5B5_u	,NULL					},
	{ FMT( X1R5G5B5		), 2	,_conv_X1R5G5B5_u	,NULL					},
	{ FMT( R5G6B5		), 2	,_conv_R5G6B5_u		,_conv_R5G6B5_f			},
//	R8G8B8
	{ FMT( A4R4G4B4		), 2	,_conv_A4R4G4B4_u	,NULL					},
	{ FMT( R3G3B2		), 1	,_conv_R3G3B2_u		,NULL					},
	{ FMT( A8			), 1	,_conv_A8_u			,_conv_A8_f				},
	{ FMT( A8R3G3B2		), 2	,_conv_A8R3G3B2_u	,NULL					},
	{ FMT( X4R4G4B4		), 2	,_conv_X4R4G4B4_u	,NULL					},
//	A2B10G10R10
//	A8B8G8R8
//	X8B8G8R8
	{ FMT( G16R16		), 4	,NULL				,_conv_G16R16_f			},
	{ FMT( A16B16G16R16	), 8	,NULL				,_conv_A16B16G16R16_f	},
//	A8P8
//	P8
	{ FMT( L8			), 1	,_conv_L8_u			,_conv_L8_f				},
	{ FMT( L16			), 2	,NULL				,_conv_L16_f			},
	{ FMT( A8L8			), 2	,_conv_A8L8_u		,_conv_A8L8_f			},
	{ FMT( A4L4			), 1	,NULL				,NULL					},

	// Depth-Stencil
	{ FMT( D16_LOCKABLE	), 2	,NULL				,NULL					},
//	D32
//	D15S1
	{ FMT( D24S8		), 0	,NULL				,NULL					},
	{ FMT( D24X8		), 0	,NULL				,NULL					},
//	D24X4S4
	{ FMT( D32F_LOCKABLE), 4	,NULL				,NULL					},
//	D24FS8
	{ FMT( D16			), 0	,NULL				,NULL					},
	// DXTn Compressed
	{ FMT( DXT1			), 0	,NULL				,NULL					},
	{ FMT( DXT2			), 0	,NULL				,NULL					},
	{ FMT( DXT3			), 0	,NULL				,NULL					},
	{ FMT( DXT4			), 0	,NULL				,NULL					},
	{ FMT( DXT5			), 0	,NULL				,NULL					},
	// Floating-Point
	{ FMT( R16F			), 2	,NULL				,NULL					},
	{ FMT( G16R16F		), 4	,NULL				,NULL					},
	{ FMT( A16B16G16R16F), 8	,NULL				,NULL					},
	// FOURCC
//	MULTI2_ARGB8
//	G8R8_G8B8
//	R8G8_B8G8
//	UYVY
//	YUY2
	// IEEE
	{ FMT( R32F			), 4	,NULL				,_conv_R32F_f			},
	{ FMT( G32R32F		), 8	,NULL				,_conv_G32R32F_f		},
	{ FMT( A32B32G32R32F),16	,NULL				,_conv_A32B32G32R32F_f	},
	// Mixed
//	L6V5U5
//	X8L8V8U8
//	A2W10V10U10
	// Signed
	{ FMT( V8U8			), 2	,NULL				,NULL					},
	{ FMT( Q8W8V8U8		), 4	,NULL				,NULL					},
	{ FMT( V16U16		), 4	,NULL				,NULL					},
	{ FMT( Q16W16V16U16	), 8	,NULL				,NULL					},
	{ FMT( CxV8U8		), 2	,NULL				,NULL					},

	// End
	{ D3DFMT_UNKNOWN, NULL, }
};

#undef FMT


static const FormatDef *_find_format(D3DFORMAT format)
{
	const FormatDef *fd = _FORMATS;
	while(fd->format!=D3DFMT_UNKNOWN)
	{
		if(fd->format==format)
			return fd;
		fd++;
	}
	return NULL;
}



int Device::GetSurfaceSize(D3DFORMAT format,int w,int h)
{
	const FormatDef *fd = _find_format(format);
	return fd ? (w*h*fd->bpp) : 0;
}

Device::fn_convert_u_t *Device::GetFormatConversionFunctionU(D3DFORMAT format)
{
	const FormatDef *fd = _find_format(format);
	return fd ? fd->fn_convert_u : NULL;
}

Device::fn_convert_f_t *Device::GetFormatConversionFunctionF(D3DFORMAT format)
{
	const FormatDef *fd = _find_format(format);
	return fd ? fd->fn_convert_f : NULL;
}

bool Device::IsFormatDepthStencil(D3DFORMAT format)
{
	return (format==D3DFMT_D24S8		|| format==D3DFMT_D16			|| format==D3DFMT_D24X8  ||
			format==D3DFMT_D16_LOCKABLE	|| format==D3DFMT_D24X4S4		|| format==D3DFMT_D15S1  ||
			format==D3DFMT_D32			|| format==D3DFMT_D32F_LOCKABLE || format==D3DFMT_D24FS8	);
}
