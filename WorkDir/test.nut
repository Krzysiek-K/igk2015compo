

print("Hello World!\n");


time <- 0


// ------------ layers ------------

layer <- new_layer(0)



// ------------ textures------------

texid		<- tex_load("data/texture.png")
t_brick		<- tex_load("data/brick.png")
t_invader	<- tex_load("data/invader.png")



script_add("entity.nut")

objects <- []







function frame()
{
	time += time_delta

	set_view(40/2,20/2,20)

	tick_all_objects()
	draw_all_objects()

//	layer.sprite(texid,0xFFFFFFFF,vec2(40,30)/2,vec2(1,1),time);
}


script_add("priv.nut")
