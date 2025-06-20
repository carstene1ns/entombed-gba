#include "guns.h"

#include <stdlib.h>
#include <tonc.h>
#include <vector>
#include <maxmod.h>

#include "gameDefines.h" //Global game defines
#include "projectiles.h"
#include "utils.h" //Debug print functions
#include "soundbank.h"

std::vector <TGun> guns; //Guns vector

void init_guns(T_LEVELSTATE *ls)
{
	int tile;
	TGun gun;
	//Find all guns initially on the map which have bullets and add
	//them to the guns vector. Guns that have no bullets don't need
	//to be added.
	//NB: Guns may be added or removed during map sequences
	guns.clear();
	for(std::vector<TMapObject>::iterator it = ls->mapObjects.begin(); it != ls->mapObjects.end();)
	{
		tile = ls->mapData[1][(it->y * 80) + it->x].tileIndex;

		if (((tile == TILE_GUN_L) || (tile == TILE_GUN_R) || (tile == TILE_BALLGUN_L)
		  || (tile == TILE_BALLGUN_R)) && (it->properties[0] != 0))
		{
			//Convert from tile coordinates to map coordinates
			gun.xPos = it->x*32; gun.yPos = it->y*16; gun.bullets = it->properties[0];
			gun.interval = it->properties[1]; gun.timer = it->properties[1];
			gun.dir = ((tile == TILE_GUN_L) || (tile == TILE_BALLGUN_L)) ? 0 : 1;
			gun.type = ((tile == TILE_GUN_L) || (tile == TILE_GUN_R)) ? 0 : 1; //0 = blowgun, 1 = ball gun.
			//Set the projectile life if type is 1 (ball gun)
			gun.lifespan = (gun.type == 1) ? it->properties[2] : 0;
			//Set the projectile weight if type is 1 (ball gun)
			gun.weight = (gun.type == 1) ? it->properties[3] : 1;
			guns.push_back(gun);
		}
		++it;
	}
}

void update_guns(T_LEVELSTATE *ls)
{
	POINT distFromViewport; //Gun's distance from centre of map viewport

	for(std::vector<TGun>::iterator it = guns.begin(); it != guns.end();)
	{
		//Check if the gun has any bullets to fire
		if (it->bullets != 0)
		{
			//If the gun is in range of the viewport, reduce it's timer and shoot
			//a bullet if necessary
			distFromViewport.x = it->xPos + ((it->dir == 0) ? 24:8) - (ls->vp.x + 120);
			distFromViewport.y = it->yPos - (ls->vp.y + 80);
			//Within ... pixels off the screen horizontally or vertically
			//TODO: I may need to adjust the range.
			if ((ABS(distFromViewport.x) < 192) && (ABS(distFromViewport.y) < 160))
			{
				it->timer--;

				if (it->timer <= 0)
				{
					it->timer = it->interval;

					//Launch a bullet in the direction the gun is facing
					if (it->dir == 0)
					{
						//Shoot left
						//Shoot the correct type of bullet depending on the gun type, blowgun or ball gun.
						if (it->type == 0)
						{
							add_projectile(ls, it->xPos - ls->vp.x + 23, it->yPos - ls->vp.y + 7,-GUN_BULLET_SPEED,0,2,it->lifespan, 1);
						}
						else
						{
							add_projectile(ls, it->xPos - ls->vp.x + 23, it->yPos - ls->vp.y + 7,-GUN_BULLET_SPEED,0,4,it->lifespan, 1);
						}
						//Play a sound
						mmEffect(SFX_BULLET);
					}
					else
					{
						//Shoot right
						//Shoot the correct type of bullet depending on the gun type, blowgun or ball gun.
						if (it->type == 0)
						{
							add_projectile(ls, it->xPos - ls->vp.x + 9, it->yPos - ls->vp.y + 7,GUN_BULLET_SPEED,0,2,it->lifespan, it->weight);
						}
						else
						{
							add_projectile(ls, it->xPos - ls->vp.x + 9, it->yPos - ls->vp.y + 7,GUN_BULLET_SPEED,0,4,it->lifespan, it->weight);
						}
						//Play a sound
						mmEffect(SFX_BULLET);
					}
					//Reduce bullets if not unimited
					if (it->bullets > 0)
					{
						it->bullets--;
					}
				}
			}
			else
			{
				//Has bullets but is out of range, reset the timer.
				it->timer = it->interval;
			}
		}
		it++;
	}
}

void add_gun(TGun gun)
{
	std::vector<TGun>::iterator it;

	guns.push_back(gun);
	//Set this gun's timer
	guns[guns.end() - it].timer = gun.timer;
}

void delete_gun(int x, int y)
{
	for(std::vector<TGun>::iterator it = guns.begin(); it != guns.end();)
	{
		if ((it->xPos*32 == x) && (it->yPos*16 == y))
		{
			it = guns.erase(it);
		}
		else
		{
			++it;
		}
	}
}
