
#include "dxfw.h"

using namespace std;
using namespace base;


Device Dev;



static const float CUBE_AXIS[6][3][3] = {
	{ { 0, 0,-1}, { 0, 1, 0}, { 1, 0, 0} },
	{ { 0, 0, 1}, { 0, 1, 0}, {-1, 0, 0} },
	{ { 1, 0, 0}, { 0, 0,-1}, { 0, 1, 0} },
	{ { 1, 0, 0}, { 0, 0, 1}, { 0,-1, 0} },
	{ { 1, 0, 0}, { 0, 1, 0}, { 0, 0, 1} },
	{ {-1, 0, 0}, { 0, 1, 0}, { 0, 0,-1} },
};



// **************** IDevResource ****************

IDevResource::IDevResource()
{
	DeviceResourceManager::GetInstance()->Register(this);
}

IDevResource::~IDevResource()
{
	DeviceResourceManager *drm = DeviceResourceManager::PeekInstance();
	if(drm)
		drm->Unregister(this);
}


// **************** DeviceResourceManager ****************

DeviceResourceManager				*DeviceResourceManager::singleton;
DeviceResourceManager::DRMHandler	DeviceResourceManager::handler;



void DeviceResourceManager::Register(IDevResource *res)
{
	Unregister(res);
	reslist.push_back(res);
}

void DeviceResourceManager::Unregister(IDevResource *res)
{
	for(int i=0;i<(int)reslist.size();i++)
		if(reslist[i]==res)
			reslist[i] = NULL;
	CompactTable();
}

void DeviceResourceManager::SendOnBeforeReset()
{
	for(int pr=1;pr<=16;pr++)
		for(int i=0;i<(int)reslist.size();i++)
			if(reslist[i] && reslist[i]->GetPriority()==(pr&0xF))
				reslist[i]->OnBeforeReset();
}

void DeviceResourceManager::SendOnAfterReset()
{
	for(int pr=1;pr<=16;pr++)
		for(int i=0;i<(int)reslist.size();i++)
			if(reslist[i] && reslist[i]->GetPriority()==(pr&0xF))
				reslist[i]->OnAfterReset();
}

void DeviceResourceManager::SendOnPreload()
{
	for(int pr=1;pr<=16;pr++)
		for(int i=0;i<(int)reslist.size();i++)
			if(reslist[i] && reslist[i]->GetPriority()==(pr&0xF))
				reslist[i]->OnPreload();
}

void DeviceResourceManager::SendOnReloadTick()
{
	for(int i=0;i<(int)reslist.size();i++)
		if(reslist[i])
			reslist[i]->OnReloadTick();
}

void DeviceResourceManager::CompactTable()
{
	int s=0,d=0;
	while(s<(int)reslist.size())
	{
		if(reslist[s]!=NULL)
			reslist[d++] = reslist[s];
		s++;
	}
	reslist.resize(d);
}



// **************** Device ****************


static LRESULT WINAPI _DeviceMsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	LONG_PTR res = GetWindowLongPtr(hWnd,GWL_USERDATA);
	if(res)
		return ((Device*)res)->MsgProc(hWnd,msg,wParam,lParam);
	return DefWindowProc( hWnd, msg, wParam, lParam );
}




Device::Device() : default_font("Verdana",12,0,false,false)
{
	d3d					= NULL;
	dev					= NULL;
	d3d_hWnd			= NULL;
	d3d_need_reset		= false;
	d3d_sizing			= false;
	d3d_screen_w		= 1024;
	d3d_screen_h		=  768;
	d3d_msaa			= 0;
	d3d_windowed		= false;
	d3d_min_zstencil_w	= 0;
	d3d_min_zstencil_h	= 0;
	has_focus			= true;
	custom_zstencil		= NULL;
	custom_zstencil_rt	= NULL;
	block_surface[0]	= NULL;
	block_surface[1]	= NULL;
	block_id			= 0;

	set_resolution_first = true;
	quited = false;

	mouse_x = 0;
	mouse_y = 0;
	mouse_dx = 0;
	mouse_dy = 0;
	mouse_dz = 0;
	mouse_capture = false;

	time_prev = timeGetTime();
	time_delta = 0;
	time_elapsed = 0;
	time_first_frame = true;
	time_reload = 0;
	time_last_reload = 0;

	{
		char buff[256];
		buff[0] = 0;
		GetModuleFileName(NULL,buff,255);
		d3d_app_name = FilePathGetPart(buff,false,true,false);
		if(d3d_app_name.size()==0)
			d3d_app_name = "DirectX Application";
		else if(d3d_app_name[0]>='a' && d3d_app_name[0]<='z')
			d3d_app_name[0] += 'A' - 'a';
	}

	debug_cam_pos		= vec3(0,0,0);
	debug_cam_ypr		= vec3(0,0,0);
}

Device::~Device()
{
	Shutdown();
}

bool Device::BeginScene()
{
	has_focus = true;
	
	if(!dev)
		return false;

	HRESULT res = dev->TestCooperativeLevel();
	if(d3d_need_reset)
	{
		res = D3DERR_DEVICENOTRESET;
		d3d_need_reset = false;
	}
	if(FAILED(res))
	{
		if(res==D3DERR_DEVICENOTRESET)
		{
			SendOnBeforeReset();
			ReleaseInternalResources();

			if(FAILED(dev->Reset(GetPresentParameters())))
				return false;

			CreateInternalResources();
			SendOnAfterReset();

			dev->SetDepthStencilSurface(custom_zstencil);
			dev->SetRenderState(D3DRS_LIGHTING,FALSE);
		}
		else
			return false;
	}

	if(FAILED(dev->BeginScene()))
		return false;

	if(time_first_frame)
	{
		time_first_frame = false;
		time_prev = timeGetTime();
	}

	dprintf_buff.clear();

	if(d3d_hWnd)
		has_focus = (GetActiveWindow()==d3d_hWnd);

	return true;
}

