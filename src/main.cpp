
#include "common.h"


//	DevEffect	*fx;
//	const char	*shader;			// technique name or microshader
//	int			rstate;
//	int			sampler_0;
//  void		(*fn_before)();
//  void		(*fn_after)();
CanvasLayerDesc CLD[] = {
	{0,"",0,0,NULL,NULL},
	{}
};


DevCanvas canvas;



void Render()
{
	// clear
	Dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x6080E0, 1, 0);

	DevFrame();
}



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pCmdLine, int nShowCmd)
{
	Dev.SetConfig("device.cfg");
	Dev.SetResolution(640, 480, false);
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

		// render
		Render();
	}

	return 0;
}
