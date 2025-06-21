#include "projectiles.h"

//Included code files
#include <stdlib.h>
#include <tonc.h>
#include <vector>

#include "gameDefines.h" //Global game defines
#include "globalvars.h"
#include "sfx.h"

std::vector <CProjectile> projectiles; //Projectiles class instance vector

int ballCollisionSoundCounter; //Allows us to space out the ball collision sounds

CProjectile::CProjectile(T_LEVELSTATE* ls, FIXED _x, FIXED _y, FIXED _dx, FIXED _dy, int _type, int _lifespan, int _weight)
{
	x = _x<<8; y = _y<<8; dx = _dx; dy = _dy; type = _type; lifespan = _lifespan; weight = _weight;
	active = true;

	//If the projectile has a weight of 2 then reduce the speed (Applies to ball gun)
	if (weight == 2)
	{
		dx = dx / 2;
	}


	//Find the next free oam entry
	oam_index = 127;
	int n;
	bool oamFound;

	n = 0;
	oamFound = false;
	while ((n < MAX_PROJECTILES) && oamFound == false)
	{
		//Check the level state for the next free oam index
		if (ls->projectileOamIsFree[n] == true)
		{
			//This oam index is free so add a new projectile to the array at position n,
			//set the levelstate projectile to active and break from the loop.
			oam_index = PROJECTILE_FIRST_OAM + n;
			ls->projectileOamIsFree[n] = false;
			oamFound = true;
		}
		n++;
	}

	OBJ_ATTR *obj = &g_obj_buffer[oam_index];

	//Pointer to global level state
	m_ls = ls;

	//Set the projectile sprite attributes based on the type
	//type 0 = left arrow, 1 = right arrow, 2 = gun bullet
	switch (type)
	{

		case 0: //Left Arrow
			obj_set_attr(obj,
			 ATTR0_WIDE |           // Wide, regular sprite
			 ATTR0_4BPP |           //16 colours
			 ATTR0_MODE(0),         //Un-hide the sprite
			 ATTR1_SIZE_8,          // 16x8p
			 ATTR2_PALBANK(0) |
			 ATTR2_PRIO(2) |
			 ATTR2_ID(SPRTILES_PROJECTILES));
		break;

		case 1: //Right Arrow
			obj_set_attr(obj,
			ATTR0_WIDE |			// Wide, regular sprite
			ATTR0_4BPP |            //16 colours
			ATTR0_MODE(0),          //Un-hide the sprite
			ATTR1_SIZE_8,           // 16x8p
			ATTR2_PALBANK(0) |
			ATTR2_PRIO(2) |
			ATTR2_ID(SPRTILES_PROJECTILES + 2));
		break;

		case 2: //Gun bullet
			obj_set_attr(obj,
			ATTR0_SQUARE |          // Square, regular sprite
			ATTR0_4BPP |            //16 colours
			ATTR0_MODE(0),          //Un-hide the sprite
			ATTR1_SIZE_8,           // 8x8p
			ATTR2_PALBANK(0) |
			ATTR2_PRIO(2) |
			ATTR2_ID(SPRTILES_PROJECTILES + 4));
		break;

		case 3: //Snake bullet
			obj_set_attr(obj,
			ATTR0_SQUARE |          // Square, regular sprite
			ATTR0_4BPP |            //16 colours
			ATTR0_MODE(0),          //Un-hide the sprite
			ATTR1_SIZE_8,           // 8x8p
			ATTR2_PALBANK(0) |
			ATTR2_PRIO(2) |
			ATTR2_ID(SPRTILES_PROJECTILES + 5));
		break;

		case 4: //Bouncing ball (Level 5 object)
			obj_set_attr(obj,
			ATTR0_WIDE |			// Wide, regular sprite
			ATTR0_4BPP |            //16 colours
			ATTR0_MODE(0),          //Un-hide the sprite
			ATTR1_SIZE_8,           // 16x8p
			ATTR2_PALBANK(0) |
			ATTR2_PRIO(2) |
			ATTR2_ID(SPRTILES_PROJECTILES + 6));
		break;
	}
	// Set object coords
	obj_set_pos(obj, x>>8, y>>8);

}

CProjectile::~CProjectile()
{
}

