#include "moving_platforms.h"

//Included code files
#include <stdlib.h>
#include <tonc.h>
#include <vector>
#include "gameDefines.h" //Global game defines
#include "globalvars.h"

std::vector <CPlatform> platforms; //Visible moving platforms class instance vector

CPlatform::CPlatform(T_LEVELSTATE* ls, FIXED _x, FIXED _y, FIXED _dx, FIXED _dy, int _xMin, int _xMax, int _obj_index)
{
	x = _x<<8; y = _y<<8; dx = _dx; dy = _dy; xMin = _xMin; xMax = _xMax;
	active = true;
	oam_index = 0;
	obj_index = _obj_index;
	playerTouchingPlatform = false;

	//Pointer to global level state
	m_ls = ls;


	//Find the next free oam entry
	int n;
	bool oamFound;

	n = 0;
	oamFound = false;
	while ((n < MAX_VISIBLE_PLATFORMS) && oamFound == false)
	{
		//Check the level state for the next free oam index
		if (ls->platformOamIsFree[n] == true)
		{
			//This oam index is free so add a new projectile to the array at position n,
			//set the levelstate projectile to active and break from the loop.
			oam_index = PLATFORM_FIRST_OAM + n;
			ls->platformOamIsFree[n] = false;
			oamFound = true;
		}
		n++;
	}

	OBJ_ATTR *obj = &g_obj_buffer[oam_index];
	obj_set_attr(obj,
	 ATTR0_WIDE |              // Wide, regular sprite
	 ATTR0_4BPP |              //16 colours
	 ATTR0_MODE(0),            //Un-hide the sprite
	 ATTR1_SIZE_32,            // 32x16p
	 ATTR2_PALBANK(0) |
	 ATTR2_PRIO(2) |
	 ATTR2_ID(SPRTILES_PLATFORM));

	//Set object coords
	obj_set_pos(obj, x>>8, y>>8);
}

CPlatform::~CPlatform()
{
}

void CPlatform::Update()
{

	POINT distFromViewport; //Platform's's distance from centre of map viewport
	OBJ_ATTR *obj =  &g_obj_buffer[oam_index];

	//See if the platform went out of range, and remove oam if it has
	distFromViewport.x = ((x>>8) + m_ls->vp.x) - (m_ls->vp.x + 120);
	distFromViewport.y = ((y>>8) + m_ls->vp.y) - (m_ls->vp.y + 80);
	//128 pixels off the screen horizontally 64 vertically
	if ((ABS(distFromViewport.x) > 248) || (ABS(distFromViewport.y) > 144))
	{
		active = false;
		m_ls->mapSprites[obj_index].visible = false;
		if (m_ls->mapSprites[obj_index].available == true)
		{
			//Store the last position and direction
			m_ls->mapSprites[obj_index].x = ((x>>8) + m_ls->vp.x);
			m_ls->mapSprites[obj_index].y = ((y>>8) + m_ls->vp.y);
			m_ls->mapSprites[obj_index].properties[3] = (dx > 0) ? 1 : 0;
		}
	}
	//See if we're at xMin or xMax and need to turn around
	//xMin and xMax are map pixel coords
	//Need to convert platform x coord from relative to screen to relative to map.
	if ( ( ((x>>8) + m_ls->vp.x) <= xMin ) && (dx < 0))
	{
		//Turn right
		dx = ABS(dx); //Set x speed to positive value
	}
	else
	{
		if ((((x>>8) + m_ls->vp.x) >= xMax) && (dx > 0))
		{
			//Turn left
			dx = 0 - dx; //Set x speed to negative value
		}
	}

	//Update the position
	x += dx; y += dy;

	//Set object coords
	obj_set_pos(obj, x>>8, y>>8);

	//If the player is touching this platform, add the
	//platform velocity to the players positon
	if (playerTouchingPlatform)
	{
		m_ls->playerPos.x += dx;

		//Scroll the map if necessary
		//If vp.x is 0 or 2560 then set mapOffsetX to playerSpr.x. This avoids problems
		//when we're at the very edge of the map.
		if ( (m_ls->vp.x == 0) || (m_ls->vp.x == 2560) )
		{
			m_ls->mapOffset.x = m_ls->playerPos.x;
		}
		//If going right
		if (dx > 0)
		{
			if (((m_ls->playerPos.x>>8) - m_ls->vp.x) >= 120)
			{

				m_ls->mapOffset.x += dx;
			}
		}
		else //going left
		{
			if ( ((m_ls->playerPos.x>>8) - m_ls->vp.x) <= 80)
			{
				m_ls->mapOffset.x += dx;
			}
		}
	}
}

