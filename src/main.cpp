
#include "common.h"


DevEffect fx("render.fx");

DevMesh mesh;
DevTexture texture("data/texture.png");



void Render()
{
	// clear
	Dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x6080E0, 1, 0);

	// pass data to the shader
	D3DXMATRIX VP;
	Dev.BuildCameraViewProjMatrix(&VP, Dev.debug_cam_pos, Dev.debug_cam_ypr, 70.f, -1, 0.01f, -1.f);

	fx->SetMatrix("ViewProj", &VP);
	fx.SetFloat3("cam_pos", Dev.debug_cam_pos);
	fx->SetTexture("tex", texture);

	// render
	fx.StartTechnique("tech");
	while( fx.StartPass() )
		mesh.DrawSection(0);
}



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pCmdLine, int nShowCmd)
{
	Config cfg("config.cfg");

	Dev.SetResolution(cfg.GetInt("screen_w", 1024), cfg.GetInt("screen_h", 768), false);
	Dev.Init();
	Dev.MainLoop();

	mesh.LoadSphere(1.f, 40, 20);

	Dev.debug_cam_pos.x = -2.f;
	Dev.RunDebugCamera(0);

	while( Dev.MainLoop() )
	{
		if( Dev.GetKeyState(VK_RBUTTON) )
			Dev.RunDebugCamera(5.f);
		else
			Dev.SetMouseCapture(false);

		Render();
	}

	return 0;
}
