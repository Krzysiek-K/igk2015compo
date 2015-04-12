print("Priv Hello!\n")

objects <- []
clear_color = 0x003068

ScreenLeftX <- 0.0
ScreenRightX <- 40.0
ScreenTopY <- 0.0
ScreenBottomY <- 30.0

NyanCatX <- ScreenLeftX+5

obj_nyan_cat <- Entity()
obj_nyan_cat.pos = vec2(NyanCatX, 10.)
obj_nyan_cat.gfx = t_nyan_cat
obj_nyan_cat.size = vec2(2, 1.2)*1.5
obj_nyan_cat.ob_layer = front_layer
obj_nyan_cat.pclick <- 0
obj_nyan_cat.tick = function()
{
	// 32 - space
	local click = get_key(1);
	if(click && !pclick)
		vel = vec2(0,-30)*1.5
	
	vel += vec2(0,100)*1.5*time_delta
	if(pos.y>ScreenBottomY)
	{
		pos = vec2(pos.x,ScreenBottomY)
		vel = vec2(0,0)
	}

	pclick = click
	pos = pos + vel * time_delta
}
obj_nyan_cat.collide = function(otherObj)
{
	//if(otherObj in RuraObjects)
	//foreach(ruraObj in RuraObjects)
	//	if(otherObj == ruraObj)
	//if(otherObj["isRura"])
	print("Nyan cat collide" + time + "\n")
}
objects.push(obj_nyan_cat)


NyanCatTailObjectHalfSizeX <- 1.0

NyanCatTailObjectCount <- (NyanCatX - ScreenLeftX) / (NyanCatTailObjectHalfSizeX * 2.0)
NyanCatTailObjects <- []
for(i <- 0; i < NyanCatTailObjectCount; ++i)
{
	local obj = Entity()
	obj.pos = vec2(ScreenLeftX + i * NyanCatTailObjectHalfSizeX * 2.0, 10.)
	obj.gfx = t_nyan_cat_tail
	obj.size = vec2(NyanCatTailObjectHalfSizeX, NyanCatTailObjectHalfSizeX * 0.6)
	obj.is_collider = 0
	obj.ob_layer = bkg_layer
	obj.tick = function()
	{
		pos = vec2(pos.x, obj_nyan_cat.pos.y)
	}
	objects.push(obj)
	NyanCatTailObjects.push(obj)
}

function RuraTick()
{
	pos = vec2(pos.x - time_delta * RuraSpeed, pos.y)
	if(pos.x < ScreenLeftX)
		pos = vec2(pos.x - ScreenLeftX + ScreenRightX, pos.y)
}

function PositionRuras(ruraUp, ruraDown)
{
	local gapY = (rand() * 1.0 / RAND_MAX) * (ScreenBottomY - ScreenTopY) * 0.5 + ScreenTopY * 0.25

	// Up
	local halfSizeY = (gapY - ScreenTopY - GapSizeY * 0.5) * 0.5
	ruraUp.pos = vec2(ruraUp.pos.x, halfSizeY)
	ruraUp.size = vec2(ruraUp.size.x, halfSizeY)

	// Down
	local halfSizeY = (ScreenBottomY - gapY - GapSizeY * 0.5) * 0.5
	ruraDown.pos = vec2(ruraDown.pos.x, ScreenBottomY - halfSizeY)
	ruraDown.size = vec2(ruraDown.size.x, -halfSizeY)
}

RuraSpeed <- 20.0
RuraColumnCount <- 2
RuraObjects <- []
RuraStepX <- (ScreenRightX - ScreenLeftX) / RuraColumnCount
x <- RuraStepX
GapSizeY <- (ScreenBottomY - ScreenTopY) / 2.5
for(i <- 0; i < RuraColumnCount; ++i)
{
	// Upper
	local obj1 = Entity()
	obj1.gfx = t_rura
	obj1.pos = vec2(ScreenLeftX + x, 0.0)
	obj1.size = vec2(1.0, 1.0)
	obj1.tick = RuraTick
	obj1.isRura <- true
	objects.push(obj1)
	RuraObjects.push(obj1)

	// Lower
	local obj2 = Entity()
	obj2.gfx = t_rura
	obj2.pos = vec2(ScreenLeftX + x, 0.0)
	obj2.size = vec2(1.0, 1.0)
	obj2.tick = RuraTick
	obj2.isRura <- true
	objects.push(obj2)
	RuraObjects.push(obj2)

	PositionRuras(obj1, obj2)

	x += RuraStepX
}