void CProjectile::Update()
{
	POINT distFromViewport; //Projectile's distance from centre of map viewport
	POINT collisionOffset; //The point on the projectile that is tested for collisions
	int projectileWidth;
	POINT tilePos; //The tile position that the projectile's collision point is in.
	POINT newTilePos; //The tile position that the projectile's collision point
					  //will be in. Used for bouncing balls.
	int collidedWith;

	//See if the projectile went out of range of the viewpoint location
	//Deactivate it if it has.
	//Screen resoulution = 240 × 160
	distFromViewport.x = ((x>>8) + m_ls->vp.x) - (m_ls->vp.x + 120);
	distFromViewport.y = ((y>>8) + m_ls->vp.y) - (m_ls->vp.y + 80);

	//20 pixels off the screen horizontally or vertically for all except gun
	//bullets, which is 32 pixels horizontally or vertically (CHANGED)
	if (type != 2) {
		if ((abs(distFromViewport.x) > 240) || (abs(distFromViewport.y) > 160))
		{
			active = false;
		}
	}
	else
	{
		if ((abs(distFromViewport.x) > 240) || (abs(distFromViewport.y) > 160))
		{
			active = false;
		}
	}

	//Check for collisions against walls. For arrows, check for collision against
	//urns.
	//NB: Collision checks for other objects will be done in other places
	//For bullets(gun/snake/ball) collisions with the player will be checked
	//in player.cpp. For arrows, collisions against enemies will be
	//checked in enemies.cpp.

	//If it's already inactive then we don't need to detect collisions
	if (active == true)
	{
		//Get the x collision offset for the projectile. Left/right point for left/right
		//arrows. Centre for others.
		switch (type)
		{
			case 0: //Left arrow
				collisionOffset.x = 0;
				collisionOffset.y = 2;
				projectileWidth = 16;
			break;
			case 1: //Right arrow
				collisionOffset.x = 15;
				collisionOffset.y = 2;
				projectileWidth = 16;
			break;
			case 2:
				//Gun bullet
				collisionOffset.x = 4;
				collisionOffset.y = 2;
				projectileWidth = 8;
			break;
			case 3:
				//Snake bullet
				collisionOffset.x = 2;
				collisionOffset.y = 2;
				projectileWidth = 4;
			break;
			case 4:
				//Bouncing Ball
				collisionOffset.x = 4;
				collisionOffset.y = 4;
				projectileWidth = 8;
			break;
			default:
				//Default (Avoids a compiler warning)
				collisionOffset.x = 2;
				collisionOffset.y = 2;
				projectileWidth = 4;
			break;
		}

		//Process all projectiles except the bouncing ball
		if (type < 4)
		{
			//Get the tile map position of the projectile's collision point.
			//I need to get the projectile position relative to the map, since
			//the x,y properties are relative to the screen (Add the viewpoint
			//coordinates)
			tilePos.x = ((((x>>8) + m_ls->vp.x) + collisionOffset.x) >>5);
			tilePos.y = ((((y>>8) + m_ls->vp.y) + collisionOffset.y) >>4);

			//Check wall collisions
			if ( m_ls->mapData[0][(tilePos.y*80) + tilePos.x].tileIndex > 0)
			{
				//Deactivate the projectile upon hitting a wall
				active = false;
			}

			//Check collision with solid wooden frames and chests
			if (( m_ls->mapData[1][(tilePos.y*80) + tilePos.x].tileIndex == TILE_WOOD_FRAME_TOP) ||
					( m_ls->mapData[1][(tilePos.y*80) + tilePos.x].tileIndex == TILE_WOOD_FRAME) ||
					( m_ls->mapData[1][(tilePos.y*80) + tilePos.x].tileIndex == TILE_CHEST))
				{
					//Deactivate the projectile
					active = false;
				}

			//Check for collisions with a closed door
			collidedWith = m_ls->mapData[1][(tilePos.y*80) + tilePos.x].tileIndex;
			if ((collidedWith == TILE_DOOR_CLOSED_TOP) || (collidedWith == TILE_DOOR_CLOSED_BOTTOM))
			{
				//Projectile intersects a door tile, so check if it intersects
				//the door's bounding box. the door width is 8 pixels.
				//Check if any part of the projectile touches the door, otherwise
				//the player is able to shoot arrows through doors.
				//TODO: You can still shoot through doors to the left, because the
				//arrow point is colliding with the block to the left of the door.
				//I can try and fix this in the future.
				if ((((x>>8) + m_ls->vp.x) <= ((tilePos.x*32) + 8)) &&
				   ((((x>>8) + m_ls->vp.x) + projectileWidth) >= (tilePos.x*32)))
				{
					active = false;
				}
			}

			//Check if it collided with an urn (top or bottom half)
			collidedWith = m_ls->mapData[1][(tilePos.y*80) + tilePos.x].tileIndex;
			if ((collidedWith == TILE_URN_TOP) || (collidedWith == TILE_URN_BOTTOM) )
			{
				//See if the projectile intersects the urns bounding box (2 left, 28 right)
				if ((((x>>8) + m_ls->vp.x) <= ((tilePos.x*32) + 28)) &&
					  ((((x>>8) + m_ls->vp.x) + projectileWidth) >= (tilePos.x*32) + 2))
				{

					//Deactivate the projectile
					active = false;

					//If this was an arrow, set the urn as being hit
					if ((type == 0) || (type == 1))
					{
						//Set the urnWasHit variable in levelstate to the urn position
						m_ls->urnWasHit.x = tilePos.x;
						if (collidedWith == TILE_URN_TOP)
						{
							//Top half was hit
							m_ls->urnWasHit.y = tilePos.y;
						}
						else
						{
							//Bottom half was hit
							m_ls->urnWasHit.y = tilePos.y - 1;
						}
					}
				}
			}
		}
		//Process the bouncing ball
		else
		{
			//The bouncing ball will fall with gravity and bounce unless the
			//gun was at ground level, then it will just roll along the floor
			//It will collide with walls (and closed doors) on all sides.
			//The bounce height will be related to the falling velocity.
			//The horizontal velocity will be constant.

			//Get the tile position of the ball's center. If this is within
			//a wall then deactivate the ball.
			tilePos.x = ((((x>>8) + m_ls->vp.x) + 5) >>5);
			tilePos.y = ((((y>>8) + m_ls->vp.y) + 4) >>4);
			if ( m_ls->mapData[0][(tilePos.y*80) + tilePos.x].tileIndex > 0)
			{
				//Deactivate the ball
				active = false;
			}

			//Calculate the current tile position of the balls sides depending
			//on it's direction and the tile position that the balls sides will
			//be in after it's movement is made during this frame.
			tilePos.x = ((((x>>8) + m_ls->vp.x) + ((dx>0) ? 9 : 0)) >>5);
			tilePos.y = ((((y>>8) + m_ls->vp.y) + ((dy>0) ? 7 : 0)) >>4);
			newTilePos.x = (((((x+dx)>>8) + m_ls->vp.x) + ((dx>0) ? 9 : 0)) >>5);
			newTilePos.y = (((((y+dy)>>8) + m_ls->vp.y) + ((dy>0) ? 7 : 0)) >>4);

			//Check right/left
			//If the new tile is different and is a solid wall, wooden frame or door
			//then reverse the x velocity. (9 is right side of ball)
			if (newTilePos.x != tilePos.x)
			{
				if ( (m_ls->mapData[0][(tilePos.y*80) + newTilePos.x].tileIndex > 0) ||
					 (m_ls->mapData[1][(tilePos.y*80) + newTilePos.x].tileIndex == TILE_WOOD_FRAME) ||
					 (m_ls->mapData[1][(tilePos.y*80) + newTilePos.x].tileIndex == TILE_WOOD_FRAME_TOP) ||
					 (m_ls->mapData[1][(tilePos.y*80) + newTilePos.x].tileIndex == TILE_DOOR_CLOSED_TOP) ||
					 (m_ls->mapData[1][(tilePos.y*80) + newTilePos.x].tileIndex == TILE_DOOR_CLOSED_BOTTOM) )
				{
					//Reverse the horizontal velocity
					dx *= -1;
					//Play a sound (Same as the title screen bounce sound)
					//Make sure the sound counter is <= zero first. This allows
					//us to space out the collision sounds.
					if (ballCollisionSoundCounter <= 0)
					{
						mmEffect(SFX_TITLE);
						ballCollisionSoundCounter = 15;
					}
				}
			}
			//Check up/down using the same method(Shift right by 4 to get tile
			//y position)
			if (newTilePos.y != tilePos.y)
			{
				//Check for a wall or the top of a wooden platform
				if ((m_ls->mapData[0][(newTilePos.y*80) + tilePos.x].tileIndex > 0)
				|| ((newTilePos.y > tilePos.y) && (( m_ls->mapData[1][(newTilePos.y*80) + tilePos.x].tileIndex == TILE_PLATFORM1) ||
				    (m_ls->mapData[1][(newTilePos.y*80) + tilePos.x].tileIndex == TILE_PLATFORM2))))
				{

					//Reverse the vertical velocity
					dy *= -1;
					//If the ball has > 1 pixel of movement then play a sound (Same as the title screen bounce sound)
					if (ABS(dy) > 128)
					{
						//Play a sound (Same as the title screen bounce sound)
						//Make sure the sound counter is <= zero first. This allows
						//us to space out the collision sounds.
						if (ballCollisionSoundCounter <= 0)
						{
							mmEffect(SFX_TITLE);
							ballCollisionSoundCounter = 15;
						}
					}
					//Check the weight value
					switch (weight)
					{
						case 0: //Light ball - Keeps same height when bouncing
							//I still have to decellerate it slightly to keep the same height
							if (dy < 0) {dy *= 0.98;}
						break;
						case 1: //Normal ball - Height is gradually reduced when bouncing
							//If the ball is now moving up, reduce the upwards velocity a little
							//Then if the vertical velocity is below a certain threshold, set it
							//to zero.
							if (dy < 0) {dy *= 0.85;}
							if (ABS(dy) < 128) {dy = 0;}
							
						break;
						case 2: //Heavy ball - Does not bounce
							//If the ball would be moving up, reduce the velocity to zero.
							if (dy < 0) {dy = 0;}
						break;
					}
				}
			}
		}

		//Update the projectile position
		x += dx; y += dy;

		// Set object coords
		obj_set_pos(&g_obj_buffer[oam_index], x>>8, y>>8);


		//Update the vertical velocity if it's a ball (Apply the force of gravity)
		//Also update the lifespan value and deactivate if necessary.
		if (type == 4)
		{
			dy += BALL_ACCELERATION;
			if (lifespan > 0) {lifespan--;}
			if (lifespan == 0) {active = false;}
		}
	}
}

