#include "moving_blocks.h"

//Included code files
#include <stdlib.h>
#include <tonc.h>
#include <vector>

#include "gameDefines.h" //Global game defines
#include "utils.h" //Debug print functions
#include "player.h"
#include "sequences.h"

//Hints: Rocks destroy spikes, hot bricks burn through wood and cool down in water,
//cold bricks heat up in fire. Spikes obstruct hot and cold bricks.

//The blocks are sprites and will have type and starting dir properties,

std::vector <CBlock> blocks; //Visible moving blocks class instance vector

CBlock::CBlock(T_LEVELSTATE* ls, FIXED _x, FIXED _y, int _type, int _startDir, int _lifespan, int _movingDown, int _obj_index)
{
	x = _x<<8; y = _y<<8;
	//Set the initial x and y velocity. This is only required for when a block comes back
	//in range after leaving range. Since blocks can only be initially placed on block
	//boundaries, the block boundary code in the Update method will take care of setting
	//the velocity in most cases.
	dy = _movingDown;
	if (dy > 0)
	{
		dx = 0;
	}
	else
	{
		dx = _startDir;
	}
	//Add a speed multiplier
	dx = (dx << 7); dy = (dy << 7);
	
	lastXDir = _startDir;
	type = _type;
	lifespan = _lifespan;
	lifespanCounter = -1; //This gets set to lifespan after initial movement.
	active = true;
	oam_index = 0;
	obj_index = _obj_index;

	//Pointer to global level state
	m_ls = ls;

	//Find the next free oam entry
	int n;
	bool oamFound;

	n = 0;
	oamFound = false;
	while ((n < MAX_VISIBLE_BLOCKS) && oamFound == false)
	{
		//Check the level state for the next free oam index
		if (ls->blockOamIsFree[n] == true)
		{
			//This oam index is free so add a new projectile to the array at position n,
			//set the levelstate projectile to active and break from the loop.
			oam_index = BLOCK_FIRST_OAM + n;
			ls->blockOamIsFree[n] = false;
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
	 ATTR2_ID(SPRTILES_BLOCKS + (type * 8)));

	//Set object coords
	obj_set_pos(obj, x>>8, y>>8); 
}

CBlock::~CBlock()
{
}

void CBlock::Update()
{
	POINT distFromViewport; //Block's's distance from centre of map viewport
	OBJ_ATTR *obj =  &g_obj_buffer[oam_index];
	POINT tilePos; //Tile position of a block when it's on a tile boundary.
	bool blockedDirs[3]; //Left, right, down
	int i, f;

	//See if the block went out of range, and remove oam if it has

	distFromViewport.x = ((x>>8) + m_ls->vp.x) - (m_ls->vp.x + 120);
	distFromViewport.y = ((y>>8) + m_ls->vp.y) - (m_ls->vp.y + 80);
	//Was 128 pixels off the screen horizontally 64 vertically (248,144) but I
	//increased it to be in accordance with the atari vesrion.
	if ((ABS(distFromViewport.x) > 248) || (ABS(distFromViewport.y) > 160))
	{
		active = false;
		m_ls->mapSprites[obj_index].visible = false;
		if (m_ls->mapSprites[obj_index].available == true)
		{
			//Store the last position, remaining lifespan and direction. Store whether it
			//was falling down in the fourth property. This way if it goes out of range and
			//back in range, we don't have to calculate the downward movement.
			m_ls->mapSprites[obj_index].x = ((x>>8) + m_ls->vp.x);
			m_ls->mapSprites[obj_index].y = ((y>>8) + m_ls->vp.y);
			m_ls->mapSprites[obj_index].properties[1] = lastXDir;
			m_ls->mapSprites[obj_index].properties[2] = lifespanCounter;
			m_ls->mapSprites[obj_index].properties[3] = (dy > 0) ? 1 : 0;
		}
		return;
	}

	//If the block is on a tile boundary then a decision will be made as to it's velocity,
	//it's type and any fixture layer tiles that can be erased will be erased at this point.
	//x and y are relative to the screen, so convert to map coordinates
	if ((((x>>8) + m_ls->vp.x) % 32 == 0) && (((y>>8) + m_ls->vp.y) % 16 == 0))
	{
		//Store the block's tile position (A tile is 32 pixels wide and 16 pixels high)
		tilePos.x = ((x>>8)+ m_ls->vp.x) / 32;
		tilePos.y = ((y>>8)+ m_ls->vp.y) / 16;

		//Erase any fixture layer tiles if necessary
		if (type == BLOCK_ROCK)
		{
			//Rocks erase spikes and spears, which are tiles 5 to 9.
			if ((m_ls->mapData[1][(tilePos.y*80)+tilePos.x].tileIndex >=5) &&
					(m_ls->mapData[1][(tilePos.y*80)+tilePos.x].tileIndex <=9))
			{
				m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
				m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].x = tilePos.x;
				m_ls->mapChanges[m_ls->mapChangeCount].y = tilePos.y;
				m_ls->mapChangeCount++;

				//Add to the erased tiles array at the next free space, if any.
				i = 0;
				while ((m_ls->erasedTiles[i].type > 0) && (i < 19))
				{
					i++;
				}
				//There shouldn't ever be more than 20. On the original levels we only care about 3 or so.
				if (i < 20)
				{
					m_ls->erasedTiles[i].type = m_ls->mapData[1][(tilePos.y*80)+tilePos.x].tileIndex;
					m_ls->erasedTiles[i].pos.x = tilePos.x;
					m_ls->erasedTiles[i].pos.y = tilePos.y;
				}
			}
		}
		if (type == BLOCK_HOT)
		{
			//Hot blocks erase wooden platforms and ladders
			if ((m_ls->mapData[1][(tilePos.y*80)+tilePos.x].tileIndex == TILE_PLATFORM1) ||
				(m_ls->mapData[1][(tilePos.y*80)+tilePos.x].tileIndex == TILE_PLATFORM2) ||
				(m_ls->mapData[1][(tilePos.y*80)+tilePos.x].tileIndex == TILE_LADDER))
			{
				m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
				m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].x = tilePos.x;
				m_ls->mapChanges[m_ls->mapChangeCount].y = tilePos.y;
				m_ls->mapChangeCount++;

				//Add to the erased tiles array at the next free space, if any. Ignore ladders.
				if (m_ls->mapData[1][(tilePos.y*80)+tilePos.x].tileIndex != TILE_LADDER)
				{
					i = 0;
					while ((m_ls->erasedTiles[i].type > 0) && (i < 19))
					{
						i++;
					}
					//There shouldn't ever be more than 20. On the original levels we only care about 3 or so.
					if (i < 20)
					{
						m_ls->erasedTiles[i].type = m_ls->mapData[1][(tilePos.y*80)+tilePos.x].tileIndex;
						m_ls->erasedTiles[i].pos.x = tilePos.x;
						m_ls->erasedTiles[i].pos.y = tilePos.y;
					}
				}
			}
		}

		//Change the block type if necessary if in fire/water
		//Rock turns to hot in fire. Hot turns to wall1 in water. Cold turns to wall1 in fire.
		if (type == BLOCK_ROCK )
		{
			//See if the block entered a fire tile. Turns into hot block if it did.
			if (((m_ls->mapData[1][((tilePos.y)*80)+tilePos.x].tileIndex) == 1) ||
				((m_ls->mapData[1][((tilePos.y)*80)+tilePos.x].tileIndex) == 2))
			{
				//Change the block to the hot type
				type = BLOCK_HOT;
				BFN_SET(obj->attr2, SPRTILES_BLOCKS + (8*1), ATTR2_ID);
			}
		}
		if (type == BLOCK_HOT)
		{	
			//See if the block entered a water tile. Turns into wall1 if it did.
			if (((m_ls->mapData[1][((tilePos.y)*80)+tilePos.x].tileIndex) == 3) ||
				((m_ls->mapData[1][((tilePos.y)*80)+tilePos.x].tileIndex) == 4))
			{
				//Deactivate the block
				active = false;
				//Place a wall and remove the water
				m_ls->mapChanges[m_ls->mapChangeCount].layer = 0;
				m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].x = tilePos.x;
				m_ls->mapChanges[m_ls->mapChangeCount].y = tilePos.y;
				m_ls->mapChangeCount++;
				m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
				m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].x = tilePos.x;
				m_ls->mapChanges[m_ls->mapChangeCount].y = tilePos.y;
				m_ls->mapChangeCount++;
			}
		}
		if (type == BLOCK_COLD)
		{
			//See if the block entered a fire tile. Turns into wall1 if it did.
			if (((m_ls->mapData[1][((tilePos.y)*80)+tilePos.x].tileIndex) == 1) ||
				((m_ls->mapData[1][((tilePos.y)*80)+tilePos.x].tileIndex) == 2))
			{
				//Deactivate the block
				active = false;
				//Place a wall and remove the fire
				m_ls->mapChanges[m_ls->mapChangeCount].layer = 0;
				m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].x = tilePos.x;
				m_ls->mapChanges[m_ls->mapChangeCount].y = tilePos.y;
				m_ls->mapChangeCount++;
				m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
				m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].x = tilePos.x;
				m_ls->mapChanges[m_ls->mapChangeCount].y = tilePos.y;
				m_ls->mapChangeCount++;
			}
		}

		//Set all directions to unblocked initially.
		blockedDirs[0] = false;
		blockedDirs[1] = false;
		blockedDirs[2] = false;

		//Check surrounding walls, left right and down.
		if ((m_ls->mapData[0][(tilePos.y*80)+tilePos.x - 1].tileIndex) > 0)
		{
			blockedDirs[0] = true;
		}
		if ((m_ls->mapData[0][(tilePos.y*80)+tilePos.x + 1].tileIndex) > 0)
		{
			blockedDirs[1] = true;
		}
		if ((m_ls->mapData[0][((tilePos.y + 1)*80)+tilePos.x].tileIndex) > 0)
		{
			blockedDirs[2] = true;
		}

		//If all directions are blocked by walls then set the velocity to 0
		if ((blockedDirs[0] == true) && (blockedDirs[1] == true) && (blockedDirs[2] == true))
		{
			dx = 0;
			dy = 0;
		}
		else
		{
			//Else there's at least one free direction so now we check for fixture layer
			//collisions.

			//Rock and cold blocks are blocked in downward direction by wooden platforms
			if (type != BLOCK_HOT)
			{
				if ( ((m_ls->mapData[1][((tilePos.y + 1)*80)+tilePos.x].tileIndex) == TILE_PLATFORM1)
				  || ((m_ls->mapData[1][((tilePos.y + 1)*80)+tilePos.x].tileIndex) == TILE_PLATFORM2))
				{
					blockedDirs[2] = true;
				}
			}
			//Hot and cold blocks are blocked by spikes/spears
			if (type != BLOCK_ROCK)
			{
				if ( ((m_ls->mapData[1][(tilePos.y*80)+tilePos.x - 1].tileIndex) >= 5) &&
					 ((m_ls->mapData[1][(tilePos.y*80)+tilePos.x - 1].tileIndex) <= 9) )
				{
					blockedDirs[0] = true;
				}
				if (((m_ls->mapData[1][(tilePos.y*80)+tilePos.x + 1].tileIndex) >= 5) &&
					((m_ls->mapData[1][(tilePos.y*80)+tilePos.x + 1].tileIndex) <= 9))
				{
					blockedDirs[1] = true;
				}
				if (((m_ls->mapData[1][((tilePos.y + 1)*80)+tilePos.x].tileIndex) >= 5) &&
					((m_ls->mapData[1][((tilePos.y + 1)*80)+tilePos.x].tileIndex) <= 9))
				{
					blockedDirs[2] = true;
				}
			}
			//Check for collisions with stationary moving blocks below. This allows blocks to
			//be initially stacked upon one another.
			//First check for blocks outside of the visible range, which are stored in the mapSprites array.
			i = 0;
			f = 0; 	//f is a flag to exit the loop early if necessary.
			while ((i < MAX_SPRITES) && f == 0)
			{
				if ( (m_ls->mapSprites[i].type == TILE_BLOCK) && (m_ls->mapSprites[i].visible == false))
				{
					if ((m_ls->mapSprites[i].x == tilePos.x*32) && (m_ls->mapSprites[i].y == (tilePos.y+1)*16))
					{
						f = 1;
						blockedDirs[2] = true;
					}
				}
				i++;
			}
			//Now check the active moving blocks that are within range of the player.
			for(std::vector<CBlock>::iterator it = blocks.begin(); it != blocks.end();it++)
			{
				//Update the coordinates.
				//it->x-=(x<<8); it->y-=(y<<8);
				if ( (((it->x>>8) + m_ls->vp.x) == tilePos.x*32) &&  (((it->y>>8) + m_ls->vp.y) == (tilePos.y+1)*16))
				{
					blockedDirs[2] = true;
				}
			}
		}

		//Set the velocity
		//Set both directions to zero initially
		dx = 0; dy = 0;
		if (blockedDirs[2] == false)
		{
			//Go down
			dx = 0; dy = 1;
		}
		else
		{
			//Set the horizontal speed based on lastXDir and blocked dirs.
			switch (lastXDir)
			{
				case 0: //Left
					//If left is not blocked, go left
					if (blockedDirs[0] == false)
					{
						dx = -1; dy = 0;
					}
					else
					{
						//If right is not blocked, go right and set lastXdir
						if (blockedDirs[1] == false)
						{
							dx = 1; dy = 0;
							lastXDir = 1;
						}
					}
				break;
				case 1: //Right
					//If right is not blocked, go right
					if (blockedDirs[1] == false)
					{
						dx = 1; dy = 0;
					}
					else
					{
						//If left is not blocked, go left and set lastXdir
						if (blockedDirs[0] == false)
						{
							dx = -1; dy = 0;
							lastXDir = 0;
						}
					}
				break;
				default: //Ekse no horizontal movement
					dx = 0;
				break;
			}
		}
		//Add a speed multiplier
		dx = (dx << 7); dy = (dy << 7);
	}

	//If the lifespan counter is 0 then deactivate the block
	if (lifespanCounter == 0)
	{
		active = false;
	}
	//If the lifespanCounter is -1 and the lifespan value is > 0 and
	//the block is moving then set the lifespanCounter to lifespan
	//Else reduce the lifespan counter
	if (lifespanCounter == -1 )
	{
		if ((lifespan > 0) && ((dx != 0) || (dy != 0)))
		{
			lifespanCounter = lifespan;
		}
	}
	else
	{
		lifespanCounter--;
	}

	//Update the position
	x += dx; y += dy;

	//Set object coords
	obj_set_pos(obj, x>>8, y>>8);
}

