

objects <- []


for(i<-0;i<20;i++)
{
	objects.push(e<-Entity())
	e.pos		= vec2(10+i*0.7,10+sin(i/3.+2)*5)
	e.index		<- i
	e.tick		= function()	{ color=0xFFFFFFFF; offs=vec2(0,cos(time*8+index/3.)); }
	e.collide	= function(o2)	{ color=0xFFFF0000; asdfadsf=0; }
}