void Device::EndScene()
{
	// debug print
	if(dprintf_buff.size()>0)
	{
		SetRenderTarget(0,NULL);
		SetDefaultRenderStates();
	
		for(int j=0;j<(int)dprintf_buff.size();j++)
		{
			for(int i=4;i>=0;i--)
			{
				static const int SEQ[6] = { 0, 0, -1, 0, 1, 0 };
				int dx = SEQ[i];
				int dy = SEQ[i+1] + j*14;
				RECT rect = { dx, dy, dx+4096, dy+4096 };
				default_font->DrawText(NULL,dprintf_buff[j].c_str(),-1,&rect,DT_TOP | DT_LEFT,i ? 0xFF000000 : 0xFFFFFFFF);
			}
		}
	}

	// end scene
	dev->EndScene();

	if(block_surface[0] && block_surface[1])
	{
		D3DLOCKED_RECT lr;
		volatile int dummy;

		static DWORD fill = 0x1234567;
		fill += 0x1243AB;
		dev->ColorFill(block_surface[block_id],0,fill);
		block_id = !block_id;

		if(!FAILED((block_surface[block_id]->LockRect(&lr,0,D3DLOCK_READONLY))))
		{
			dummy = *(int*)lr.pBits;
			block_surface[block_id]->UnlockRect();
		}
	}

	dev->Present( NULL, NULL, NULL, NULL );

	DWORD time = timeGetTime();
	time_delta = float(time-time_prev)*0.001f;
	time_prev = time;

	if(time_delta>0.3f) time_delta = 0.3f;
	time_elapsed += time_delta;

	if(time_reload>0 && time_elapsed-time_last_reload>=time_reload)
	{
		time_last_reload = time_elapsed;
		SendOnReloadTick();
	}

    // keyboard
    read_keys.clear();
}

bool Device::Sync()
{
	if(!block_surface[0])
		return false;

	D3DLOCKED_RECT lr;
	volatile int dummy;

	static DWORD fill = 0x1654123;
	fill += 0x1243AB;
	
	dev->ColorFill(block_surface[0],0,fill);

	if(FAILED((block_surface[block_id]->LockRect(&lr,0,D3DLOCK_READONLY))))
		return false;

	dummy = *(int*)lr.pBits;
	block_surface[0]->UnlockRect();

	return true;
}

