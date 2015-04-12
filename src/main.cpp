
#include "common.h"
#include "FmodSound.hpp"


//	DevEffect	*fx;
//	const char	*shader;			// technique name or microshader
//	int			rstate;
//	int			sampler_0;
//  void		(*fn_before)();
//  void		(*fn_after)();
CanvasLayerDesc CLD[] = {
	{0,"if(t.x>.9 && t.y<.1 && t.z>.9) o.w=0;",RSF_BLEND_ALPHA,0,NULL,NULL},
	{0,"if(t.x>.9 && t.y<.1 && t.z>.9) o.w=0;",RSF_BLEND_ALPHA,0,NULL,NULL},
	{0,"if(t.x>.9 && t.y<.1 && t.z>.9) o.w=0;",RSF_BLEND_ALPHA,0,NULL,NULL},
	{0,"if(t.x>.9 && t.y<.1 && t.z>.9) o.w=0;",RSF_BLEND_ALPHA,0,NULL,NULL},
	{}
};


DevCanvas canvas(CLD);



void Render()
{
	// clear
	Dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, vm.GetInt("clear_color"), 1, 0);

	DevFrame();
}



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pCmdLine, int nShowCmd)
{
	Dev.SetConfig("device.cfg");
	Dev.SetResolution(640, 480, false);
	Sound::g_System = new Sound::System();
	Sound::g_System->StartMusic("_distorted_mind_.xm", true);
	Dev.Init();
	Dev.MainLoop();

	Dev.SetTopmost(true);

	NutInit();
	vm.AddFile("api.nut");
	vm.AddFile("test.nut");

	bool last_reset = false;
	while( Dev.MainLoop() )
	{
		// handle VM
		vm.CheckFiles();

		bool reset = (GetAsyncKeyState(VK_F5)<0);
		if( reset && !last_reset)
			vm.Reset();
		last_reset = reset;

		if(Dev.GetKeyState(VK_LBUTTON))	Dev.SetMouseCapture(true);
		if(Dev.GetKeyState(VK_ESCAPE))	Dev.SetMouseCapture(false);
		
		int mice = Dev.GetMiceCount();
		for(int i=0;i<mice;i++)
		{
			RawMouse &m = Dev.GetMiceData(i);
			vm.Run("mouse_update_pos",i,m.dx,m.dy,m.dz);
			vm.Run("mouse_update_buttons",i,m.bt_click,m.bt_down);
		}

		// render
		Render();

		Sound::g_System->Update();
	}

	delete Sound::g_System;
	return 0;
}
