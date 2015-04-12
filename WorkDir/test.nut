

print("Hello World!\n");


time <- 0


// ------------ layers ------------

layer <- new_layer(0)



// ------------ textures------------

texid		<- tex_load("data/texture.png")
t_brick		<- tex_load("data/brick.png")
t_invader	<- tex_load("data/invader.png")





// ------------ Entity ------------

tabclass("Entity",null,{
	pos		= vec2(0,0)
	size	= vec2(1,1)/2
	offs	= vec2(0,0)		// object offset from position
	gfx		= t_invader
	tick	= function(){}
})



function _Entity::Draw()
{
	layer.sprite(gfx,0xFFFFFFFF,pos+offs,size,0);
}



objects <- []







function frame()
{
	time += time_delta

	set_view(40/2,20/2,20)

	foreach(e in objects)	e.tick();
	foreach(e in objects)	e.Draw();

//	layer.sprite(texid,0xFFFFFFFF,vec2(40,30)/2,vec2(1,1),time);
}


script_add("priv.nut")