void Device::SetResolution(int width,int height,bool fullscreen,int msaa)
{
	d3d_windowed = !fullscreen;

	if(!d3d_hWnd)
	{
		d3d_screen_w = width;
		d3d_screen_h = height;
		d3d_msaa = msaa;
		return;
	}

	config.SetInt( "Windowed", d3d_windowed?1:0 );
	config.SetInt( "ScreenWidth", width );
	config.SetInt( "ScreenHeight", height );
	config.SetInt( "MSAA", msaa);


	DWORD exstyle = d3d_windowed ?	(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE) :
									(WS_EX_APPWINDOW | WS_EX_TOPMOST);
	DWORD style = d3d_windowed ? WS_OVERLAPPEDWINDOW : WS_POPUP;

	if( config.GetInt("Topmost",0) )
		exstyle |= WS_EX_TOPMOST;

	SetWindowLong( d3d_hWnd, GWL_EXSTYLE, exstyle );
	SetWindowLong( d3d_hWnd, GWL_STYLE, style );

	if( d3d_windowed )
	{
		if( config.GetInt("Topmost",0) )	SetWindowPos(d3d_hWnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		else								SetWindowPos(d3d_hWnd,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	}


	//if(!fullscreen)
	//	ChangeDisplaySettings(NULL,0);

	RECT pos;
	if(fullscreen)
	{
		pos.left = 0;
		pos.top = 0;
	}
	else
	{
		SystemParametersInfo(SPI_GETWORKAREA,0,&pos,0);
		pos.left = (pos.left + pos.right - width)/2;
		pos.top = (pos.top + pos.bottom - height)/2;

		// Apply config
		int cfg_x = config.GetInt("WindowPositionX",pos.left);
		int cfg_y = config.GetInt("WindowPositionY",pos.top);
		if( cfg_x || cfg_y )
		{
			pos.left = cfg_x;
			pos.top  = cfg_y;
		}
	}
	pos.right = pos.left + width;
	pos.bottom = pos.top + height;

	AdjustWindowRectEx(&pos,style,false,exstyle);

	//if(!fullscreen)
	//	ChangeDisplaySettings(NULL,0);

	SetWindowPos( d3d_hWnd, HWND_TOP, pos.left, pos.top,
					pos.right - pos.left, pos.bottom - pos.top,SWP_SHOWWINDOW);

	//if(!fullscreen)
	//	ChangeDisplaySettings(NULL,0);

	ShowWindow( d3d_hWnd, SW_SHOW );
	UpdateWindow( d3d_hWnd );

	d3d_need_reset = true;

	if(!fullscreen && set_resolution_first)
	{
		PumpMessages();
		set_resolution_first = false;
		SetResolution(width,height,fullscreen);
		set_resolution_first = true;
	}

	set_resolution_first = true;

	POINT pt = { width/2, height/2 };
	ClientToScreen(d3d_hWnd,&pt);
	SetCursorPos(pt.x,pt.y);
}

void Device::SetTopmost(bool topmost)
{
	config.SetInt( "Topmost", topmost );

	if( d3d_hWnd && d3d_windowed )
	{
		DWORD exstyle = GetWindowLong( d3d_hWnd, GWL_EXSTYLE );
		
		if(topmost)	exstyle |= WS_EX_TOPMOST;
		else		exstyle &= ~WS_EX_TOPMOST;

		SetWindowLong( d3d_hWnd, GWL_EXSTYLE, exstyle );

		if( topmost )	SetWindowPos(d3d_hWnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		else			SetWindowPos(d3d_hWnd,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	}
}


void Device::SetAppName(const char *name)
{
	d3d_app_name = name;
	if(d3d_hWnd)
		SetWindowText(d3d_hWnd,d3d_app_name.c_str());
}


void Device::SetMinZStencilSize(int width,int height)
{
	int _w = max(d3d_screen_w,d3d_min_zstencil_w);
	int _h = max(d3d_screen_h,d3d_min_zstencil_h);
	if(width>_w || height>_h)
		d3d_need_reset = true;
	d3d_min_zstencil_w = max(width,d3d_min_zstencil_w);
	d3d_min_zstencil_h = max(height,d3d_min_zstencil_h);
}

bool Device::PumpMessages()
{
	MSG msg;

	if(quited)
		return false;

	while( PeekMessage(&msg,NULL,0,0,PM_REMOVE) )
	{
		if(msg.message==WM_QUIT)
		{
			quited = true;
			return false;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

bool Device::Init(bool soft_vp)
{
	const char *app_name = d3d_app_name.c_str();
	const char *class_name = "DXFW Window Class";

	// Init app name
	d3d_app_name = app_name;

	// Use config
	d3d_windowed = ( config.GetInt( "Windowed", d3d_windowed?1:0 ) != 0 );
	d3d_screen_w = config.GetInt( "ScreenWidth", d3d_screen_w );
	d3d_screen_h = config.GetInt( "ScreenHeight", d3d_screen_h );
	d3d_msaa = config.GetInt( "MSAA", d3d_msaa );

	// Register window class
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, _DeviceMsgProc, 0L, 0L, 
						GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
						class_name, NULL };
	d3d_wc = wc;
	RegisterClassEx( &d3d_wc );

    // Create window
	int x0=0, y0=0, w=0, h=0;
    d3d_hWnd = CreateWindowEx(	d3d_windowed ?	(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE) :
												(WS_EX_APPWINDOW /*| WS_EX_TOPMOST*/),	// TODO: check this
								class_name, app_name,
								d3d_windowed ? WS_OVERLAPPEDWINDOW : WS_POPUP,
								0, 0, d3d_screen_w, d3d_screen_h,
								NULL, NULL, d3d_wc.hInstance, NULL );

	SetWindowLongPtr(d3d_hWnd,GWL_USERDATA,(LONG_PTR)this);
	has_focus = true;

	// position and init window
	SetResolution(d3d_screen_w,d3d_screen_h,!d3d_windowed,d3d_msaa);

	// Create Direct3D
	if( NULL == ( d3d = Direct3DCreate9( D3D_SDK_VERSION ) ) )
	{
		MessageBox(NULL,"Can't initialize Direct3D!","Error!",MB_OK);
		Shutdown();
		return false;
	}

	// Create device
	DWORD flags = 0;
	if(soft_vp)	flags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	else		flags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
	if( FAILED( d3d->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3d_hWnd,
						flags, GetPresentParameters(), &dev ) ) )
	{
		MessageBox(NULL,"Can't initialize Direct3D!","Error!",MB_OK);
		Shutdown();
		return false;
	}

	// Set device state
	dev->SetRenderState(D3DRS_LIGHTING,FALSE);

	ShowWindow( d3d_hWnd, SW_SHOW );
	UpdateWindow( d3d_hWnd );

	// Create z/stencil & locking surfaces
	CreateInternalResources();

	PreloadResources();

    // Init raw input
    RAWINPUTDEVICE device;
    device.usUsagePage = 0x01;
    device.usUsage = 0x02;
    device.dwFlags = 0;
    device.hwndTarget = 0;
    RegisterRawInputDevices(&device, 1, sizeof(device));

	return true;
}

void Device::Shutdown()
{
	ReleaseInternalResources();

	if(dev) dev->Release();		dev = NULL;
	if(d3d) d3d->Release();		d3d = NULL;
	DestroyWindow(d3d_hWnd);	d3d_hWnd = NULL;

	UnregisterClass( "DXFW Window Class", d3d_wc.hInstance );
}



bool Device::MainLoop()
{
	if(!d3d_hWnd)
	{
		if(!Dev.Init())
			return false;
	}
	else
		Dev.EndScene();

	while(1)
	{
		if(!Dev.PumpMessages())
		{
			Dev.Shutdown();
			return false;
		}

		if(Dev.BeginScene())
			break;

		Sleep(300);
	}

	return true;
}


void Device::SetConfig(const char *path)
{
	config.Load(path,true);
}


void Device::SetDefaultRenderStates()
{
	// vertex processing
	dev->SetRenderState(D3DRS_LIGHTING,FALSE);
	dev->SetVertexShader(NULL);

	// z/stencil
	dev->SetRenderState(D3DRS_ZENABLE,TRUE);
	dev->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
	dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
	dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
	dev->SetRenderState(D3DRS_STENCILENABLE,FALSE);

	// pixel processing
	dev->SetPixelShader(NULL);

	dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
	dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
	dev->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
	dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
	dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
	dev->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
	dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
	
	dev->SetTexture(0,NULL);

	// framebuffer
	dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
	dev->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);

	dev->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);

	dev->SetRenderState(D3DRS_COLORWRITEENABLE,0xF);
}

void Device::SetRState(int mode)
{
    Dev->SetRenderState(D3DRS_ZENABLE,(mode & RSF_NO_ZENABLE)==0);
    Dev->SetRenderState(D3DRS_ZWRITEENABLE,(mode & RSF_NO_ZWRITE)==0);
    Dev->SetRenderState(D3DRS_CULLMODE,((mode>>2)&3)+1);
    Dev->SetRenderState(D3DRS_ALPHATESTENABLE,(mode&0x00FF0000)!=0);
    if(mode&0x00FF0000)
    {
        Dev->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATEREQUAL);
        Dev->SetRenderState(D3DRS_ALPHAREF,(mode>>16)&0xFF);
    }
    Dev->SetRenderState(D3DRS_ALPHABLENDENABLE,(mode&0xFF000070)!=0);
    if(mode&0xFF000070)
    {
        Dev->SetRenderState(D3DRS_SRCBLEND,((mode>>24)&0xF)+1);
        Dev->SetRenderState(D3DRS_DESTBLEND,((mode>>28)&0xF)+1);
        Dev->SetRenderState(D3DRS_BLENDOP,(mode>>4)&7);
    }
    Dev->SetRenderState(D3DRS_COLORWRITEENABLE,(~(mode>>8))&15);
}

void Device::SetSampler(int id,int flags)
{
	static const int minmag[5] = { D3DTEXF_POINT,	D3DTEXF_LINEAR,	D3DTEXF_LINEAR, D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC };
	static const int mip[5]    = { D3DTEXF_NONE,	D3DTEXF_NONE,	D3DTEXF_POINT,	D3DTEXF_LINEAR,	D3DTEXF_LINEAR };
    int texf = flags & 0x1F;
    int texa = (flags >> 5)&0x7;
    if(!texa) texa = D3DTADDRESS_WRAP;

	int t = (texf<=4) ? texf : 4;
	dev->SetSamplerState(id,D3DSAMP_MAGFILTER,minmag[t]);
	dev->SetSamplerState(id,D3DSAMP_MINFILTER,minmag[t]);
	dev->SetSamplerState(id,D3DSAMP_MIPFILTER,mip[t]);
	if(t>=4) dev->SetSamplerState(id,D3DSAMP_MAXANISOTROPY,t-2);
	dev->SetSamplerState(id,D3DSAMP_ADDRESSU,texa);
	dev->SetSamplerState(id,D3DSAMP_ADDRESSV,texa);
	dev->SetSamplerState(id,D3DSAMP_ADDRESSW,texa);
    if(texa==TEXA_BORDER) dev->SetSamplerState(id,D3DSAMP_BORDERCOLOR,(flags&TEXB_WHITE) ? 0xFFFFFFFF : 0x00000000);
}

void Device::SetRenderTarget(int id,DevRenderTarget *rt)
{
	if(id==0 && custom_zstencil_rt)
		dev->SetDepthStencilSurface( rt ? custom_zstencil_rt : custom_zstencil );

	if(!rt)
	{
		if(id==0)
		{
			IDirect3DSurface9 *surf = NULL;
			dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&surf);
			if(surf)
			{
				dev->SetRenderTarget(0,surf);
				surf->Release();
			}
		}
		else
			dev->SetRenderTarget(id,NULL);
		return;
	}

	IDirect3DSurface9 *surf = rt->GetSurface();
	if(!surf) return;
	dev->SetRenderTarget(id,surf);
	surf->Release();
}


void Device::SetCubeRenderTarget(int id,DevCubeRenderTarget *rt,int face)
{
	SetCubeRenderTargetLevel(id,rt,face,0);
}

void Device::SetCubeRenderTargetLevel(int id,DevCubeRenderTarget *rt,int face,int level)
{
	if(id==0 && custom_zstencil_rt)
		dev->SetDepthStencilSurface( rt ? custom_zstencil_rt : custom_zstencil );

	if(!rt)
	{
		SetRenderTarget(id,NULL);
		return;
	}

	IDirect3DSurface9 *surf = rt->GetSurfaceLevel(face,level);
	if(!surf) return;
	dev->SetRenderTarget(id,surf);
	surf->Release();
}


void Device::SetDepthStencil(DevDepthStencilSurface *ds)
{
	if(!ds)
	{
		dev->SetDepthStencilSurface(custom_zstencil);
		return;
	}

	dev->SetDepthStencilSurface(ds->GetSurface());
}

void Device::StretchRect(DevRenderTarget *src,const RECT *srect,
						 DevRenderTarget *dst,const RECT *drect,
						 D3DTEXTUREFILTERTYPE filter)
{
	IDirect3DSurface9 *ssurf = NULL;
	IDirect3DSurface9 *dsurf = NULL;
	
	if(src)	ssurf = src->GetSurface();
	else	dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&ssurf);

	if(dst)	dsurf = dst->GetSurface();
	else	dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&dsurf);

	if(ssurf && dsurf)
	{
		dev->StretchRect(ssurf,srect,dsurf,drect,filter);
	}

	if(ssurf) ssurf->Release();
	if(dsurf) dsurf->Release();
}