void update_blocks(T_LEVELSTATE *ls)
{
	int n;
	int x,y;
	POINT distFromViewport; //Block's distance from centre of map viewport

	//Make sure we don't already have too many visible blocks
	if (blocks.size() <= MAX_VISIBLE_BLOCKS)
	{
		for (n=0; n < ls->totalSprites; n++)
		{
			if ((ls->mapSprites[n].type == TILE_BLOCK)
			  && (ls->mapSprites[n].visible == false) && (ls->mapSprites[n].available == true))
			{
				//See if it's position is in range
				distFromViewport.x = ls->mapSprites[n].x - (ls->vp.x + 120);
				distFromViewport.y = ls->mapSprites[n].y - (ls->vp.y + 80);
				//Was 128 pixels off the screen horizontally 64 vertically (248,144) but I
				//increased it to be in accordance with the atari vesrion.
				if ((ABS(distFromViewport.x) < 248) && (ABS(distFromViewport.y) < 160))
				{
					//x and y coordinates must be relative to the screen. Subtract the viewport coords.
					x = ls->mapSprites[n].x - ls->vp.x;
					y = ls->mapSprites[n].y - ls->vp.y;

					//x,y,type,startDir,lifespan, moving down.
					//The initial movement value is obnly used if the block went out of range and
					//then came back in range. It uses the spare property variable [3] and stores.
					//the block's last movement in the lats 2 bits.
					blocks.push_back(CBlock(ls,x,y,ls->mapSprites[n].properties[0],
										    ls->mapSprites[n].properties[1],ls->mapSprites[n].properties[2],
											ls->mapSprites[n].properties[3],n));
					ls->mapSprites[n].visible = true;
				}
			}
		}
	}

	//Iterate through the visible blocks vector
	for(std::vector<CBlock>::iterator it = blocks.begin(); it != blocks.end();)
	{
		//Check whether this block is active
		if (it->active == true)
		{
			it->Update();
			++it;
		}
		else
		{
			//Mark this block's oam index as being free in the levelstate struct.
			it->m_ls->blockOamIsFree[it->oam_index - BLOCK_FIRST_OAM] = true;

			//Hide the object
			obj_hide(&g_obj_buffer[it->oam_index]);

			it = blocks.erase(it);
		}
	}
}

void scroll_blocks(int x, int y)
{
	//Update the moving block positions by x and y, which will be got from map.cpp and is the amount
	//the map scrolled on this vbl.
	for(std::vector<CBlock>::iterator it = blocks.begin(); it != blocks.end();it++)
	{
		//Update the coordinates.
		it->x-=(x<<8); it->y-=(y<<8);
	}
}

void reset_blocks(T_LEVELSTATE *ls)
{
	int n;

	blocks.clear();
	for (n=0;n<MAX_VISIBLE_BLOCKS;n++)
	{
		ls->blockOamIsFree[n] = true;
		//Hide the object
		obj_hide(&g_obj_buffer[BLOCK_FIRST_OAM + n]);
	}
}

std::vector <CBlock>& getBlocks(){return blocks;}
