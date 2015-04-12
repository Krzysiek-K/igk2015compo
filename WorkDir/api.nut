
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

function new_layer(id)
{
	return {
		layer = id,
		quad		= @(tex,col,bmin,bmax) draw_quad(layer,tex,col,bmin,bmax),
		sprite		= @(tex,col,pos,size,a) draw_sprite(layer,tex,col,pos.x,pos.y,size.x,size.y,a),
//		print		= @(font,x,y,col,align,txt) font_print(layer,font.font,x,y,align,font.height,col,txt)
//		print_wrap	= @(font,x,y,box_w,col,txt) font_print_wrap(layer,font.font,x,y,box_w,font.height,col,txt)
	};
}