void Device::DrawScreenQuad(float x1,float y1,float x2,float y2,float z)
{
	struct Vertex {
		float x, y, z, w;
		float u, v;
	};

	Vertex vtx[4];
	for(int i=0;i<4;i++)
	{
		vtx[i].x = ((i==0 || i==3) ? x1 : x2) - 0.5f;
		vtx[i].y = ((i==0 || i==1) ? y1 : y2) - 0.5f;
		vtx[i].z = z;
		vtx[i].w = 1;
		vtx[i].u = ((i==0 || i==3) ? 0.f : 1.f);
		vtx[i].v = ((i==0 || i==1) ? 0.f : 1.f);
	}

	dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,vtx,sizeof(Vertex));
}

void Device::DrawScreenQuadTC(float x1,float y1,float x2,float y2,float *coord_rects,int n_coords,float z)
{
	struct Vertex {
		float x, y, z, w;
		vec2 uv[8];
	};

	Vertex vtx[4];
	for(int i=0;i<4;i++)
	{
		vtx[i].x = ((i==0 || i==3) ? x1 : x2) - 0.5f;
		vtx[i].y = ((i==0 || i==1) ? y1 : y2) - 0.5f;
		vtx[i].z = z;
		vtx[i].w = 1;
		for(int j=0;j<n_coords;j++)
		{
			vtx[i].uv[j].x = ((i==0 || i==3) ? coord_rects[j*4+0] : coord_rects[j*4+2]);
			vtx[i].uv[j].y = ((i==0 || i==1) ? coord_rects[j*4+1] : coord_rects[j*4+3]);
		}

	}

	dev->SetFVF(D3DFVF_XYZRHW|(D3DFVF_TEX1*n_coords));
	dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,vtx,sizeof(Vertex));
}

void Device::DrawScreenQuadTCI(float x1,float y1,float x2,float y2,float *tc00,float *tc10,float *tc01,float *tc11,
								int n_interp,int tc_fvf,float z)
{
	struct Vertex {
		float x, y, z, w;
		float uv[8*4];
	};

	Vertex vtx[4];
	float dd = 0;

	if(!(tc_fvf & D3DFVF_POSITION_MASK))
	{
		tc_fvf |= D3DFVF_XYZRHW;
		dd = 0.5f;
	}

	for(int i=0;i<4;i++)
	{
		vtx[i].x = ((i==0 || i==3) ? x1 : x2) - dd;
		vtx[i].y = ((i==0 || i==1) ? y1 : y2) - dd;
		vtx[i].z = z;
		vtx[i].w = 1;
	}
	memcpy(vtx[0].uv,tc00,sizeof(float)*n_interp);
	memcpy(vtx[1].uv,tc10,sizeof(float)*n_interp);
	memcpy(vtx[2].uv,tc11,sizeof(float)*n_interp);
	memcpy(vtx[3].uv,tc01,sizeof(float)*n_interp);

	dev->SetFVF(tc_fvf);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,vtx,sizeof(Vertex));
}

