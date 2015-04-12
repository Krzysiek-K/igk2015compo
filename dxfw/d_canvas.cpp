
#include "dxfw.h"

using namespace std;
using namespace base;


//
// Microshader data:
//  s[0-3]      sampler     - texture samplers
//  c           float4      - input color param
//  t           float4      - color from texture
//  o           float4      - output color (default: t*c)
//  uv          float2      - texcoord
//  wpos        float2      - virtual (world) position
//  vpos        float2      - screen position in pixels
//  spos        float2      - normalized screen position (for full screen rendertarget sampling)
//
// Microshader functions:
//  mask        - true if given color is close to color (value is 0..1 or 1..255)
//  killmask    - as above, but kills pixel if true
//


static const char *MICRO_PREFIX =
"sampler s0 : register(s0);\n"
"sampler s1 : register(s1);\n"
"sampler s2 : register(s2);\n"
"sampler s3 : register(s3);\n"
"float4 v2s : register(c30);\n"
"float2 ssize : register(c31);\n"
"float fix(float x) { return x<=1 ? x : x/255; }\n"
"bool mask(float4 x,float r,float g,float b,float e) { return dot(abs(x.xyz-float3(fix(r),fix(g),fix(b))),1)<fix(e); }\n"
"void killmask(float4 x,float r,float g,float b,float e) { if(mask(x,r,g,b,e)) discard; }\n"
"float4 PS(float4 c : COLOR0, float2 uv : TEXCOORD0, float2 wpos : TEXCOORD1) : COLOR0\n"
"{ float4 t=tex2D(s0,uv), o=t*c;\n"
"  float2 vpos = wpos*v2s.wz+v2s.xy;\n"
"  float2 spos = vpos/ssize;\n";

static const char *MICRO_SUFFIX =
"\nreturn o;\n"
"}\n";


static const char *MICRO_VS =
"float4 v2s : register(c30);\n"
"float2 ssize : register(c31);\n"
"void VS( float4 pos : POSITION, float4 col : COLOR, float2 uv : TEXCOORD0,\n"
"         out float4 hpos : POSITION, out float4 _col : COLOR0, out float2 _uv : TEXCOORD0, out float2 _wpos : TEXCOORD1)\n"
"{hpos.xy=(pos.xy*v2s.wz+v2s.xy)/ssize*2-1;\n"
" hpos.y*=-1;\n"
" hpos.zw=float2(.5,1);\n"
" _col=col; _uv=uv; _wpos=pos.xy;\n"
"}\n";




// ---------------- DevTileSet ----------------


void DevTileSet::InitTileNames()
{
	tiles.clear();

	for(int y=0;y<tile_div_y;y++)
		for(int x=0;x<tile_div_x;x++)
		{
			TileInfo ti;
			ti.uvmin.x = float(x  )/tile_div_x;
			ti.uvmin.y = float(y  )/tile_div_y;
			ti.uvmax.x = float(x+1)/tile_div_x;
			ti.uvmax.y = float(y+1)/tile_div_y;
			tiles.push_back(ti);
		}
}

void DevTileSet::SetTileNames(const char *tn)
{
	int tx = 0, ty = 0;
	while(*tn)
	{
		int c = int(*tn)&0xFF;

		if(*tn=='\n') tx=0, ty++;
		if(*tn=='\r') tx=0, ty=0;

		if(c>=' ')
		{
			if(c>=(int)tiles.size())
				tiles.resize(c+1);
			
			TileInfo &ti = tiles[c];
			ti.uvmin.x = float(tx  )/tile_div_x;
			ti.uvmin.y = float(ty  )/tile_div_y;
			ti.uvmax.x = float(tx+1)/tile_div_x;
			ti.uvmax.y = float(ty+1)/tile_div_y;
			tx++;
		}

		tn++;
	}
}

template<class T>
void DevTileSet::DrawInternal(DevCanvas &c,int layer,const base::vec2 &pos,float tsize,const T *tp,int stride,int end_value,int newline_value,int max_w,int max_h)
{
    int xp=0, yp=0;

    while(1)
    {
        if(xp==max_w) xp=0, yp++;
        if(yp==max_h) break;

        int v = *tp;
        if(v==end_value) break;
        
        if(v==newline_value)
        {
            xp=0, yp++;
            if(yp==max_h) break;
        }
        else
        {
            if(v>=0 && v<(int)tiles.size())
                c.Draw(layer,*this,v)()()(pos+vec2(xp+.5f,yp+.5f)*tsize,tsize*.5f);
            xp++;
        }

        *(byte**)&tp += stride;
    }
}

void DevTileSet::Draw(DevCanvas &c,int layer,const base::vec2 &pos,float tsize,const char *text)
{
    DrawInternal(c,layer,pos,tsize,text,sizeof(char),0,'\n',-1,-1);
}

void DevTileSet::Draw(DevCanvas &c,int layer,const base::vec2 &pos,float tsize,const int *tp,int w,int h)
{
    DrawInternal(c,layer,pos,tsize,tp,sizeof(int),0x80000000,0x80000000,w,h);
}

