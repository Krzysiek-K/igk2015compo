

tabclass("Entity",null,{
	pos = vec2(0,0)
	size = vec2(1,1)
})


function _Entity::Draw()
{
	layer.sprite(t_brick,0xFFFFFFFF,pos,size,0);
}


t_brick <- tex_load("data/brick.png")


objects <- []


for(i<-0;i<20;i++)
{
	objects.push(e<-Entity())
	e.pos = vec2(10+i,10+sin(i/3.)*5)
}



function frame()
{
	time += time_delta

	set_view(40/2,30/2,30)

	foreach(e in objects)
		e.Draw();
}
