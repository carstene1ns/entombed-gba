#include "enemies.h"

#include <tonc.h>
#include <vector>

#include "gameDefines.h" //Global game defines
#include "globalvars.h"
#include "projectiles.h"
#include "sfx.h"

std::vector <CEnemy> enemies; //Visible enemies class instance vector

CEnemy::CEnemy(T_LEVELSTATE* ls, FIXED _x, FIXED _y, FIXED _dx, FIXED _dy, int _type, int _xMin,
               int _xMax, int _hitPoints, int _fireRate,  int _obj_index)
{
	x = _x << 8;
	y = _y << 8;
	dx = _dx;
	dy = _dy;
	type = _type;
	xMin = _xMin;
	xMax = _xMax;
	obj_index = _obj_index;
	active = true;
	hitPoints = _hitPoints;
	fireRate = _fireRate;

	//Prevent divide by zero error with fire rate
	if (fireRate == 0) fireRate = 1;
	isDying = false;
	dyingMotionCounter = 0;
	animFrame = 0;
	flashCounter = -1;

	//Pointer to global level state
	m_ls = ls;

	//Find the next free oam entry
	oam_index = 127;
	int n;
	bool oamFound;

	n = 0;
	oamFound = false;
	while ((n < MAX_VISIBLE_ENEMIES) && oamFound == false)
	{
		//Check the level state for the next free oam index
		if (ls->enemyOamIsFree[n] == true)
		{
			//This oam index is free so add a new projectile to the array at position n,
			//set the levelstate projectile to active and break from the loop.
			oam_index = ENEMY_FIRST_OAM + n;
			ls->enemyOamIsFree[n] = false;
			oamFound = true;
		}
		n++;
	}

	OBJ_ATTR *obj = &g_obj_buffer[oam_index];

	switch (type)
	{
		//Each enemy has 8 frames of animation, 2 walking left, 2 walking right, flash left
		//after being hit, flash right after being hit and 2 dying frames.
		//so that's 8 * 16 tiles = 128
		case TILE_SNAKE: //Snake
			shootCounter = SHOOT_DELAY / fireRate;

			obj_set_attr(obj,
			             ATTR0_SQUARE |              // Square, regular sprite
			             ATTR0_4BPP |                //16 colours
			             ATTR0_MODE(0),              //Un-hide the sprite
			             ATTR1_SIZE_32,              // 32x32p
			             ATTR2_PALBANK(0) |
			             ATTR2_PRIO(2) |
			             //Facing left or right depending on speed.
			             ATTR2_ID((dx < 0) ? SPRTILES_ENEMIES : SPRTILES_ENEMIES + (16 * 2)));
			break;

		case TILE_SPHINX: //Sphinx
			shootCounter = 0;

			obj_set_attr(obj,
			             ATTR0_SQUARE |              // Square, regular sprite
			             ATTR0_4BPP |                //16 colours
			             ATTR0_MODE(0),              //Un-hide the sprite
			             ATTR1_SIZE_32,              // 32x32p
			             ATTR2_PALBANK(0) |
			             ATTR2_PRIO(2) |
			             //Facing left or right depending on speed.
			             ATTR2_ID((dx < 0) ? SPRTILES_ENEMIES + (16 * 8) : SPRTILES_ENEMIES + (16 * 10)));
			break;

	}

	// Set object coords
	obj_set_pos(obj, x >> 8, y >> 8);

}