void Device::DrawScreenQuadTCIR(float x1,float y1,float x2,float y2,float *rects,int n_interp,int tc_fvf,float z)
{
	struct Vertex {
		float x, y, z, w;
		float uv[8*4];
	};

	Vertex vtx[4];
	float dd = 0;

	if(!(tc_fvf & D3DFVF_POSITION_MASK))
	{
		tc_fvf |= D3DFVF_XYZRHW;
		dd = 0.5f;
	}

	for(int i=0;i<4;i++)
	{
		vtx[i].x = ((i==0 || i==3) ? x1 : x2) - dd;
		vtx[i].y = ((i==0 || i==1) ? y1 : y2) - dd;
		vtx[i].z = z;
		vtx[i].w = 1;
		for(int j=0;j<n_interp;j++)
			vtx[i].uv[j] = rects[j*4+(i^(i>>1))];
	}

	dev->SetFVF(tc_fvf);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,vtx,sizeof(Vertex));
}

void Device::DrawScreenQuadVS(float x1,float y1,float x2,float y2,float z)
{
	struct Vertex {
		float x, y, z;
		float u, v;
	};

	Vertex vtx[4];
	for(int i=0;i<4;i++)
	{
		vtx[i].x = ((i==0 || i==3) ? x1 : x2);
		vtx[i].y = ((i==0 || i==1) ? y1 : y2);
		vtx[i].z = z;
		vtx[i].u = ((i==0 || i==3) ? 0.f : 1.f);
		vtx[i].v = ((i==0 || i==1) ? 0.f : 1.f);
	}

	dev->SetFVF(D3DFVF_XYZ|D3DFVF_TEX1);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,vtx,sizeof(Vertex));
}

void Device::DrawScreenQuad(const vec2 &p1,const vec2 &p2,float z)
{
	DrawScreenQuad(p1.x,p1.y,p2.x,p2.y,z);
}

void Device::DrawScreenQuadTC(const vec2 &p1,const vec2 &p2,const vec2 &tc1,const vec2 &tc2,float z)
{
	float cr[4] = { tc1.x, tc1.y, tc2.x, tc2.y };
	DrawScreenQuadTC(p1.x,p1.y,p2.x,p2.y,cr,1,z);
}


void Device::DrawSkyBoxQuad(const vec3 &ypr,float fov)
{
	vec3 vc, vx, vy;
	Dev.BuildCameraVectors(ypr,&vc,&vx,&vy);
	vc *= 1.f/tanf(fov*(0.5f*D3DX_PI/180.f));
	vy = -vy;
	vx *= Dev.GetAspectRatio();

	int sw, sh;
	Dev.GetScreenSize(sw,sh);

	vec3 TC[4] = {
		vc-vx-vy, vc+vx-vy,
		vc-vx+vy, vc+vx+vy,
	};

	Dev.DrawScreenQuadTCI(0,0,float(sw),float(sh),&TC[0].x,&TC[1].x,&TC[2].x,&TC[3].x,3,
							D3DFVF_TEX1|D3DFVF_TEXCOORDSIZE3(0),1.f - 1.f/(1<<16));
}


void Device::Print(DevFont *fnt,int xp,int yp,int color,const char *text)
{
	RECT rect = { xp, yp, xp+4096, yp+4096 };
	if(!fnt) fnt = &default_font;
	fnt->GetFont()->DrawText(NULL,text,-1,&rect,DT_TOP | DT_LEFT,color);
}

void Device::PrintF(DevFont *fnt,int xp,int yp,int color,const char *fmt, ...)
{
	va_list arg;
	string tmp;
	va_start(arg,fmt);
	vsprintf(tmp,fmt,arg);
	va_end(arg);

	Print(fnt,xp,yp,color,tmp.c_str());
}

void Device::AlignPrint(DevFont *fnt,int xp,int yp,int align,int color,const char *text)
{
	int ax = (align>>4)&3;
	int ay = align&3;
	DWORD fl = DT_SINGLELINE;
	RECT rect = { xp, yp, xp, yp };
	if(ax>=1) rect.left		-= 2048;
	if(ax<=1) rect.right	+= 2048;
	if(ay>=1) rect.top		-= 2048;
	if(ay<=1) rect.bottom	+= 2048;
	if(ax==0) fl |= DT_LEFT;
	if(ax==1) fl |= DT_CENTER;
	if(ax==2) fl |= DT_RIGHT;
	if(ay==0) fl |= DT_TOP;
	if(ay==1) fl |= DT_VCENTER;
	if(ay==2) fl |= DT_BOTTOM;

	if(!fnt) fnt = &default_font;
	fnt->GetFont()->DrawText(NULL,text,-1,&rect,fl,color);
}

void Device::AlignPrintF(DevFont *fnt,int xp,int yp,int color,int align,const char *fmt, ...)
{
	va_list arg;
	string tmp;
	va_start(arg,fmt);
	vsprintf(tmp,fmt,arg);
	va_end(arg);

	AlignPrint(fnt,xp,yp,color,align,tmp.c_str());
}


void Device::DPrintF(const char *fmt, ...)
{
	va_list arg;
	string tmp;
	va_start(arg,fmt);
	vsprintf(tmp,fmt,arg);
	va_end(arg);

	dprintf_buff.push_back(tmp);
}

int *Device::GetQuadsIndices(int n_quads)
{
	if((int)quad_idx.size()<n_quads*6)
	{
		static int IDX[6] = {0,1,3,3,1,2};
		int p = quad_idx.size();
		quad_idx.resize(n_quads*6);
		while(p<(int)quad_idx.size())
		{
			quad_idx[p] = (p/6)*4 + IDX[p%6];
			p++;
		}
	}
	return quad_idx.size() ? &quad_idx[0] : NULL;
}


