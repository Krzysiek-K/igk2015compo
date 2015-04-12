
print("======== entity.nut ========\n")


// ------------ Entity ------------

tabclass("Entity",null,{
	pos			= vec2(0,0)
	size		= vec2(1,1)/2
	offs		= vec2(0,0)			// object offset from position
	gfx			= t_invader
	angle		= 0					// draw rotation (in revolutions)
	color		= 0xFFFFFFFF
	tick		= @()0				// ran before collision tests
	latetick	= @()0				// ran after collision tests
	collide		= @(o2)0			// called every collision frame
	kill		= @(killer)Remove()	// called when something attempts to kill me
	destroyed	= @()_defdestr()	// called just before being removed (return 0 to prevent)
	is_collider	= 1
	ob_layer	= layer
	owner		= null

	fixedtick	= @()0				// ran at constant intervals (always before tick())
	fixedtime	= 0					// if>0 then runs fixedtick() at that intervals
	fixedmulti	= 1					// max fixed invoke count per frame
	_fixcnt		= 0					// counter for fixedtime

	// ----- updated automatically after tick() -----
	array_index	= -1				// index in objects[] array
	realpos		= vec2(0,0)			// realpos = pos + offs
})


function _dim()
{
	killtime += time_delta
	size = size1*(1+killtime*10)
	angle += time_delta*1.5

	color = (color&0xFFFFFF) | ((255-killtime/0.25*255).tointeger()*0x01000000)

	if(killtime>0.25)
		Remove();
}

function _defdestr()
{
	size1 <- size
	killtime <- 0
	tick		= _dim
	latetick	= @()0
	collide		= @(o2)0
	kill		= @()0
	destroyed	= @()1
	is_collider	= 0
	return 0;
}



function _Entity::Remove()
{
	if(array_index<0) return;
	if(!destroyed()) return;

	objects[array_index] = objects.top();
	objects.pop();

	if(array_index<objects.len())
		objects[array_index].array_index = array_index
}


function _Entity::Tick()
{
	if(fixedtime>0)
	{
		if(fixedtime<_fixcnt)
			fixedtime = _fixcnt;

		local invokes = fixedmulti;

		_fixcnt -= time_delta
		while(_fixcnt<=0 && invokes>0)
		{
			invokes--
			_fixcnt += fixedtime
			fixedtick()
		}
		if(_fixcnt<0)
			_fixcnt = 0;
	}
	else
		_fixcnt = 0

	tick()
	realpos = pos + offs
}

function _Entity::Collider(id)
{
	if(!is_collider) return;
	local bmin = realpos - size;
	local bmax = realpos + size;
	col_box(id,bmin.x,bmin.y,bmax.x,bmax.y)
}

function _Entity::LateTick()
{
	latetick()
}


function _Entity::Draw()
{
	ob_layer.sprite(gfx,color,pos+offs,size,angle);
}


function tick_all_objects()
{
	local i;

	foreach(e in objects)	e.color = 0xFFFFFFFF;
	foreach(i,e in objects)
	{
		e.Tick();
		e.array_index = i;
	}

	local colid = {}
	col_clear()
	foreach(i,e in objects)
	{
		e.Collider(i);
		colid[i] <- e
	}
	col_compute()

	while(col_getcol())
	{
		local a = colid[col_id1];
		local b = colid[col_id2];
		if( a.owner!=b && b.owner!=a )
		{
			a.collide(b);
			b.collide(a);
		}
	}

	foreach(e in objects)	e.LateTick();
}

function draw_all_objects()
{
	foreach(e in objects)
		e.Draw();
}
