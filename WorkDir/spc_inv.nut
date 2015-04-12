print("Hello World!\n");

time <- 0



// --------------------------------

function think_invader()
{
	offs=vec2(0,cos(time*8+index/3.)) 
	if (rand() % 1000 < 1)
	{ 
		ebullet<-Entity()
		objects.push(ebullet)
		ebullet.pos = realpos 
		ebullet.gfx = t_brick 
		ebullet.tick = function() { 
			pos += vec2(-50*time_delta,0)

			if (pos.x < 0) 
				Remove() 
		}
	}
	pos += vec2(-5,0)*time_delta;
	if(pos.x<0)
		Remove();
}



// --------------------------------

for(y<-0;y<3;y++)
	for(x<-0;x<5;x++)
	{
		objects.push(e<-Entity())

		e.pos = vec2(x * 5 + 45, y * 8 + 5)
		e.size = vec2(1,1)
		e.index <- y*2+x*3
		e.tick = think_invader
	}






////////// -------------Player-------------
////////
////////player <- Entity() 
////////player.pos = vec2(10, 19)
////////player.gfx = t_brick
////////player.tick = function(){
////////	if (get_key(37)) // turn left 
////////	{
////////		player.pos += vec2(-time_delta, 0)
////////	}
////////	if (get_key(39)) //turn right 
////////	{
////////		player.pos += vec2(time_delta, 0)
////////	}
////////	if (get_key(1) && bullet.active == 0) // 32 - space 
////////	{
////////		bullet.pos = player.pos 
////////		bullet.active = 1
////////	}
////////}
////////
////////player.collide = @(s2)Remove()
////////
////////objects.push(player)
////////
////////// --------------------------------
////////
////////// --------player's bullet -------- 
////////
////////bullet <- Entity() 
////////bullet.pos = vec2(10, 20)
////////bullet.gfx = t_brick
////////bullet.active <- 0
////////bullet.owner = player
////////bullet.tick = function() {
////////
////////	if (bullet.active == 1)
////////	{ 
////////		bullet.pos += vec2(0, -time_delta * 5)
////////	} 
////////	
////////	if (bullet.pos.y < 0) 
////////	{
////////		bullet.active = 0 
////////	}
////////
////////}
////////
//////////bullet.pos = vec2(10, 10)
////////
////////objects.push(bullet)  
