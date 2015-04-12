
#include "common.h"



/** @file */



map<DevTexture*,int>	atex_tex2int;
map<int,DevTexture*>	atex_int2tex;
int						atex_alloc = 1;





// ---------------- API: Text files ----------------
static vector<string>	cur_file;
static int				cur_file_pos;

// ---- reading ----
/** \brief Read text file into internal array and set pointer to its start. */
bool text_readfile(const char *path)
{
	cur_file_pos = 0;
	cur_file.clear();
	return NFS.GetFileLines(path,cur_file);
}
XSQ_REGISTER_FN(text_readfile);

/** \brief Get next line of text. */
const char *text_getline()
{
	if(cur_file_pos>=cur_file.size())
		return "";
	return cur_file[cur_file_pos++].c_str();
}
XSQ_REGISTER_FN(text_getline);

/** \brief Check if file pointer is at the end. */
bool text_iseof()
{
	return cur_file_pos>=cur_file.size();
}
XSQ_REGISTER_FN(text_iseof);



// ---- writing ----
/** \brief Clear text buffer. */
void text_clear()
{
	cur_file_pos = 0;
	cur_file.clear();
}
XSQ_REGISTER_FN(text_clear);

/** \brief Add line to text buffer. */
void text_addline(const char *line)
{
	cur_file.insert(cur_file.begin()+cur_file_pos,line);
	cur_file_pos++;
}
XSQ_REGISTER_FN(text_addline);

/** \brief Frite text buffer to file. */
bool text_writefile(const char *path)
{
	return NFS.WriteFileLines(path,cur_file);
}
XSQ_REGISTER_FN(text_writefile);


// ---------------- API: Scripting ----------------

/** \brief Add script file to list of watched files. */
void script_add(const char *path)
{
	vm.AddFile(path);
}
XSQ_REGISTER_FN(script_add);


// ---------------- API: Parsing ----------------

static string _parse_buffer;
static const char *_parse_ptr = "";

/** \brief Set line to be parsed. */
void parse_setline(const char *line)
{
	_parse_buffer = line;
	_parse_ptr = _parse_buffer.c_str();
}
XSQ_REGISTER_FN(parse_setline);

/** \brief Skip whitespaces. */
void parse_white()
{
	ParseWhitespace(_parse_ptr);
}
XSQ_REGISTER_FN(parse_white);

/** \brief Check if numerical value is coming next. */
bool parse_isint()
{
	ParseWhitespace(_parse_ptr);
	return *_parse_ptr=='-' || (*_parse_ptr>='0' && *_parse_ptr<='9');
}
XSQ_REGISTER_FN(parse_isint);

/** \brief Parse integer. */
int parse_int()
{
	ParseWhitespace(_parse_ptr);
	return ParseInt(_parse_ptr);
}
XSQ_REGISTER_FN(parse_int);

/** \brief Parse float. */
float parse_float()
{
	ParseWhitespace(_parse_ptr);
	return ParseFloat(_parse_ptr);
}
XSQ_REGISTER_FN(parse_float);

/** \brief Parse string. */
const char *parse_string()
{
	static string ps;
	ParseWhitespace(_parse_ptr);
	ParseString(_parse_ptr,ps);
	return ps.c_str();
}
XSQ_REGISTER_FN(parse_string);




// ---------------- API: layers ----------------



/** \brief Flush all layers in order of creation. */
void layer_flushall()
{
	canvas.Flush();
}
XSQ_REGISTER_FN(layer_flushall);



// ---------------- API: textures ----------------

DevTexture *_id2tex(int id)
{
	if(id<=0 || id>=atex_alloc)
		return NULL;
	auto p = atex_int2tex.find(id);
	if(p==atex_int2tex.end())
		return NULL;
	return p->second;
}


/** \brief Load texture. */
int tex_load(const char *tex)
{
	DevTexture *t = canvas.GetAuto(tex);

	auto p = atex_tex2int.find(t);
	if(p!=atex_tex2int.end())
		return p->second;

	int id = atex_alloc++;
	atex_tex2int[t] = id;
	atex_int2tex[id] = t;
	
	return id;
}
XSQ_REGISTER_FN(tex_load);

/** \brief Get texture size (components: x, y). */
vec2 tex_size(int texid)
{
	DevTexture *t = _id2tex(texid);
	if(!t) return vec2(0,0);

	return t->GetSize2D();
}
XSQ_REGISTER_FN(tex_size);



#if 0
// ---------------- API: fonts ----------------
/** \brief Load font. */
int font_load(const char *font)
{
	string path = font;
	for(int i=0;i<all_fonts.size();i++)
		if( path == all_fonts[i]->GetLoadPath() )
			return i+1;
	Font *f = new Font(path.c_str());
	all_fonts.push_back(f);
	return (all_fonts.size()-1)+1;
}
XSQ_REGISTER_FN(font_load);

/** \brief Get font information (components: height). */
SqRef font_info(int fontid)
{
	SqRef ss = sq.NewObject();
	float h=0;
	fontid--;
	if(fontid>=0 && fontid<(int)all_fonts.size())
		h = all_fonts[fontid]->GetHeight();
	ss.Set("height",h);
	return ss;
}
XSQ_REGISTER_FN(font_info);

/** \brief Print single line of text. */
void font_print(int layerid,int fontid,float xp,float yp,int align,float height,int color,const char *text)
{
	layerid--;
	if(layerid<0 || layerid>=all_layers.size())
		return;
	fontid--;
	if(fontid<0 || fontid>=(int)all_fonts.size())
		return;
	Font &f = *all_fonts[fontid];
	float scale = height / f.GetHeight();
	f.DrawText(all_layers[layerid]->layer,vec2(xp,yp),align,vec2(scale,scale),color,text);
}
XSQ_REGISTER_FN(font_print);

