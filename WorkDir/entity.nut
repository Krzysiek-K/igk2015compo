
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



function _Entity::Tick()
{
	tick()
	realpos = pos + offs
}

function _Entity::Collider(id)
{
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
	layer.sprite(gfx,color,pos+offs,size,0);
}


function tick_all_objects()
{
	local i;

	foreach(e in objects)	e.color = 0xFFFFFFFF;
	foreach(e in objects)	e._col_next = null;
	foreach(e in objects)	e.Tick();

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
		a.collide(b);
		b.collide(a);
	}

	foreach(e in objects)	e.LateTick();
}

function draw_all_objects()
{
	foreach(e in objects)
		e.Draw();
}