void add_projectile(T_LEVELSTATE *ls, int _x, int _y, int _dx, int _dy, int _type, int _lifespan, int _weight)
{
	//If we already have the maximum number of projectiles,
	//erase the oldest one
	if (projectiles.size() == MAX_PROJECTILES)
	{
		//Mark this projectile's oam index as being free in the levelstate struct.
		projectiles.begin()->m_ls->projectileOamIsFree[projectiles.begin()->oam_index - PROJECTILE_FIRST_OAM] = true;

		//Hide the object
		obj_hide(&g_obj_buffer[projectiles.begin()->oam_index]);

		projectiles.erase(projectiles.begin());
	}
	projectiles.push_back(CProjectile(ls, _x, _y, _dx, _dy, _type, _lifespan, _weight));
}

void update_projectiles(T_LEVELSTATE *ls)
{
	(void) ls;

	//Use a for loop without an it++ in the for line. Allows us to delete the last element
	//in the vector without crashing.
	//See: http://stackoverflow.com/questions/9927163/erase-element-in-vector-while-iterating-the-same-vector
	for(std::vector<CProjectile>::iterator it = projectiles.begin(); it != projectiles.end();)
	{
		//Check whether this projectile is active
		if (it->active == true)
		{
			it->Update();
			++it;
		}
		else
		{
			//Mark this projectile's oam index as being free in the levelstate struct.
			it->m_ls->projectileOamIsFree[it->oam_index - PROJECTILE_FIRST_OAM] = true;

			//Hide the object
			obj_hide(&g_obj_buffer[it->oam_index]);

			it = projectiles.erase(it);
		}
	}
	
	//If the ball collision sound counter is > 0 then reduce it
	if (ballCollisionSoundCounter > 0)
	{
		ballCollisionSoundCounter--;
	}
}

void scroll_projectiles(int x, int y)
{
	//Update the projectiles positions by x and y, which will be got from map.cpp and is the amount
	//the map scrolled on this vbl.
	for(std::vector<CProjectile>::iterator it = projectiles.begin(); it != projectiles.end();it++)
	{
		//Update the coordinates.
		it->x-=(x<<8); it->y-=(y<<8);
	}
}

void reset_projectiles(T_LEVELSTATE *ls)
{
	int n;

	projectiles.clear();
	for (n=0;n<MAX_PROJECTILES;n++)
	{
		ls->projectileOamIsFree[n] = true;
		//Hide the object
		obj_hide(&g_obj_buffer[PROJECTILE_FIRST_OAM + n]);
	}
	
	//Reset the ball collision sound counter. 
	ballCollisionSoundCounter = 0;
}

std::vector <CProjectile>& getProjectiles() {return projectiles;}