void CEnemy::Update()
{
	int arrowXPoint;
	POINT distFromViewport; //Enemy's distance from centre of map viewport
	OBJ_ATTR *obj =  &g_obj_buffer[oam_index];
	std::vector <CProjectile> &projectiles = getProjectiles();

	//If the enemy went out of visible range, deactivate it and
	//set the position in ls->mapSprites to it's last known position
	//if it is still available (hasn't been killed)
	//Screen resoulution = 240 × 160
	distFromViewport.x = ((x >> 8) + m_ls->vp.x) - (m_ls->vp.x + 120);
	distFromViewport.y = ((y >> 8) + m_ls->vp.y) - (m_ls->vp.y + 80);
	//64 pixels off the screen horizontally or vertically
	if ((ABS(distFromViewport.x) > 184) || (ABS(distFromViewport.y) > 144))
	{
		active = false;
		m_ls->mapSprites[obj_index].visible = false;
		if (m_ls->mapSprites[obj_index].available == true)
		{
			//Store position and direction
			m_ls->mapSprites[obj_index].x = ((x >> 8) + m_ls->vp.x);
			m_ls->mapSprites[obj_index].y = ((y >> 8) + m_ls->vp.y);
			m_ls->mapSprites[obj_index].properties[3] = (dx > 0) ? 1 : 0;
		}
	}

	//Check that we're not dying. If we are then we're only going to
	//update the position and animation frame.
	if (isDying == false)
	{
		//See if we're at xMin or xMax and need to turn around
		//xMin and xMax are pixel coords relative to the map
		//Need to convert enemy x coord from relative to screen to relative to map.
		if ((((x >> 8) + m_ls->vp.x) <= xMin) && (dx < 0))
		{
			//Turn right
			dx = ABS(dx); //Set x speed to positive value
		}
		else
		{
			if ((((x >> 8) + m_ls->vp.x) >= xMax) && (dx > 0))
			{
				//Turn left
				dx = 0 - dx; //Set x speed to negative value
			}
		}

		//Check for any arrows that are colliding with this enemy
		for (auto& projectile : projectiles)
		{
			//Check if the projectile is an arrow
			if (projectile.type < 2)
			{
				//See if the bullet intersects the enemy's bounding box.
				//NB: projectile and enemy coordinates are both relative to the screen
				//Check for collisions with the front point of the arrow.
				arrowXPoint = projectile.type == 0 ? 0 : 15;
				if ((((projectile.x >> 8) + arrowXPoint) >= ((x >> 8) + ENEMY_BBOX_LEFT)) &&
				    (((projectile.x >> 8) + arrowXPoint) <= ((x >> 8) + ENEMY_BBOX_RIGHT)) &&
				    (((projectile.y >> 8) + 1) >= ((y >> 8) + ENEMY_BBOX_TOP)) &&
				    (((projectile.y >> 8) + 1) <= ((y >> 8) + 31))) //Bottom
				{
					//This bullet intersects the enemy's bounding box so reduce the enemy's hitpoints
					//by 1 * the number of player bows.
					hitPoints -= m_ls->bows;

					//Deactivate the arrow
					projectile.active = false;

					//Set the enemy to flashing
					flashCounter = ENEMY_FLASH_FRAMES;
				}
			}
		}

		//If it's a snake, if it's firing counter is 0 and the player is level with the snake then
		//fire a bullet.
		if (type == TILE_SNAKE)
		{
			//See if the player is level with this enemy vertically
			if (((m_ls->playerPos.y >> 8) - m_ls->vp.y + 7 <= ((y >> 8) + 31)) &&
			    (((m_ls->playerPos.y >> 8) - m_ls->vp.y + 31) >= ((y >> 8) + 9)))
			{
				if (shootCounter > 0)
				{
					shootCounter--;
				}
				else
				{
					//Shoot a bullet in the direction we're facing
					if (dx < 0)
					{
						//Shoot left
						add_projectile(m_ls, (x >> 8) - 4, (y >> 8) + 12, -SNAKE_BULLET_SPEED, 0, 3, -1, 1);
						//Play a sound
						mmEffect(SFX_BULLET);
					}
					else
					{
						//Shoot right
						add_projectile(m_ls, (x >> 8) + 32, (y >> 8) + 12, SNAKE_BULLET_SPEED, 0, 3, -1, 1);
						//Play a sound
						mmEffect(SFX_BULLET);
					}
					//Reset shootcounter
					shootCounter = SHOOT_DELAY / fireRate;
				}
			}
		}

		//Update the animation (Not dying)
		//If we're currently flashing after an arrow hit, set the animation
		//to the flashing frame then reduce the flash counter
		if (flashCounter >= 0)
		{
			if (type == TILE_SNAKE)
			{
				//Snake (584,600, frame 4-5)
				BFN_SET(obj->attr2, SPRTILES_ENEMIES + (16 * 4) + ((dx > 0) ? 16 : 0), ATTR2_ID);
			}
			else
			{
				//Sphinx (712,728, frame 12-13)
				BFN_SET(obj->attr2, SPRTILES_ENEMIES + (16 * 12) + ((dx > 0) ? 16 : 0), ATTR2_ID);
			}
			//Decrement the flash counter
			flashCounter--;

			//If flashCounter is now <0 then see if we have <0 hitpoints
			if ((flashCounter < 0) && (hitPoints <= 0))
			{
				//Set the enemy as dying, and set the mapSprites
				//object as unavailable
				isDying = true;
				m_ls->mapSprites[obj_index].available = false;
				dyingMotionCounter = 16;
				dy = 0 - (ABS(dx) << 1); //Double the walking speed
				dx = 0;

				//Play a sound
				mmEffect(SFX_ENEMY_DIE);
				//Add 1000 points for a sphinx, 1500 for a snake
				if (type == TILE_SPHINX)
				{
					g_score += 1000;
				}
				else
				{
					g_score += 1500;
				}
			}
		}
		else
		{
			//Use a walking animation frame
			if (type == TILE_SNAKE)
			{
				//Snake (From 520, frame 0)
				BFN_SET(obj->attr2, SPRTILES_ENEMIES + ((dx > 0) ? 32 : 0) + ((animFrame >> 8) * 16), ATTR2_ID);
			}
			else
			{
				//Sphinx (From 648, frame 8)
				BFN_SET(obj->attr2, SPRTILES_ENEMIES + (16 * 8) + ((dx > 0) ? 32 : 0) + ((animFrame >> 8) * 16),
				        ATTR2_ID);
			}
			//Increment anim frame
			animFrame += ENEMY_ANI_SPEED_WALK;
			if ((animFrame >> 8) > 1)
			{
				//Wrap back around to frame 0
				animFrame = 0;
			}
		}
	}
	else
	{
		//Enemy is dying so set the animation frame to a dying frame
		if (type == TILE_SNAKE)
		{
			//Snake (616, frame 6)
			BFN_SET(obj->attr2, SPRTILES_ENEMIES + (16 * 6) + ((animFrame >> 8) * 16), ATTR2_ID);
		}
		else
		{
			//Sphinx (744, frame 14)
			BFN_SET(obj->attr2, SPRTILES_ENEMIES + (16 * 14) + ((animFrame >> 8) * 16), ATTR2_ID);
		}
		//Increment anim frame
		animFrame += ENEMY_ANI_SPEED_DIE;
		if ((animFrame >> 8) > 1)
		{
			//Wrap back around to frame 0
			animFrame = 0;
		}
	}

	//If we're dying and (dyingMotionCounter < 0 then reverse
	//the vertical speed, otherwise just decrement dyingMotionCounter
	if ((isDying == true) && (dyingMotionCounter == 0))
	{
		dy = ABS(dy);
		//Set the counter to -1
		dyingMotionCounter--;
	}
	else
	{
		dyingMotionCounter--;
	}

	//Update the position
	x += dx;
	y += dy;

	//Set object coords
	obj_set_pos(obj, x >> 8, y >> 8);

}

