

print("Hello World!\n");


time <- 0
texid <- tex_load("data/texture.png")

layer <- new_layer(0)




function frame()
{
	time += time_delta

	set_view(40/2,30/2,30)
	layer.sprite(texid,0xFFFFFFFF,vec2(40,30)/2,vec2(1,1),time);
}


script_add("priv.nut")