void Device::SetMouseCapture(bool capture)
{
    if(capture && !mouse_capture)
    {
		SetCursor(NULL);
		POINT pt = { d3d_screen_w/2, d3d_screen_h/2 };
		ClientToScreen(d3d_hWnd,&pt);
		SetCursorPos(pt.x,pt.y);
    }

    mouse_capture = capture;
}

bool Device::GetKeyStroke(int vk)
{
    for(int i=0;i<(int)read_keys.size();i++)
        if(read_keys[i]==vk)
        {
            read_keys.erase(read_keys.begin()+i);
            return true;
        }

    return false;
}



void Device::RegisterMessageHandler(IDevMsgHandler *dmh)
{
	UnregisterMessageHandler(dmh);
	msg_handlers.push_back(dmh);
}

void Device::UnregisterMessageHandler(IDevMsgHandler *dmh)
{
	for(int i=0;i<(int)msg_handlers.size();i++)
		if(msg_handlers[i]==dmh)
		{
			msg_handlers.erase(msg_handlers.begin()+i);
			i--;
		}
}



// ------------


D3DPRESENT_PARAMETERS *Device::GetPresentParameters()
{
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed					= d3d_windowed;
	d3dpp.BackBufferWidth			= d3d_screen_w;
	d3dpp.BackBufferHeight			= d3d_screen_h;
	d3dpp.BackBufferFormat			= D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount			= 1;
	d3dpp.EnableAutoDepthStencil	= FALSE;
	d3dpp.AutoDepthStencilFormat	= D3DFMT_UNKNOWN;
	d3dpp.SwapEffect				= D3DSWAPEFFECT_DISCARD;
	d3dpp.Flags						= 0;
	d3dpp.MultiSampleType			= (D3DMULTISAMPLE_TYPE)d3d_msaa;
	d3dpp.MultiSampleQuality		= 0;
	d3dpp.hDeviceWindow				= d3d_hWnd;
	d3dpp.FullScreen_RefreshRateInHz= 0;
	d3dpp.PresentationInterval		= D3DPRESENT_INTERVAL_ONE;	// D3DPRESENT_INTERVAL_DEFAULT;
	return &d3dpp;
}


