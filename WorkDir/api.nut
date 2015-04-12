
// -------- font/layer wrappers --------


function new_font(name,_height)
{
	local fnt = font_load(name);
	if(_height==0)
		_height = font_info(fnt).height;
	return {
		font	= fnt,
		height	= _height
	};
}

function new_layer(shader,blend)
{
	return {
		layer = layer_add(shader,blend),
		quad		= @(tex,col,x,y,w,h) draw_quad(layer,tex,col,x,y,w,h),
		sprite		= @(tex,col,x,y,w,h,a) draw_sprite(layer,tex,col,x,y,w,h,a),
		print		= @(font,x,y,col,align,txt) font_print(layer,font.font,x,y,align,font.height,col,txt)
		print_wrap	= @(font,x,y,box_w,col,txt) font_print_wrap(layer,font.font,x,y,box_w,font.height,col,txt)
		flush		= @() layer_flush(layer)
	};
}



// -------- TouchBox --------


tabclass("TouchBox",null,{
	x=0,
	y=0,
	w=0,
	h=0
});

function _TouchBox::delta()				{ return touch_sense_delta(this,this);	}
function _TouchBox::local_prc()			{ local p=touch_sense_local_pos(this,this,-1); return p.x<0 ? null : p; }
function _TouchBox::local_pos()			{ local p=touch_sense_local_pos(this,this,-1); return p.x<0 ? null : {x=p.x*w,y=p.y*h}; }
function _TouchBox::drag_prc()			{ local p=touch_sense_drag_pos(this,this,-1); return p.x<0 ? null : p; }
function _TouchBox::slider_x(def)		{ return touch_sense_slider_x(this,this,def); }
function _TouchBox::slider_y(def)		{ return touch_sense_slider_y(this,this,def); }
function _TouchBox::id_active()			{ return touch_sense_active(this,this); }
function _TouchBox::scavenge()			{ return touch_sense_scavenge(this,this); }
function _TouchBox::is_click()			{ return touch_sense_click(this,this); }
function _TouchBox::is_hold()			{ return touch_sense_hold(this,this); }
	//	float touch_sense_hold_shared(SqRef owner,SqRef box)
function _TouchBox::is_fullbt()			{ return touch_sense_full_button(this,this); }
function _TouchBox::is_halfbt()			{ return touch_sense_half_button(this,this); }
function _TouchBox::is_btime()			{ return touch_sense_half_button_time(this,this); }
function _TouchBox::transfer_to(dst)	{ return touch_transfer_owner(this,dst); }


// constructor
function touch_box(_x,_y,_w,_h)
{
	local box = TouchBox();
	box.x = _x;
	box.y = _y;
	box.w = _w;
	box.h = _h;
	return box;
}
