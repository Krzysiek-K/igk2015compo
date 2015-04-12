print("Hello World!\n");

time <- 0

print(time)
// ------------ layers ------------



// --------------------------------

function think_invader() { 
	offs=vec2(0,cos(time*8+index/3.)) 
	if (rand() % 1000 < 1)
	{ 
		ebullet<-Entity()
		objects.push(ebullet)
		ebullet.pos = realpos 
		ebullet.gfx = t_brick 
		ebullet.tick = function() { 
			pos += vec2(0, time_delta * 5)

		if (pos.y > 25) 
			Remove() 
		}
	}
}




// -------------Player-------------

player <- Entity() 
player.pos = vec2(10, 19)
player.gfx = t_brick
player.tick = function(){
	if (get_key(37)) // turn left 
	{
		player.pos += vec2(-time_delta, 0)
	}
	if (get_key(39)) //turn right 
	{
		player.pos += vec2(time_delta, 0)
	}
	if (get_key(32) && bullet.active == 0) //space 
	{
		bullet.pos = player.pos 
		bullet.active = 1
	}
}

player.collide = @(s2)Remove()

objects.push(player)

// --------------------------------

// --------player's bullet -------- 

bullet <- Entity() 
bullet.pos = vec2(10, 20)
bullet.gfx = t_brick
bullet.active <- 0
bullet.owner = player
bullet.tick = function() {

	if (bullet.active == 1)
	{ 
		bullet.pos += vec2(0, -time_delta * 5)
	} 
	
	if (bullet.pos.y < 0) 
	{
		bullet.active = 0 
	}

}

//bullet.pos = vec2(10, 10)

objects.push(bullet)  

// --------------------------------

layer <- new_layer(0) 
x <- 0
y <- 0 

	for(i<-0;i<30;i++)
	{
		y = (i / 10) 

		x = i % 10; 

		objects.push(e<-Entity())
		//e.pos = vec2(10+i,10+sin(i/3.)*5) 

		e.pos = vec2(x * 2 + 10, y * 2 + 2)

		/*
		print("To jest TEN print\n")
		print(e.pos)
		print("\n")
		*/

		e.index <- x
		e.real_index <- i
		e.tick = think_invader
	}

// ------------ textures------------