/** \brief Print multiline text wrapping within given width. */
void font_print_wrap(int layerid,int fontid,float xp,float yp,float box_width,float height,int color,const char *text)
{
	layerid--;
	if(layerid<0 || layerid>=all_layers.size())
		return;
	fontid--;
	if(fontid<0 || fontid>=(int)all_fonts.size())
		return;
	Font &f = *all_fonts[fontid];
	float scale = height / f.GetHeight();
	f.DrawTextWrap(all_layers[layerid]->layer,vec2(xp,yp),box_width,vec2(scale,scale),color,text,false);
}
XSQ_REGISTER_FN(font_print_wrap);


#endif


// ---------------- API: drawing ----------------

/** \brief Create color from RGB triple (range 0..1). */
int make_color(float r,float g,float b)
{
	return vec3(r,g,b).make_rgba(1.f);
}
XSQ_REGISTER_FN(make_color);


/** \brief Set center and Y scale of rendered canvas. */
void set_view(float x,float y,float ysize)
{
	canvas.SetView(vec2(x,y),ysize);
}
XSQ_REGISTER_FN(set_view);



/** \brief Create color from RGB and alpha (range 0..1). */
int make_color_a(float r,float g,float b,float a)
{
	return vec3(r,g,b).make_rgba(a);
}
XSQ_REGISTER_FN(make_color_a);

/** \brief Draw quad. */
void draw_quad(int layerid,int texid,int col,vec2 bmin,vec2 bmax)
{
	DevTexture *t = _id2tex(texid);
	canvas.Draw(layerid,*t)()(col)((bmin+bmax)/2,(bmax-bmin)/2);
}
XSQ_REGISTER_FN(draw_quad);

/** \brief Draw rotated sprite. */
void draw_sprite(int layerid,int texid,int col,float x,float y,float w,float h,float angle)
{
	DevTexture *t = _id2tex(texid);
	canvas.Draw(layerid,*t)()(col)(vec2(x,y),vec2(w,h),angle);
}
XSQ_REGISTER_FN(draw_sprite);


/** \brief Draw rotated & flipped sprite (flip: bit flags). */
void draw_sprite_flip(int layerid,int texid,int col,vec2 pos,vec2 size,float angle,int flip)
{
	DevTexture *t = _id2tex(texid);
	canvas.Draw(layerid,*t)(flip)(col)(pos,size,angle);
}
XSQ_REGISTER_FN(draw_sprite_flip);

/** \brief Draw line. */
void draw_line(int layerid,int texid,int col,vec2 p1,vec2 p2,float size)
{
	DevTexture *t = _id2tex(texid);
	canvas.Draw(layerid,*t)()(col).line(p1,p2,size);
}
XSQ_REGISTER_FN(draw_line);

/** \brief Draw line with independent sizes of both ends. */
void draw_line_wedge(int layerid,int texid,int col,vec2 p1,vec2 p2,float size1,float size2)
{
	DevTexture *t = _id2tex(texid);
	canvas.Draw(layerid,*t)()(col).line(p1,p2,size1,size2);
}
XSQ_REGISTER_FN(draw_line_wedge);

/** \brief Draw line. */
void draw_line_rep(int layerid,int texid,int col,vec2 p1,vec2 p2,float size)
{
	DevTexture *t = _id2tex(texid);
	canvas.Draw(layerid,*t)()(col).linerep(p1,p2,size);
}
XSQ_REGISTER_FN(draw_line_rep);





// ---------------- API: colisions ----------------

struct ColObject {
	vec2	bmin;
	vec2	bmax;
	int		index;
};

struct Collision {
	int		id1;
	int		id2;
};

vector<ColObject> colliders;
vector<Collision> cols;
int cols_read;


/** \brief Reset colision system for a new frame. */
void col_clear()
{
	colliders.clear();
	cols.clear();
	cols_read = 0;
}
XSQ_REGISTER_FN(col_clear);

/** \brief Add circle to collision system. */
void col_box(int index,float x1,float y1,float x2,float y2)
{
	ColObject c;
	c.bmin = vec2(x1,y1);
	c.bmax = vec2(x2,y2);
	c.index = index;
	colliders.push_back(c);
}
XSQ_REGISTER_FN(col_box);

/** \brief Compute all collisions. */
void col_compute()
{
	cols.clear();
	cols_read = 0;
	for(int i=0;i<colliders.size();i++)
		for(int j=i+1;j<colliders.size();j++)
		{
			ColObject &a = colliders[i];
			ColObject &b = colliders[j];
			if( a.bmax.x <= b.bmin.x ) continue;
			if( a.bmax.y <= b.bmin.y ) continue;
			if( b.bmax.x <= a.bmin.x ) continue;
			if( b.bmax.y <= a.bmin.y ) continue;
			Collision c;
			c.id1 = i;
			c.id2 = j;
			cols.push_back(c);
		}
}
XSQ_REGISTER_FN(col_compute);

/** \brief Get next colision (returns 0/1, sets globals "col_id1" and "col_id2"). */
int col_getcol()
{
	if( cols_read>=cols.size() )
		return 0;
	vm.Set("col_id1",cols[cols_read].id1);
	vm.Set("col_id2",cols[cols_read].id2);
	cols_read++;
	return 1;
}
XSQ_REGISTER_FN(col_getcol);



// ---------------- API: input ----------------

/** \brief Get current state of a key. */
int get_key(int key)
{
	return Dev.GetKeyState(key) ? 1 : 0;
}
XSQ_REGISTER_FN(get_key);



void DevFrame()
{
	vm.Set("reso",Dev.GetScreenSizeV());
	vm.Set("time_delta",Dev.GetTimeDelta());

	vm.Run("frame");

	canvas.Flush();
}
