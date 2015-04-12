

print("Hello World!\n");


time <- 0


// ------------ layers ------------

clear_color <- 0x6080E0

bkg_layer <- new_layer(0)
layer <- new_layer(1)
front_layer <- new_layer(2)
gui_layer <- new_layer(3)



// ------------ textures------------

texid		<- tex_load("data/texture.png")
t_brick		<- tex_load("data/brick.png")
t_invader	<- tex_load("data/invader.png")
t_nyan_cat	<- tex_load("data/Nyan Cat.png")
t_nyan_cat_tail	<- tex_load("data/Nyan Cat Tail.png")
t_rura	<- tex_load("data/Rura.png")



script_add("entity.nut")

objects <- []







function frame()
{
	time += time_delta

	set_view(40/2,20/2,20)

	tick_all_objects()
	draw_all_objects()
}



tabclass("Mouse",null,{
	pos			= vec2(0,0)
	delta		= vec2(0,0)
	id			= -1
}


mice <- {}


function mouse_update_pos(id,dx,dy,dz)
{
	if(!(id in mice)) mice[id] <- Mouse();
	mice[id].delta += vec2(dx,dy);
}

function mouse_update_buttons(id,click,down)
{
	if(!(id in mice)) mice[id] <- Mouse();
}




script_add("priv.nut")