void DevTileSet::Draw(DevCanvas &c,int layer,const base::vec2 &pos,float tsize,const int *tp,int stride,int w,int h)
{
    DrawInternal(c,layer,pos,tsize,tp,stride,0x80000000,0x80000000,w,h);
}



// ---------------- CoordSpace ----------------


void CoordSpace::SetSpace(int type,const vec2 &bmin,const vec2 &bmax,int align)
{
	fit_mode = type;
	view_size = bmax - bmin;
	center = (bmin+bmax)*.5f;
	zoom = 1;
	view_align.x =  align/16 /2.f;
	view_align.y = (align%16)/2.f;

	Update();
}

void CoordSpace::Update()
{
	vec4 w2u, u2v, v2s, s2hw;
	vec2 ss = screen_size.x>0 ? screen_size : Dev.GetScreenSizeV();

	float z = .5f/zoom;
	w2u.make_map_to_unit( center - view_size*z, center + view_size*z );
	u2v.make_map_unit_to( vec2(0,0), view_size );
	if(fit_mode==T_FIT)
	{
		vec4 tmp;
		tmp.make_map_box_scale_fit( ss, view_size, view_align );
		v2s.make_map_inverse(tmp);
	}
	else
		v2s.make_map_box_scale_fit( view_size, ss, view_align );
	s2hw.make_map_to_view( vec2(0,0), ss );

	map_world2screen = w2u;
	map_world2screen.make_map_concat(map_world2screen,u2v);
	map_world2screen.make_map_concat(map_world2screen,v2s);
	map_world2ogl = map_world2screen;
	
	map_screen2world.make_map_inverse(map_world2screen);
}


// ---------------- DevCanvas ----------------

void DevCanvas::Init()
{
	active_layer = NULL;
	active_color = 0xFFFFFFFF;
    micro_vs = NULL;
}


void DevCanvas::SetLayers(int first_id,const CanvasLayerDesc *descs)
{
    for(int i=0;descs[i].shader;i++)
    {
        int id = first_id + i;

        if(id>=int(layers.size()))
            layers.resize(id+1);

        const CanvasLayerDesc &d = descs[i];
        LayerInfo &li = layers[id];

        li.fx = d.fx;
        li.tech = d.shader;
        li.rstate = d.rstate;
        li.sampler_0 = d.sampler_0;
        li.fn_before = d.fn_before;
        li.fn_after = d.fn_after;

		if(li.microshader)
		{
			li.microshader->Release();
			li.microshader = NULL;
		}
		li.microshader_bin.clear();
	}

    BuildMicroShaders();
}


void DevCanvas::SetView(base::vec2 center,float vsize)
{
	screen_size = Dev.GetScreenSizeV();
	screen_center = screen_size*0.5f - vec2(.5f,.5f);
	v_center = center;
	v_size.y = vsize;
	v_size.x = vsize/screen_size.y*screen_size.x;
}

void DevCanvas::SetScreenBox(base::vec2 bmin,base::vec2 bmax)
{
	screen_size = bmax - bmin;
	screen_center = (bmin+bmax)/2 - vec2(.5f,.5f);
	v_size.x = v_size.y/screen_size.y*screen_size.x;
}

void DevCanvas::SelectLayer(int layer,DevTexture &tex)
{
	BatchKey key;
	key.layer = layer;
	key.tex = &tex;

    if(layer>=int(layers.size()))
        layers.resize(layer+1);
    
    active_layer = &vbs[key];
}

void DevCanvas::Flush()
{
	tVBS::iterator p;
	Dev->SetFVF(Vertex::FVF);
	Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	Dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	Dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	Dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    // shader setup
    vec2 v2s_add = V2S(vec2(0,0));
    vec2 v2s_mul = V2S(vec2(1,1)) - v2s_add;
    vec4 shdata[2] = {
        vec4(v2s_add.x,v2s_add.y,v2s_mul.y,v2s_mul.x),
        vec4(screen_size.x,screen_size.y,0,0)
    };


    Dev->SetVertexShader(micro_vs);
    Dev->SetVertexShaderConstantF(30,&shdata[0].x,2);
    Dev->SetPixelShaderConstantF(30,&shdata[0].x,2);
	
	for(p=vbs.begin();p!=vbs.end();p++)
    {
        LayerInfo &li = layers[p->first.layer];

        if(li.fn_before)
            li.fn_before();

        if(p->second.size()>0 && p->first.layer>=0 && p->first.layer<(int)layers.size())
		{
			DevTexture *tex = p->first.tex;

            Dev.SetRState(li.rstate);
            Dev.SetSampler(0,li.sampler_0);
			Dev->SetTexture(0,tex ? tex->GetTexture() : NULL);

			if(li.fx)
			{
				(*li.fx)->SetTexture("tex",tex ? tex->GetTexture() : NULL);
				li.fx->StartTechnique(li.tech.c_str());

				while(li.fx->StartPass())
				{
					Dev->SetFVF( Vertex::FVF );
					Dev->DrawIndexedPrimitiveUP(
						D3DPT_TRIANGLELIST,0,int(p->second.size()),int(p->second.size())/4*2,
						Dev.GetQuadsIndices(int(p->second.size())/4),D3DFMT_INDEX32,
						&p->second[0],sizeof(Vertex));
				}
            }
			else
			{
                Dev->SetPixelShader(li.microshader);

                Dev->SetFVF( Vertex::FVF );
				Dev->DrawIndexedPrimitiveUP(
					D3DPT_TRIANGLELIST,0,int(p->second.size()),int(p->second.size())/4*2,
					Dev.GetQuadsIndices(int(p->second.size())/4),D3DFMT_INDEX32,
					&p->second[0],sizeof(Vertex));

                Dev->SetPixelShader(NULL);
			}

			p->second.clear();
		}

        if(li.fn_after)
            li.fn_after();
    }

    Dev->SetVertexShader(NULL);
}

