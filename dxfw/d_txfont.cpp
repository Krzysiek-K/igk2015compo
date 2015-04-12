
#include "dxfw.h"

using namespace std;
using namespace base;




static bool ParseKeyValue(const char *&s,string &key,int &value)
{
	ParseWhitespace(s);
	if(!*s) return false;

	string ps;
	ParseString(s,ps);
	const char *p = ps.c_str();
	const char *x = p;
	while(*x && *x!='=') x++;
	if(*x=='=')
	{
		const char *xx = x+1;
		key.assign(p,x);
		value = ParseInt(xx);
		return true;
	}

	return ParseKeyValue(s,key,value);
}


DevTxFont::DevTxFont(const char *face)
{
	path = face;
}


void DevTxFont::Clear()
{
	chars.clear();
	chars.resize(256-32);
	memset(&chars[0],0,sizeof(CharInfo)*chars.size());
	ker.clear();
    height = 0;
}

bool DevTxFont::Load(const char *name)
{
	Clear();
	path = name;

	vector<string> file;
	if(!NFS.GetFileLines(format("%s.fnt",name).c_str(),file))
		return false;

	texture.Load(format("%s_0",name).c_str());
	tex_size = texture.GetSize2D();

	for(int i=0;i<int(file.size());i++)
	{
		const char *s = file[i].c_str();
		string cmd, key;
		ParseString(s,cmd);
		int value;

		if(cmd=="char")
		{
			CharInfo ch;
			memset(&ch,0,sizeof(ch));
			int id = -1;

			while(ParseKeyValue(s,key,value))
			{
				if(key=="id"      ) id    = value;
				if(key=="x"       ) ch.tx = value;
				if(key=="y"       ) ch.ty = value;
				if(key=="width"   ) ch.tw = value;
				if(key=="height"  ) ch.th = value;
				if(key=="xoffset" ) ch.ox = value;
				if(key=="yoffset" ) ch.oy = value;
				if(key=="xadvance") ch.dx = value;
			}

			if(id>=32 && id<=255)
            {
				chars[id-32] = ch;
                if(ch.oy + ch.th > height)
                    height = ch.oy + ch.th;
            }
		}
		else if(cmd=="kerning")
		{
			KerningInfo k;

			while(ParseKeyValue(s,key,value))
			{
				if(key=="first"   ) k.id  |= value << 8;
				if(key=="second"  ) k.id  |= value;
				if(key=="amount"  ) k.adj  = value;
			}

			ker.push_back(k);
		}
	}
	sort(ker.begin(),ker.end());

	return true;
}

int DevTxFont::ComputeLength(const char *s,const char *e)
{
    if(!*s || s==e) return 0;

    int len = 0, last = 0;
    while(*s && s!=e)
    {
		int code = (unsigned char)*s;
        if(code>=32 && code<256)
            len += chars[(last=code)-32].dx;
        s++;
    }
    if(last)
		len += chars[last-32].ox + chars[last-32].tw - chars[last-32].dx;
    return len;
}

void DevTxFont::DrawText(DevCanvas &canvas,int l,float xp,float yp,int align,const base::vec2 &scale,int color,const char *s,const char *e)
{
	canvas.SelectActive(l,texture);
	vector<DevCanvas::Vertex> *_layer = canvas.GetActiveBuffer();
	if(!_layer) return;
	vector<DevCanvas::Vertex> &layer = *_layer;

	xp/=scale.x;
	yp/=scale.y;
    
    if((align&0xF0)==0x10) xp -= (ComputeLength(s,e)+1)/2;
    if((align&0xF0)==0x20) xp -=  ComputeLength(s,e);
    if((align&0x0F)==0x01) yp -= height/2;
    if((align&0x0F)==0x02) yp -= height;

    while(*s && s!=e)
	{
    	int cid = (unsigned char)(*s++);
		if(cid<32 || cid>=256)
			continue;

		int p = cid-32;
		float x1 = (xp+chars[p].ox)*scale.x;
		float y1 = (yp+chars[p].oy)*scale.y;
		float x2 = x1+chars[p].tw*scale.x;
		float y2 = y1+chars[p].th*scale.y;
		float u1 = float(chars[p].tx)/tex_size.x;
		float v1 = float(chars[p].ty)/tex_size.y;
		float u2 = float(chars[p].tx+chars[p].tw)/tex_size.x;
		float v2 = float(chars[p].ty+chars[p].th)/tex_size.y;

		layer.resize(layer.size()+4);
		DevCanvas::Vertex *v = &layer[layer.size()-4];
    
		v->pos.x = x1;
		v->pos.y = y1;
		v->z = 0;
		v->color = color;
		v->tc.x = u1;
		v->tc.y = v1;
		v++;

		v->pos.x = x2;
		v->pos.y = y1;
		v->z = 0;
		v->color = color;
		v->tc.x = u2;
		v->tc.y = v1;
		v++;

		v->pos.x = x2;
		v->pos.y = y2;
		v->z = 0;
		v->color = color;
		v->tc.x = u2;
		v->tc.y = v2;
		v++;

		v->pos.x = x1;
		v->pos.y = y2;
		v->z = 0;
		v->color = color;
		v->tc.x = u1;
		v->tc.y = v2;
		//v++;

		xp += chars[p].dx;
	}
}

void DevTxFont::DrawTextF(DevCanvas &canvas,int layer,float xp,float yp,int align,const base::vec2 &scale,int color,const char *fmt,...)
{
	string tmp;
	va_list arg;
	va_start(arg,fmt);
	vsprintf(tmp,fmt,arg);
	va_end(arg);
    DrawText(canvas,layer,xp,yp,align,scale,color,tmp.c_str());
}

void DevTxFont::DrawTextF(DevCanvas &canvas,int layer,float xp,float yp,int align,float scale,int color,const char *fmt,...)
{
	string tmp;
	va_list arg;
	va_start(arg,fmt);
	vsprintf(tmp,fmt,arg);
	va_end(arg);
    DrawText(canvas,layer,xp,yp,align,vec2(scale,scale),color,tmp.c_str());
}

float DevTxFont::DrawTextWrap(DevCanvas &canvas,int layer,float xp,float yp,float width,const base::vec2 &scale,int color,const char *s,bool nodraw)
{
	while(*s)
	{
		const char *b = s, *w = s;

		float plen = 0;
		while(*s && *s!='\n' && *s!='\\')
		{
			int code = (unsigned char)*s;
            if(code<=32) w = s;
			if(code>=32 && code<256)
			{
				float len = plen + (chars[code-32].ox + chars[code-32].tw)*scale.x;
                if(len>width) { s=w; break; }
				plen += chars[code-32].dx*scale.x;
			}
			s++;
		}
		if(s==b) s++;
		const char *e = s;

		while(*s==' ') s++;
		if(*s=='\n' || *s=='\\') s++;

		if(!nodraw) DrawText(canvas,layer,xp,yp,0x00,scale,color,b,e);
		yp += height*scale.y;
	}
    return yp;
}