LRESULT Device::MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	for(int i=0;i<(int)msg_handlers.size();i++)
	{
		bool pass = true;
		LRESULT res = msg_handlers[i]->MsgProc(this,hWnd,msg,wParam,lParam,pass);
		if(!pass) return res;
	}

	switch( msg )
	{
		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;

		case WM_MOVE:
			// save to config
			config.SetInt( "WindowPositionX", LOWORD(lParam) );
			config.SetInt( "WindowPositionY", HIWORD(lParam) );
			break;

		case WM_SIZE:
			d3d_screen_w = LOWORD(lParam);
			d3d_screen_h = HIWORD(lParam);
			d3d_need_reset = true;

			// save to config
			config.SetInt( "ScreenWidth", d3d_screen_w );
			config.SetInt( "ScreenHeight", d3d_screen_h );
			config.SetInt( "Windowed", d3d_windowed );
			return 0;

		case WM_SIZING:
			{
				RECT *box = (RECT*)lParam;
				RECT wr,cr;
				GetWindowRect(hWnd,&wr);
				GetClientRect(hWnd,&cr);

				// compute frames width
				wr.left -= cr.left;
				wr.right -= cr.right;
				wr.top -= cr.top;
				wr.bottom -= cr.bottom;

				// compute new client rect
				cr = *box;
				cr.left -= wr.left;
				cr.right -= wr.right;
				cr.top -= wr.top;
				cr.bottom -= wr.bottom;

				// compute client rect
				cr.right -= cr.left;
				cr.bottom -= cr.top;
				cr.right = (cr.right+16)&~0x1F;
				cr.bottom = (cr.bottom+16)&~0x1F;
				if(cr.right<64) cr.right = 64;
				if(cr.bottom<64) cr.bottom = 64;
				cr.right += cr.left;
				cr.bottom += cr.top;

				// compute new window rect
				cr.left += wr.left;
				cr.right += wr.right;
				cr.top += wr.top;
				cr.bottom += wr.bottom;
				*box = cr;

				d3d_sizing = true;
				UpdateWindow(hWnd);
			}
		return 1;

		case WM_PAINT:
			if(d3d_sizing)
			{
				HDC hDC = GetDC(hWnd);
				RECT rect;
				rect.top = rect.left = 0;
				rect.bottom = rect.right = 4096;
				FillRect(hDC,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
				ReleaseDC(hWnd,hDC);
				d3d_sizing = false;
				return 0;
			}
		break;

		case WM_MOUSEMOVE:
			{
				if(!mouse_capture)
				{
					mouse_x = LOWORD(lParam);
					mouse_y = HIWORD(lParam);
					SetCursor(LoadCursor(NULL,IDC_ARROW));
				}
				else
				{
					int mx = LOWORD(lParam);
					int my = HIWORD(lParam);

					SetCursor(NULL);
					int dx = mx - d3d_screen_w/2;
					int dy = my - d3d_screen_h/2;
					mouse_dx += dx;
					mouse_dy += dy;
					if(dx || dy)
					{
						POINT pt = { d3d_screen_w/2, d3d_screen_h/2 };
						ClientToScreen(d3d_hWnd,&pt);
						SetCursorPos(pt.x,pt.y);
					}
				}
			}
		return 0;

		case WM_MOUSEWHEEL:
			{
				mouse_dz += short(HIWORD(wParam))/float(WHEEL_DELTA);
			}
		return 0;

		case WM_LBUTTONDOWN:	read_keys.push_back(VK_LBUTTON);	return 0;
		case WM_RBUTTONDOWN:	read_keys.push_back(VK_RBUTTON);	return 0;
		case WM_MBUTTONDOWN:	read_keys.push_back(VK_MBUTTON);	return 0;

		case WM_SYSKEYDOWN:
			if(wParam==VK_F4 && ((GetAsyncKeyState(VK_LMENU)<0) || (GetAsyncKeyState(VK_RMENU)<0)))
				DestroyWindow(hWnd);
			read_keys.push_back(wParam);
		return 0;

		case WM_KEYDOWN:
			read_keys.push_back(wParam);
/*			if(wParam=='1') D3D_SetResolution( 640, 480, true);
			if(wParam=='2') D3D_SetResolution( 800, 600, true);
			if(wParam=='3') D3D_SetResolution(1024, 768, true);
			if(wParam=='4') D3D_SetResolution(1280,1024, true);
			if(wParam=='Q') D3D_SetResolution( 640, 480,false);
			if(wParam=='W') D3D_SetResolution( 800, 600,false);
			if(wParam=='E') D3D_SetResolution(1024, 768,false);
			if(wParam=='R') D3D_SetResolution(1280,1024,false);
*/
		return 0;

		case WM_CHAR:
			read_keys.push_back(~wParam);
		return 0;

		case WM_SYSCHAR:
			read_keys.push_back(wParam | 0x10000);
		return TRUE;

        case WM_INPUT:
        {
            static vector<byte> rawInputMessageData;

            bool inForeground = (GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT);
            HRAWINPUT hRawInput = (HRAWINPUT)lParam;

            UINT dataSize;
            UINT u = GetRawInputData(hRawInput, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

            if( u==0 && dataSize != 0)
            {
                rawInputMessageData.resize(dataSize);
                
                void* dataBuf = &rawInputMessageData[0];

                if( GetRawInputData(hRawInput, RID_INPUT, dataBuf, &dataSize, sizeof(RAWINPUTHEADER)) == dataSize )
                {
                    const RAWINPUT *raw = (const RAWINPUT*)dataBuf;

                    if (raw->header.dwType == RIM_TYPEMOUSE)
                    {
                        const RAWMOUSE& mouseData = raw->data.mouse;
                        HANDLE deviceHandle = raw->header.hDevice;
                        USHORT flags = mouseData.usButtonFlags;
                        short wheelDelta = (short)mouseData.usButtonData;
                        LONG x = mouseData.lLastX, y = mouseData.lLastY;

                        size_t index = 0;
                        while(index < mouse_data.size() && mouse_data[index].handle != deviceHandle)
                            ++index;
                        if(index >= mouse_data.size())
                        {
                            mouse_data.push_back(RawMouse());
                            mouse_data[index].Clear();
                            mouse_data[index].handle = deviceHandle;
                        }

                        RawMouse &data = mouse_data[index];
                        data.xp += x;
                        data.yp += y;
                        data.dx += x;
                        data.dy += y;
                        data.dz += wheelDelta;

                        double tt = Dev.GetElapsedTime();
                        for(int b=0;b<5;b++)
                        {
                            bool press   = (flags & (1<<(b*2)))!=0;
                            bool release = (flags & (2<<(b*2)))!=0;
                            if(press)
                            {
                                data.bt_down  |= 1<<b;
                                data.bt_click |= 1<<b;
                                if(tt - data.press_time[b] <= data.dbclick_timeout)
                                    data.bt_2click |= 1<<b;
                                data.press_time[b] = tt;
                            }
                            if(release)
                            {
                                data.bt_down &= ~(1<<b);
                            }
                        }
                    }
                }
            }
        }
        break;
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}

void Device::CreateInternalResources()
{
	ReleaseInternalResources();

	for(int i=0;i<2;i++)
		dev->CreateOffscreenPlainSurface(16,16,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&block_surface[i],0);

	dev->CreateDepthStencilSurface(
		max(d3d_screen_w,d3d_min_zstencil_w),max(d3d_screen_h,d3d_min_zstencil_h),
		D3DFMT_D24S8, (D3DMULTISAMPLE_TYPE)d3d_msaa, 0, FALSE, &custom_zstencil, NULL );

	if(d3d_msaa)
	{
		dev->CreateDepthStencilSurface(
			max(d3d_screen_w,d3d_min_zstencil_w),max(d3d_screen_h,d3d_min_zstencil_h),
			D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, &custom_zstencil_rt, NULL );
	}

	dev->SetDepthStencilSurface(custom_zstencil);

//	if(!custom_zstencil)
//		MessageBox(NULL,format("Failed to create primary z/stencil surface (code %08x)",res).c_str(),"Error!",MB_OK);
}

void Device::ReleaseInternalResources()
{
	for(int i=0;i<2;i++)
		if(block_surface[i])
		{
			block_surface[i]->Release();
			block_surface[i] = NULL;
		}

	if(custom_zstencil)
	{
		dev->SetDepthStencilSurface(NULL);
		custom_zstencil->Release();
		custom_zstencil = NULL;
	}

	if(custom_zstencil_rt)
	{
		dev->SetDepthStencilSurface(NULL);
		custom_zstencil_rt->Release();
		custom_zstencil_rt = NULL;
	}

}



// -------- helper functions --------


void Device::BuildProjectionMatrix(D3DXMATRIX *out,float vport[4],int rt_size_x,int rt_size_y,
								   const vec3 &ax,const vec3 &ay,const vec3 &az,float zn,float zf)
{
	static float DEFAULT_VPORT[4] = { -1, -1, 1, 1 };

	if(!vport)
		vport = DEFAULT_VPORT;

	float _vp[4] = { vport[0], vport[1], vport[2], vport[3] };
	float s = 1.0f;
	_vp[0] -= s/rt_size_x;
	_vp[1] += s/rt_size_y;
	_vp[2] -= s/rt_size_x;
	_vp[3] += s/rt_size_y;

	float sx = 2/(_vp[2] - _vp[0]);
	float sy = 2/(_vp[3] - _vp[1]);
	float dx = -(_vp[2] + _vp[0])/(_vp[2] - _vp[0]);
	float dy = -(_vp[3] + _vp[1])/(_vp[3] - _vp[1]);

	out->_11 = sx*ax.x - dx*az.x;
	out->_21 = sx*ax.y - dx*az.y;
	out->_31 = sx*ax.z - dx*az.z;
	out->_41 = 0.f;

	out->_12 = sy*ay.x - dy*az.x;
	out->_22 = sy*ay.y - dy*az.y;
	out->_32 = sy*ay.z - dy*az.z;
	out->_42 = 0.f;

	float Q = zf/(zf-zn);
	if(zf<=0)
		Q = (zf<0) ? (1 - 1.f/(1<<16)) : (1 + 1.f/(1<<16));
	out->_13 = Q*az.x;
	out->_23 = Q*az.y;
	out->_33 = Q*az.z;
	out->_43 = -Q*zn;

	out->_14 = az.x;
	out->_24 = az.y;
	out->_34 = az.z;
	out->_44 = 0.f;
}

void Device::BuildCubemapProjectionMatrix(D3DXMATRIX *mtx,int face,int size,float zn,float zf,float *vport)
{
	BuildProjectionMatrix(mtx,vport,size,size,
		*(vec3*)CUBE_AXIS[face][0],
		*(vec3*)CUBE_AXIS[face][1],
		*(vec3*)CUBE_AXIS[face][2],
		zn,zf);
}

void Device::BuildCameraViewProjMatrix(D3DXMATRIX *mtx,const vec3 &pos,const vec3 &_ypr,float fov,
									float aspect,float zn,float zf,bool m_view,bool m_proj)
{
	if(!m_view && !m_proj)
	{
		D3DXMatrixIdentity(mtx);
		return;
	}

	D3DXMATRIX view, proj;
	D3DXMATRIX *p = &proj;

	if(m_view)
	{
		D3DXVECTOR3 up(0,0,1), target;

		vec3 ypr = _ypr*(D3DX_PI/180);
		target.x = cosf(ypr.x)*cosf(ypr.y);
		target.y = sinf(ypr.x)*cosf(ypr.y);
		target.z = sinf(ypr.y);
		target += *(D3DXVECTOR3*)&pos;

		if(!m_proj)
		{
			D3DXMatrixLookAtLH(mtx,(D3DXVECTOR3*)&pos,(D3DXVECTOR3*)&target,&up);
			return;
		}

		D3DXMatrixLookAtLH(&view,(D3DXVECTOR3*)&pos,(D3DXVECTOR3*)&target,&up);
	}
	else
		p = mtx;

	if(aspect<=0)
		aspect = GetAspectRatio();
	D3DXMatrixPerspectiveFovLH(p,fov*(D3DX_PI/180.f),aspect,zn,zf);
	
	if(zf<=0)
	{
		float Q = (zf<0) ? (1 - 1.f/(1<<16)) : (1 + 1.f/(1<<16));
		p->_33 = Q;
		p->_43 = -Q*zn;
	}

	if(m_view && m_proj)
		D3DXMatrixMultiply(mtx,&view,&proj);
}

void Device::BuildCameraVectors(const vec3 &ypr,vec3 *front,vec3 *right,vec3 *up)
{
	float ax = ypr.x*(D3DX_PI/180);
	float ay = ypr.y*(D3DX_PI/180);
	if(front)
	{
		front->x = cosf(ax)*cosf(ay);
		front->y = sinf(ax)*cosf(ay);
		front->z = sinf(ay);
	}
	if(right)
	{
		right->x = -sinf(ax);
		right->y =  cosf(ax);
		right->z =  0;
	}
	if(up)
	{
		up->x = -cosf(ax)*sinf(ay);
		up->y = -sinf(ax)*sinf(ay);
		up->z =  cosf(ay);
	}
}


void Device::TickFPSCamera(vec3 &pos,vec3 &ypr,float move_speed,float sens,bool flat)
{
	vec3 d(0,0,0);
	move_speed *= GetTimeDelta();
	if(GetKeyState('W')) d.x += move_speed;
	if(GetKeyState('S')) d.x -= move_speed;
	if(GetKeyState('A')) d.y -= move_speed;
	if(GetKeyState('D')) d.y += move_speed;
	if(GetKeyState('Q')) d.z -= move_speed;
	if(GetKeyState('E')) d.z += move_speed;

	float ax = ypr.x*(D3DX_PI/180);
	if(flat)
	{
		pos.x += cosf(ax)*d.x - sinf(ax)*d.y;
		pos.y += cosf(ax)*d.y + sinf(ax)*d.x;
		pos.z += d.z;
	}
	else
	{
		vec3 front, right;
		BuildCameraVectors(ypr,&front,&right,NULL);
		pos += front*d.x + right*d.y;
		pos.z += d.z;
	}

	int mx, my;
	SetMouseCapture(true);
	GetMouseDelta(mx,my);
	ypr.x += mx*sens;
	ypr.y -= my*sens;

	if(ypr.y<-89.9f) ypr.y= -89.9f;
	if(ypr.y> 89.9f) ypr.y=  89.9f;

}

void Device::RunDebugCamera(float speed,float sens,float zn,float fov,bool flat)
{
	TickFPSCamera(debug_cam_pos,debug_cam_ypr,speed,sens,flat);

	D3DXMATRIX vp;
	
	BuildCameraViewProjMatrix(&vp,debug_cam_pos,debug_cam_ypr,fov,0,zn,-1,true,false);
	dev->SetTransform(D3DTS_VIEW,&vp);

	BuildCameraViewProjMatrix(&vp,debug_cam_pos,debug_cam_ypr,fov,0,zn,-1,false,true);
	dev->SetTransform(D3DTS_PROJECTION,&vp);
}

void Device::LoadCameraFromConfig()
{
	debug_cam_pos.x = config.GetFloat( "CameraPosX",	debug_cam_pos.x );
	debug_cam_pos.y = config.GetFloat( "CameraPosY",	debug_cam_pos.y	);
	debug_cam_pos.z = config.GetFloat( "CameraPosZ",	debug_cam_pos.z );
	debug_cam_ypr.x = config.GetFloat( "CameraYaw",		debug_cam_ypr.x );
	debug_cam_ypr.y = config.GetFloat( "CameraPitch",	debug_cam_ypr.y );
	debug_cam_ypr.z = config.GetFloat( "CameraRoll",	debug_cam_ypr.z );
}

void Device::SaveCameraToConfig()
{
	config.SetFloat( "CameraPosX",	debug_cam_pos.x );
	config.SetFloat( "CameraPosY",	debug_cam_pos.y	);
	config.SetFloat( "CameraPosZ",	debug_cam_pos.z );
	config.SetFloat( "CameraYaw",	debug_cam_ypr.x );
	config.SetFloat( "CameraPitch",	debug_cam_ypr.y );
	config.SetFloat( "CameraRoll",	debug_cam_ypr.z );
}
