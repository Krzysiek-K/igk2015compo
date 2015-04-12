

objects <- []


clear_color <- 0x6080E0



for(i<-0;i<20;i++)
{
	objects.push(e<-Entity())
	e.pos		= vec2(10+i*0.9,10+sin(i/3.+2)*8)
	e.index		<- i
	e.tick		= function()	{ offs=vec2(0,3*cos(time*2+index/3.)); }
	e.collide	= function(o2)	{ Remove(); }
}
