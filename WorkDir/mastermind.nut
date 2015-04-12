


function spawn_cloud()
{
	objects.push(e<-Entity())

	e.pos = vec2(50,rand()%30)
	e.gfx = (e.pos.y>20 ? t_grass : t_cloud)
	e.size = vec2(43,27)/10*0.75
	e.is_collider = 0
	e.vel = vec2(-80,0)
	e.tick = @() pos+=vel*time_delta
	e.ob_layer = bkg_layer
	e.color = 0x40FFFFFF
}


function pong_think()
{
	pos+=vel*time_delta
}

function wave_pong()
{
	objects.push(e<-Entity())

	e.pos = vec2(50,rand()%30)
	e.gfx = t_pong
	e.size = vec2(1,1)
	e.is_collider = 0
	e.vel = vec2(-40,0)
	e.tick = pong_think
}


function wave_breakout()
{
	for(y<-0;y<11;y++)
		for(x<-0;x<3;x++)
		{
			objects.push(e<-Entity())

			e.pos = vec2(44+x*4.5,y*2.5+2.5)
			e.gfx = t_pong
			e.size = vec2(4,2)/2
			e.vel = vec2(-4,0)
			e.tick = @() pos+=vel*time_delta
			if(x==0) e.color = 0xFF00FF00;
			if(x==1) e.color = 0xFFFFFF00;
			if(x==2) e.color = 0xFFFF0000;
			e.collide = @(o2) Remove()
			e.enem = 1
			e.points = 25
		}
}


function add_ghost(tfx,cycle,off1,spd)
{
	objects.push(e<-Entity())

	e.pos = vec2(44+rand()*30.0/RAND_MAX,rand()*20.0/RAND_MAX+5.0)
	e.gfx = tfx
	e.size = vec2(1,1)*1.2
	e.vel = vec2(-4,0)*spd
	e.cycle <- cycle
	e.off1 <- off1
	e.tick = function() {
		pos+=vel*time_delta
		local t = time*cycle
		offs=vec2(-sin(t),cos(t))*off1
	}
	e.collide = @(o2) Remove()
	e.enem = 1
	e.points = 25
}

function wave_ghosts()
{
	local i;
	for(i=0;i<8;i++)
	{
		add_ghost( t_gh1, -20.0, vec2(1,1)*0.5, 2 )
		add_ghost( t_gh2, 3.0, vec2(4,1), 1 )
		add_ghost( t_gh3, 2.0, vec2(1,-5), 1 )
		add_ghost( t_gh4, 2.0, vec2(1,5), 1 )
	}
}


function mastermind_send()
{
	local r = rand()%4
	if(r==0) wave_invader()
	if(r==1) wave_flappy()
	if(r==2) wave_breakout()
	if(r==3) wave_ghosts()
	spawn_cat()
}

function master_mind()
{
	if(rand()%30<1) spawn_cloud();

	local en=0, ncat=0, i;
	for(i=0;i<objects.len();i++)
	{
		en += objects[i].enem;
		ncat += objects[i].cat;
	}

	if(ncat<=0) score = 0
	if(en<4) mastermind_send();

//	enemies -= time_delta
//	if(enmies<0)
//	{
//		mastermind_send();
//		enemies = 5;
//	}
}


e <- Entity()
master <- e
e.is_collider = 0
e.tick = master_mind
//e.fixedtick = mastermind_send
//e.fixedtime = 7
objects.push(e)
