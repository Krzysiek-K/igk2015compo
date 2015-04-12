
print("======== entity.nut ========\n")


// ------------ Entity ------------

tabclass("Entity",null,{
	pos			= vec2(0,0)
	realpos		= vec2(0,0)			// realpos = pos + offs		(recomputed after every tick)
	size		= vec2(1,1)/2
	offs		= vec2(0,0)			// object offset from position
	gfx			= t_invader
	color		= 0xFFFFFFFF
	tick		= function(){}		// ran before collision tests
	latetick	= function(){}		// ran after collision tests
	collide		= function(o2){}	// called every collision frame
	_col_next	= null				// do not touch - colision linked list
})

colision_hash <- {}




function _Entity::Tick()
{
	tick()

	// fill collision

	realpos = pos + offs;
	local h = floor(realpos.x)*15+floor(realpos.y)*101;
	if( h in colision_hash )
	{
		colision_hash[h]._col_next = this
		_col_next = colision_hash[h]
	}
	colision_hash[h] <- this
}

function _Entity::Collide()
{
	ColWithHash(realpos-size)
	ColWithHash(realpos+size)
	ColWithHash(realpos-vec2(size.x,-size.y))
	ColWithHash(realpos+vec2(size.x,-size.y))
		print("--------------\n")
}

function _Entity::ColWithHash(ph)
{
	local h = floor(ph.x)*15+floor(ph.y)*101;
	print(h+" ")
	if( !(h in colision_hash) )
	{
		print("-\n")
		return;
	}
	local o2 = colision_hash[h]
	while(o2)
	{
		print(o2)
		TryCollide(o2);
		o2 = o2._col_next;
	}
	print("\n")
}

function _Entity::TryCollide(o2)
{
	if(o2==this) return;
	local bmin = realpos - size - o2.size;
	local bmax = realpos + size + o2.size;
	if( o2.realpos.x <= bmin.x ) return;
	if( o2.realpos.y <= bmin.y ) return;
	if( o2.realpos.x >= bmax.x ) return;
	if( o2.realpos.y >= bmax.y ) return;
	color = 0xFF00FF00;
	o2.color = 0xFF00FF00;
//	collide(o2)
//	o2.collide(this)
}


function _Entity::LateTick()
{
	latetick()
}


function _Entity::Draw()
{
	layer.sprite(gfx,color,pos+offs,size,0);
}


function tick_all_objects()
{
	colision_hash <- {}
	foreach(e in objects)	e.color = 0xFFFFFFFF;
	foreach(e in objects)	e._col_next = null;
	foreach(e in objects)	e.Tick();
	foreach(e in objects)	{e.Collide(); break;}
	foreach(e in objects)	e.LateTick();
}

function draw_all_objects()
{
	foreach(e in objects)
		e.Draw();
}