void DevCanvas::ClearMicroShaders()
{
    for(int i=0;i<(int)layers.size();i++)
    {
        LayerInfo &l = layers[i];

        if(l.microshader)
        {
            l.microshader->Release();
            l.microshader = NULL;
        }
    }

    if(micro_vs)
    {
        micro_vs->Release();
        micro_vs = NULL;
    }
}

void DevCanvas::BuildMicroShaders()
{
    for(int i=0;i<(int)layers.size();i++)
    {
        LayerInfo &l = layers[i];
        if(l.fx || l.tech.size()<=0) continue;

        if(l.microshader_bin.size()<=0)
        {
            // Try compiling shader bytecode.
            string code = MICRO_PREFIX;
            code += l.tech;
            code += MICRO_SUFFIX;

            ID3DXBuffer *bin=0, *err=0;
            if(FAILED(D3DXCompileShader(code.c_str(),(UINT)code.size(),0,0,"PS","ps_2_0",0,&bin,&err,0)) || !bin || err)
            {
                const char *error = (err ? (const char*)err->GetBufferPointer() : "Unknown error");
                string e = error;
                e += "\n\nwhile compiling microshader:\n";

                const char *s = code.c_str();
                int line = 1;
                while(*s)
                {
                    e += format("[%2d] ",line++);
                    const char *b = s;
                    while(*s && *s!='\n') s++;
                    if(*s=='\n') s++;
                    e.append(b,s);
                }

                if(MessageBox(0,e.c_str(),"Microshader error!",MB_OKCANCEL)==IDCANCEL)
                    ExitProcess(0);

                l.microshader_bin.clear();
                l.microshader_bin.push_back(0xFFFFFFFF);
            }
            else
            {
                int ndwords = bin->GetBufferSize()/4;
                l.microshader_bin.resize(ndwords);
                memcpy(&l.microshader_bin[0],bin->GetBufferPointer(),ndwords*sizeof(DWORD));
            }

            if(bin) bin->Release();
            if(err) err->Release();
        }

        if(Dev.GetIsReady() && !l.microshader && l.microshader_bin.size()>0 && l.microshader_bin[0]!=0xFFFFFFFF)
        {
            HRESULT res = Dev->CreatePixelShader(&l.microshader_bin[0],&l.microshader);

			assert( SUCCEEDED(res) );
        }
    }

    if(!micro_vs)
    {
        // Try compiling shader bytecode.

        ID3DXBuffer *bin=0, *err=0;
        if(FAILED(D3DXCompileShader(MICRO_VS,strlen(MICRO_VS),0,0,"VS","vs_2_0",0,&bin,&err,0)) || !bin || err)
        {
            const char *error = (err ? (const char*)err->GetBufferPointer() : "Unknown error");
            string e = error;
            e += "\n\nwhile compiling micro VS:\n";

            const char *s = MICRO_VS;
            int line = 1;
            while(*s)
            {
                e += format("[%2d] ",line++);
                const char *b = s;
                while(*s && *s!='\n') s++;
                if(*s=='\n') s++;
                e.append(b,s);
            }

            if(MessageBox(0,e.c_str(),"Microshader error!",MB_OKCANCEL)==IDCANCEL)
                ExitProcess(0);
        }
        else
        {
            if(Dev.GetIsReady() && !micro_vs)
            {
                HRESULT res = Dev->CreateVertexShader((DWORD*)bin->GetBufferPointer(),&micro_vs);

                assert( SUCCEEDED(res) );
            }
        }

        if(bin) bin->Release();
        if(err) err->Release();
    }
}

void DevCanvas::ClearAuto()
{
	for(map<string,DevTexture*>::iterator p=auto_tex.begin();p!=auto_tex.end();p++)
		if(p->second)
			delete p->second;
	auto_tex.clear();
}

DevTexture *DevCanvas::GetAuto(const char *name)
{
	map<string,DevTexture*>::iterator p = auto_tex.find(name);
	if(p!=auto_tex.end())
		return p->second;

	// load texture
	DevTexture *tex = new DevTexture();

	if(!tex->Load((auto_prefix+name).c_str()))
	{
		delete tex;
		tex = NULL;
	}

	auto_tex[name] = tex;
	return tex;
}