void update_platforms(T_LEVELSTATE *ls)
{
	int n;
	int x,y;
	int speed;
	POINT distFromViewport; //Platform's distance from centre of map viewport

	//Make sure we don't already have too many visible platforms
	if (platforms.size() <= MAX_VISIBLE_PLATFORMS)
	{
		for (n=0; n < ls->totalSprites; n++)
		{
			if ((ls->mapSprites[n].type == TILE_MOVING_PLATFORM)
			  && (ls->mapSprites[n].visible == false) && (ls->mapSprites[n].available == true))
			{
				//See if it's position is in range
				distFromViewport.x = ls->mapSprites[n].x - (ls->vp.x + 120);
				distFromViewport.y = ls->mapSprites[n].y - (ls->vp.y + 80);
				//Less than 128 pixels off the screen horizontally, 64 vertically
				if ((ABS(distFromViewport.x) < 248) && (ABS(distFromViewport.y) < 144))
				{
					//x and y coordinates must be relative to the screen. Subtract the viewport coords.
					x = ls->mapSprites[n].x - ls->vp.x;
					y = ls->mapSprites[n].y - ls->vp.y;
					//Speed is negative if StartingDir(property 4) is 0, positive if it's 1.
					//A property 5(Speed) value of 3 is half speed.
					if (ls->mapSprites[n].properties[4] == 3)
					{
						speed = (ls->mapSprites[n].properties[3] == 0) ? 0 - 1 : 1;
						//Give it some extra speed
						speed = speed << 6;
					}
					else
					{
						speed = (ls->mapSprites[n].properties[3] == 0) ? 0 - ls->mapSprites[n].properties[4] : ls->mapSprites[n].properties[4];
						//Give it some extra speed
						speed = speed << 7;
					}
					platforms.push_back(CPlatform(ls,x,y,speed,0,ls->mapSprites[n].properties[1],
							ls->mapSprites[n].properties[2],n));
					ls->mapSprites[n].visible = true;
				}
			}
		}
	}

	//Iterate through the visible platforms vector
	for(std::vector<CPlatform>::iterator it = platforms.begin(); it != platforms.end();)
	{
		//Check whether this enemy is active
		if (it->active == true)
		{
			it->Update();
			++it;
		}
		else
		{
			//Mark this platrform's oam index as being free in the levelstate struct.
			it->m_ls->platformOamIsFree[it->oam_index - PLATFORM_FIRST_OAM] = true;

			//Hide the object
			obj_hide(&g_obj_buffer[it->oam_index]);

			it = platforms.erase(it);
		}
	}
}

void scroll_platforms(int x, int y)
{
	//Update the moving platform positions by x and y, which will be got from map.cpp and is the amount
	//the map scrolled on this vbl.
	for(std::vector<CPlatform>::iterator it = platforms.begin(); it != platforms.end();it++)
	{
		//Update the coordinates.
		it->x-=(x<<8); it->y-=(y<<8);
	}
}

void reset_platforms(T_LEVELSTATE *ls)
{
	int n;

	platforms.clear();
	for (n=0;n<MAX_VISIBLE_PLATFORMS;n++)
	{
		ls->platformOamIsFree[n] = true;
		//Hide the object
		obj_hide(&g_obj_buffer[PLATFORM_FIRST_OAM + n]);
	}
}

std::vector <CPlatform>& getPlatforms(){return platforms;}