void update_enemies(T_LEVELSTATE *ls)
{
	//Loop through ls->mapSprites for  enemy sprites and see if any of the enemy
	//positions have now come within visible range. If so, add the enemy to the visible enemy
	//vector along with it's properties.
	int n;
	int x, y;
	int speed;
	POINT distFromViewport; //Enemy's distance from centre of map viewport

	//Make sure we don't already have too many visible enemies
	if (enemies.size() <= MAX_VISIBLE_ENEMIES)
	{
		for (n = 0; n < ls->totalSprites; n++)
		{
			if (((ls->mapSprites[n].type == TILE_SNAKE) || (ls->mapSprites[n].type == TILE_SPHINX))
			    && (ls->mapSprites[n].visible == false) && (ls->mapSprites[n].available == true))
			{
				//See if it's position is in range
				distFromViewport.x = ls->mapSprites[n].x - (ls->vp.x + 120);
				distFromViewport.y = ls->mapSprites[n].y - (ls->vp.y + 80);
				//Less than 64 pixels off the screen horizontally and vertically
				if ((ABS(distFromViewport.x) < 184) && (ABS(distFromViewport.y) < 144))
				{
					//x and y coordinates must be relative to the screen. Subtract the viewport coords.
					x = ls->mapSprites[n].x - ls->vp.x;
					y = ls->mapSprites[n].y - ls->vp.y;
					//Speed is negative if StartingDir(property 4) is 0, positive if it's 1.
					speed = (ls->mapSprites[n].properties[3] == 0) ? 0 - 1 : 1;
					//Give it some extra speed
					speed = speed << 7;
					enemies.push_back(CEnemy(ls, x, y, speed, 0, ls->mapSprites[n].type,
					                         ls->mapSprites[n].properties[1], ls->mapSprites[n].properties[2],
					                         ls->mapSprites[n].properties[0], ls->mapSprites[n].properties[4], n));
					ls->mapSprites[n].visible = true;
				}
			}
		}
	}

	//Iterate through the visible enemies vector
	for (std::vector<CEnemy>::iterator it = enemies.begin(); it != enemies.end();)
	{
		//Check whether this enemy is active
		if (it->active == true)
		{
			it->Update();
			++it;
		}
		else
		{
			//Mark this enemy's oam index as being free in the levelstate struct.
			it->m_ls->enemyOamIsFree[it->oam_index - ENEMY_FIRST_OAM] = true;

			//Hide the object
			obj_hide(&g_obj_buffer[it->oam_index]);

			it = enemies.erase(it);
		}
	}
}

void scroll_enemies(int x, int y)
{
	//Update the projectiles positions by x and y, which will be got from map.cpp and is the amount
	//the map scrolled on this vbl.
	for (auto& enemy : enemies)
	{
		//Update the coordinates.
		enemy.x -= (x << 8);
		enemy.y -= (y << 8);
	}
}

void reset_enemies(T_LEVELSTATE *ls)
{
	int n;

	enemies.clear();
	for (n = 0; n < MAX_VISIBLE_ENEMIES; n++)
	{
		ls->enemyOamIsFree[n] = true;
		//Hide the object
		obj_hide(&g_obj_buffer[ENEMY_FIRST_OAM + n]);
	}
}

std::vector <CEnemy> &getEnemies()
{
	return enemies;
}
