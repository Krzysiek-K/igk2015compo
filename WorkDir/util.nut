
// declares a "class" using tables as instances
//  define class methods separately like:	function _MyClass::myfun() {...}
function tabclass( name, parent, defs )
{
	local root = getroottable();
	local _name = "_"+name;
	if(!root.rawin(_name)) root[_name] <- {}
	root[_name].setdelegate(parent);

	defs.setdelegate( root[_name] );

	root[name] <- function() {
		return clone defs;
	};
}




tabclass("Rect",null,{
	x=0,
	y=0,
	w=0,
	h=0
});

function RectPos(_x,_y,_w,_h) {
	local r = Rect();
	r.x=_x;
	r.y=_y;
	r.w=_w;
	r.h=_h;
	return r;
}




function is(name)
{
	return getroottable().rawin(name);
}
