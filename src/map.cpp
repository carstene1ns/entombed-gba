#include "map.h"

//Included code files
#include <string.h>
#include <tonc.h>
#include <maxmod.h>

#include "gameDefines.h" //Global game defines
#include "globalvars.h"
#include "text.h"
#include "fade.h"
#include "itoa.h"
#include "enemies.h"
#include "projectiles.h"
#include "moving_platforms.h"
#include "moving_blocks.h"
#include "soundbank.h"

//Graphics data
#include "tilemap_walls_gfx.h"
#include "tilemap_fixtures_gfx.h"

//Level data
#include "level_data.h"

//Lookup table for layer 2 tile mappings
#include "layer2tileLUT.h"

//Lookup table for object property counts
#include "objPropertiesLUT.h"

//******CMap class function implementations******
//Public
CMap::CMap()
{
    m_ls = 0;
    //m_paletteBuffer = 0;
    m_tileAnimCounter = 0;
    m_tileAnimFrame = 0;
    m_urnFlashCount = -1;
    m_urnBreakFrame = -1;
    m_urnHitPoints = 0;
}

CMap::~CMap()
{
    //Do deconstructor stuff
}

void CMap::Init(T_LEVELSTATE *ls)
{
	
    //Initialise non-changing member properties
    //Set local levelstate property as pointer to the global levelstate struct
    m_ls = ls;

    m_ls->vp.xmin = 0;
    m_ls->vp.xmax = 2560;
    m_ls->vp.xpage = 240;
    m_ls->vp.ymin = 0;
    m_ls->vp.ymax = 784;
    m_ls->vp.ypage = 160;

    //Set the variable for the last urn that was hit with an arrow.
    m_ls->urnWasHit.x = -1; m_ls->urnWasHit.y = -1;

    //Call the ResetMap function to set the dynamic member variables
    ResetMap();
	
	//Load the background graphics data
	//Use memcpy16 here, because if you use memcpy you get a bug which is probably
    //to do with having the wrong byte alignment. See tonc 5.4.6. for details.
    //Load the wall tiles that correspond to the map property value.
    //TODO: Possibly use GRIT_CPY instead
    memcpy16(tile_mem[0], &tilemap_walls_gfx[m_ls->wallTiles * 1024 * sizeof(u16)], 1024);
	GFX_CPY(tile_mem[1], tilemap_fixtures);

	//Initialise the text system (background 0, SBB 26, prio 0)
	txt_init(0, 26, 0); //Text

	//Set the display register
	REG_DISPCNT= DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;

	//Set the hourglass digit oam data, but set to hidden. We can do this because
	//these sprites will never change position and can have the tile and hidden
	//status changed when needed
	obj_set_attr(&g_obj_buffer[HOURGLASS_DIGIT_OAM],
				ATTR0_SQUARE | ATTR0_4BPP | ATTR0_HIDE, ATTR1_SIZE_16, ATTR2_PALBANK(0)
				| ATTR2_PRIO(0) | ATTR2_ID(SPRTILES_DIGITS)); //Digit 0, hidden
	obj_set_pos(&g_obj_buffer[HOURGLASS_DIGIT_OAM],24,128);
	obj_set_attr(&g_obj_buffer[HOURGLASS_DIGIT_OAM + 1],
				ATTR0_SQUARE | ATTR0_4BPP | ATTR0_HIDE, ATTR1_SIZE_16, ATTR2_PALBANK(0)
				| ATTR2_PRIO(0) | ATTR2_ID(SPRTILES_DIGITS)); //Digit 0, hidden
	obj_set_pos(&g_obj_buffer[HOURGLASS_DIGIT_OAM + 1],40,128);


}

void CMap::Update()
{
    int i;
    OBJ_ATTR *obj= &g_obj_buffer[URN_OAM]; //Breaking urn sprite object

    //Centre the map on the mapOffset
    vp_center(&m_ls->vp, (m_ls->mapOffset.x>>8), m_ls->mapOffset.y>>8);
	
	//Display the score
	//First display six zeros
	txt_puts(8, 144, "000000");
    //Display the score right justified
	itoa(g_score, m_str, 10);
    txt_puts(8 + (48 - (strlen(m_str) * 8)), 144, m_str);
    
    //Display lives
    for (i = 0; i < g_lives; i++)
    {
        txt_putc(8 + (i * 8), 152, 96 + 32); //Life tile
    }
    
    //Display bows
    for (i = 0; i < m_ls->bows; i++)
    {
        txt_putc(80 + (i * 8), 144, 97 + 32); //Bow tile
    }
    
    //Clear the whole arrow area (I could change this in future to clear a single
    //arrow character upon firing the bow)
    //txt_puts(64, 152, "          ");
    //Display arrows
    for (i = 0; i < m_ls->arrows; i++)
    {
        txt_putc(64 + (i * 8), 152, 98 + 32); //Arrow tile
    }
    
    //Display keys
    for (i = 0; i < m_ls->keys; i++)
    {
        txt_putc(144 + (i * 8), 144, 99 + 32); //Key tile
    }
    
    //Display selected seconds
    //First display two zeros and a forward slash
	txt_puts(192, 144, "00/");
    //Display the total seconds right justified
	itoa(m_ls->selectedSeconds, m_str, 10);
    txt_puts(192 + (16 - (strlen(m_str) * 8)), 144, m_str);
    
    //Display total seconds
    //First display two zeros
	txt_puts(216, 144, "00");
    //Display the total seconds right justified
	itoa(m_ls->seconds, m_str, 10);
    txt_puts(216 + (16 - (strlen(m_str) * 8)), 144, m_str);
	
    //If an urn has already been hit(flashing or breaking), process it here.
    //m_urnBreakFrame is fixed point.
    if (m_urnBreakFrame > -1)
    {
    	//Are we at the end of the animation? (Past the 4th frame)
    	if ((m_urnBreakFrame>>8) >= 3)
    	{
    		//Remove the breaking urn sprite and reset m_urnBreakFrame
    		//and  m_urnPos
			//Set the object attributes to hidden
    		obj_hide(obj);

    		m_urnBreakFrame = -1;
    		m_urnPos.x = -1;
    		m_urnPos.y = -1;
    	}
    	else
    	{
    		//The urn is still in the process of breaking

        	//Make sure the urn is in the correct position in case the
    		//map has scrolled
    		obj_set_pos(obj,(m_urnPos.x*32) - m_ls->vp.x,(m_urnPos.y*16) - m_ls->vp.y);

    		//Save the current animation frame value in i
    		i = m_urnBreakFrame >> 8;
    		m_urnBreakFrame +=  URN_ANIM_SPEED;
    		//If it advanced to a new frame, update the OAM tile attribute
    		if ((m_urnBreakFrame >> 8) > i)
    		{
    			BFN_SET(obj->attr2, SPRTILES_URN + ((m_urnBreakFrame >> 8) * 16), ATTR2_ID);
    		}
    	}
    }
    else
    {
    	//See if an urn is currently flashing
    	if (m_urnFlashCount > -1)
    	{
    		//Reduce the flash counter
    		m_urnFlashCount--;

    		//If the flash counter is now < 0, change
    		//the urn back to normal
    		if (m_urnFlashCount < 0)
    		{
    			//Top half
				m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = TILE_URN_TOP;
				m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].x = m_urnPos.x;
				m_ls->mapChanges[m_ls->mapChangeCount].y = m_urnPos.y;
				m_ls->mapChangeCount++;
				//Bottom half
				m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = TILE_URN_BOTTOM;
				m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].x = m_urnPos.x;
				m_ls->mapChanges[m_ls->mapChangeCount].y = m_urnPos.y + 1;
				m_ls->mapChangeCount++;

				//If the urn has no hitpoints then call the UrnBroke method
				//Otherwise reset the urn position variables
				if (m_urnHitPoints <= 0)
				{
					UrnBroke();
				}
				else
				{
					//Reset m_urnPos
					m_urnPos.x = -1;
					m_urnPos.y = -1;
				}
    		}
    	}
    }

	//If an urn has just been hit, call the UrnHit method
    //Just check one coordinate.
    if (m_ls->urnWasHit.x >= 0)
    {
    	UrnHit();
    	//Play a sound (Same as the title screen bounce sound)
    	mmEffect(SFX_TITLE);
    }

    //Update any coin score sprites that are currently being shown
    for (i = 0; i < MAX_COIN_SCORE_SPRITES; i++)
    {
    	if (m_ls->coinScoreSprites[i].type > -1)
    	{
    		if ((m_ls->coinScoreSprites[i].delay) > 0)
    		{
    			//Move the coin up.
    			m_ls->coinScoreSprites[i].y -= COIN_SCORE_SPEED;

    			//Position it relative to the viewport
				obj_set_pos(&g_obj_buffer[COIN_SCORE_OAM + i],(m_ls->coinScoreSprites[i].x) - m_ls->vp.x,
						   ((m_ls->coinScoreSprites[i].y>>8) - m_ls->vp.y));
				m_ls->coinScoreSprites[i].delay--;
    		}
    		else
    		{
    			//Remove the coin sprite
    			obj_hide(&g_obj_buffer[COIN_SCORE_OAM + i]);
    			m_ls->coinScoreSprites[i].type = -1;
    		}
    	}
    }

    //Update the hourglass digits if necessary
    //This assumes the maximum seconds is 10.
    if (m_ls->secondsCounter >= 0)
    {
    	if (m_ls->secondsCounter == 0)
    	{
    		m_ls->currentSecond--;

    		if (m_ls->currentSecond == 0)
    		{
    			//Add 10 frames to secondCounter so we can show
    			//00 seconds on screen for a short time.
    			m_ls->secondsCounter = 10;
    			//Display 00
    			BFN_SET(g_obj_buffer[HOURGLASS_DIGIT_OAM].attr2, SPRTILES_DIGITS, ATTR2_ID);
    			BFN_SET(g_obj_buffer[HOURGLASS_DIGIT_OAM + 1].attr2, SPRTILES_DIGITS, ATTR2_ID);
    		}
    		else
    		{
    			if (m_ls->currentSecond < 0)
    			{
    				//Hide the digits
    				obj_hide(&g_obj_buffer[HOURGLASS_DIGIT_OAM]);
    				obj_hide(&g_obj_buffer[HOURGLASS_DIGIT_OAM+1]);
    				m_ls->secondsCounter--; //Set to -1
    			}
    			else
    			{
    				//Else seconds are 1 or more so update them and reset secondsCounter
    				BFN_SET(g_obj_buffer[HOURGLASS_DIGIT_OAM].attr2, SPRTILES_DIGITS, ATTR2_ID); // "0"
    				BFN_SET(g_obj_buffer[HOURGLASS_DIGIT_OAM + 1].attr2, SPRTILES_DIGITS + (4 * m_ls->currentSecond), ATTR2_ID);
    				m_ls->secondsCounter = 60;
    			}
    		}
    	}
    	else
    	{
    		m_ls->secondsCounter--;
    	}
    }

	//Call the processMapChanges function if there are any changes to process
    if (m_ls->mapChangeCount > 0)
    {
	   processMapChanges();
    }

    //Update both map layers
	bgt_update(&m_ls->bg[0], &m_ls->vp);
	bgt_update(&m_ls->bg[1], &m_ls->vp);
	
	bgt_animate(); //Update the tile animation frame and redraw any animated tiles in
    //the map memory

}


void CMap::vp_center(VIEWPORT *vp, int x, int y)
{	vp_set_pos(vp, x - vp->xpage/2, y - vp->ypage/2);	}


void CMap::vp_set_pos(VIEWPORT *vp, int x, int y)
{
	//Save the old x and y positions, then center the viewport
	int oldX = vp->x; int oldY = vp->y;

	vp->x= clamp(x, vp->xmin, vp->xmax - vp->xpage);
	vp->y= clamp(y, vp->ymin, vp->ymax - vp->ypage);

	//If vp->x or vp->y are different to the old vp->x and vp->y, move all on-screen
	//sprites by the difference. This way they move with the scrolling map.
	if ((oldX != vp->x) || (oldY != vp->y))
	{
		scroll_projectiles(vp->x-oldX,vp->y-oldY);

		//Scroll the enemies
		scroll_enemies(vp->x-oldX,vp->y-oldY);

		//Scroll the moving platforms
		scroll_platforms(vp->x-oldX,vp->y-oldY);

		//Scroll the moving blocks
		scroll_blocks(vp->x-oldX,vp->y-oldY);
	}
}


void CMap::bgt_init(BGTYPE *bgt, int bgnr, u32 ctrl,
	const unsigned char *map, u32 map_width, u32 map_height, u32 layer)
{
	memset(bgt, 0, sizeof(BGTYPE));
	

	bgt->flags= bgnr;
	bgt->cnt= ctrl;
	bgt->dst_map= se_mem[BFN_GET(ctrl, BG_SBB)];
	
	//Set map coords to initial viewpoint coords
	bgt->map_x = m_ls->vp.x;
    bgt->map_y = m_ls->vp.y;
    
	REG_BGCNT[bgnr]= ctrl;
	
	//Set scroll offset to viewpoint coordinates
	REG_BG_OFS[bgnr].x= m_ls->vp.x;
	REG_BG_OFS[bgnr].y= m_ls->vp.y;

	bgt->src_map_width= map_width;
	bgt->src_map_height= map_height;
	bgt->layer = layer;

	int ix, iy;
	
	//Setup the map data for the layer (Layer is bgnr - 1 so make sure layer0 is set to
	//background 1 and layer 2 to background 2)
	for (iy=0; iy<48; iy++)
	{
		for(ix=0; ix<80; ix++)
		{
			//Read the map data starting from the correct position based on the levelnum value
			m_ls->mapData[bgnr - 1][(iy*80)+ix].tileIndex = map[(m_ls->levelNum*3840) + ((iy*80)+ix)];
			m_ls->mapData[bgnr - 1][(iy*80)+ix].visibleTileIndex = map[(m_ls->levelNum*3840) + ((iy*80)+ix)];

            //If layer 2 then check if we need to add bottom section tiles at a given
            //location
            if ((bgnr == 2) & (iy > 0))
            {
                switch (map[(m_ls->levelNum*3840) + ((iy - 1)*80)+ix])
                {
                    case TILE_DOOR_CLOSED_TOP: //Door(Closed) top
                        m_ls->mapData[bgnr - 1][(iy*80)+ix].tileIndex = TILE_DOOR_CLOSED_BOTTOM;
                        m_ls->mapData[bgnr - 1][(iy*80)+ix].visibleTileIndex = TILE_DOOR_CLOSED_BOTTOM;
                        break;
                    case TILE_DOOR_OPEN_TOP: //Door(Open) top (Used only one time on level 5 only)
						m_ls->mapData[bgnr - 1][(iy*80)+ix].tileIndex = TILE_DOOR_OPEN_BOTTOM;
						m_ls->mapData[bgnr - 1][(iy*80)+ix].visibleTileIndex = TILE_DOOR_OPEN_BOTTOM;
						break;
                    case TILE_URN_TOP: //Urn top
                        m_ls->mapData[bgnr - 1][(iy*80)+ix].tileIndex = TILE_URN_BOTTOM;
                        m_ls->mapData[bgnr - 1][(iy*80)+ix].visibleTileIndex = TILE_URN_BOTTOM;
                        break;
                    case TILE_EXIT_TOP: //Exit top
                        m_ls->mapData[bgnr - 1][(iy*80)+ix].tileIndex = TILE_EXIT_BOTTOM;
                        m_ls->mapData[bgnr - 1][(iy*80)+ix].visibleTileIndex = TILE_EXIT_BOTTOM;
                        break;
                }
            }
        }
    }
}

//Function to return a rectangle of 8 by 8 GBA tiles when given an x and y GBA map coordinate
//i.e. Converts 32p x 16p blocks or parts thereof into the correct 8p x 8p tiles.
void CMap::getGBATiles(int left, int top, int width, int height, int layer)
{

    int bx, bxStart; //Which block column we are starting from
    int by; //Which block row we are starting from

    int bxx, bxxStart; //Block Row X, which quarter of the block in X direction are we starting from, 0, 1, 2 or 3
    int byy; //Block Column Y, which half of the block in Y direction are we starting in, 0 for top, 1 for bottom

    int ix, iy; //Loop counters
    int addShadowing;

    SCR_ENTRY *dstL = &m_tileList[0];

    bx = left>>2; //Divide the tile position by 4
    bxStart = bx; //Set bxStart to bx so we can reset bx to this after every row of tiles
    
    by = top>>1; //Divide the tile position by 2

    bxx = left % 4;
    byy = top % 2;
    bxxStart = bxx;


    //Get the tiles
    for (iy = 0; iy < height; iy++)
    {
        for (ix = 0; ix < width; ix++)
        {
            if (layer == 0)
            {
					*dstL++ = (m_ls->mapData[layer][(by*80+bx)].visibleTileIndex * 8) + bxx + (byy * 4)
					+ SE_PALBANK(0 + m_ls->shadows[(by*80+bx)]);
            }
            else
            {

                //If the layer 2 tile is a ladder or the top of an open
                //door then add shadowing if necessary
                if (( m_ls->shadows[(by*80+bx)] == 1)  &&
                ((m_ls->mapData[layer][(by*80+bx)].visibleTileIndex == TILE_LADDER) ||
                (m_ls->mapData[layer][(by*80+bx)].visibleTileIndex == TILE_DOOR_OPEN_TOP)) )
                {
                    addShadowing = 1;
                }
                else
                {
                    addShadowing = 0;
                }
                
                //If the last tile was an animated tile then we must synchronise it with
                // the current tile animation frame, otherwise it'll disappear from
                //the screen
                switch (m_ls->mapData[layer][(by*80+bx)].visibleTileIndex)
                {
                    case 1: //FireTop
                    case 2: //FireBottom
                    case 3: //WaterTop
                    case 4: //WaterBottom
                    case 9: //Spears
                    case 12: //Coin
                    case 31: //Teleporter
                        //Add the correct tile offset based on m_titleAnimFrame
                        *dstL++ = (layer2tileLUT[m_ls->mapData[layer][(by*80+bx)].visibleTileIndex])
                        + bxx + (byy * 4) + (m_tileAnimFrame * 8)
                         + SE_PALBANK(0 + addShadowing);
                    break;
               
                    default:
                        //Otherwise just add the tile as normal
                        *dstL++ = (layer2tileLUT[m_ls->mapData[layer][(by*80+bx)].visibleTileIndex])
                         + bxx + (byy * 4) + SE_PALBANK(0 + addShadowing);
                    break;
                }
            }
            //Increment bxx, and bx if necessary
            bxx++;
            if (bxx > 3)
            {
                bxx = 0;
                bx++;
            }
        }
        //Reset bxx and bx, ready to write the next row down (If there is one)
        bxx = bxxStart;
        bx = bxStart;
        
        //Increment byy, and by if necessary
        byy++;
        if (byy > 1)
        {
            byy = 0;
            by++;
        }
    }
}

void CMap::bgt_colcpy(BGTYPE *bgt, int tx, int ty)
{
	int iy, y0= ty&31;

    SCR_ENTRY *srcL= &m_tileList[0];
	SCR_ENTRY *dstL= &bgt->dst_map[y0*32 + (tx&31)];
	
    //Get a column of tiles
    getGBATiles(tx, ty, 1, 32, bgt->layer);

	for(iy=y0; iy<32; iy++)
	
	{	*dstL= *srcL++;	dstL += 32;	}

	dstL -= 1024;

	for(iy=0; iy<y0; iy++)
	{	*dstL= *srcL++;	dstL += 32;	}			
}

void CMap::bgt_rowcpy(BGTYPE *bgt, int tx, int ty)
{
    
    int ix, x0= tx&31;

    //Get a row of tiles 
    getGBATiles(tx, ty, 32, 1, bgt->layer);

	SCR_ENTRY *srcL= &m_tileList[0];
	SCR_ENTRY *dstL= &bgt->dst_map[(ty&31)*32 + x0];

	for(ix=x0; ix<32; ix++)
		*dstL++= *srcL++;

	dstL -= 32;

	for(ix=0; ix<x0; ix++)
		*dstL++= *srcL++;
}

void CMap::bgt_update(BGTYPE *bgt, VIEWPORT *vp)
{
	// Pixel coords
	int vx= vp->x, vy= vp->y;
	int bx= bgt->map_x, by= bgt->map_y;

	// Tile coords
	int tvx= vx>>3, tvy= vy>>3;
	int tbx= bx>>3, tby= by>>3;

	// Basically, we need another row/col when the viewport
	// exposes another row/col, i.e., if the tile coords
	// have changed

	if(tvx < tbx)		// add on left
		bgt_colcpy(bgt, tvx, tvy);
	else if(tvx > tbx)	// add on right
		bgt_colcpy(bgt, tvx+31, tvy);

	if(tvy < tby)		// add on top
		bgt_rowcpy(bgt, tvx, tvy);
	else if(tvy > tby)	// add on bottom
		bgt_rowcpy(bgt, tvx, tvy+31);

	// Update BGTYPE and reg-offsets
	int bgnr= bgt->flags;
	
	//Update scroll registers
    REG_BG_OFS[bgnr].x= bgt->map_x= vx;
	REG_BG_OFS[bgnr].y= bgt->map_y= vy;
}

void CMap::bgt_animate()
{
    int ix, iy;
    
    SCR_ENTRY *dst= m_ls->bg[1].dst_map;
    
    //Update the animation counter
    m_tileAnimCounter++;
    
    //Only update the animation frame every 8 frames
    if (m_tileAnimCounter == 8)
    {
        //Reset the animation counter
        m_tileAnimCounter = 0;
        
        //Go to the next tile animation frame
        m_tileAnimFrame++;
        if (m_tileAnimFrame == 4)
        {
            m_tileAnimFrame = 0;
        }

        //Loop through all 8x8 map tiles to find which ones need changing
        for (iy = 0; iy < 32; iy++)
        {
            for (ix = 0; ix < 32; ix++)
            {

                //Fire and water (Tiles 1 to 5, past first blank tile)
            	if (((BFN_GET(dst[(iy*32)+ix], SE_ID) >= 8 ) &&
				(BFN_GET(dst[(iy*32)+ix], SE_ID) < layer2tileLUT[5])) ||
                //Spears (Tile 10)
                ((BFN_GET(dst[(iy*32)+ix], SE_ID) >= layer2tileLUT[9] ) &&
                (BFN_GET(dst[(iy*32)+ix], SE_ID) < layer2tileLUT[9]+32 )) ||
                //Coin(Tile 12)
                ((BFN_GET(dst[(iy*32)+ix], SE_ID) >= layer2tileLUT[12] ) &&
                (BFN_GET(dst[(iy*32)+ix], SE_ID) < layer2tileLUT[12]+32 )) ||
				//Teleporter(Tile 29)
				((BFN_GET(dst[(iy*32)+ix], SE_ID) >= layer2tileLUT[31] ) &&
				(BFN_GET(dst[(iy*32)+ix], SE_ID) < layer2tileLUT[31]+32 )))
                {
                    //If m_tileAnimFrame is above 0 then add 8 to the tile, otherwise
                    //subtract 24
                    if (m_tileAnimFrame > 0)
                    {
                        dst[(iy*32)+ix] += 8;
                    }
                     else
                    {
                        dst[(iy*32)+ix] -= 24;
                    }
                }
            }
        }
    }
}

void CMap::processMapChanges()
{
    int i;
    int ix, iy;

    //Loop from 0 to mapChangeCount
    for (i = 0; i < m_ls->mapChangeCount; i++)
    {
        //Reset the shadow value for the block
        m_ls->shadows[(m_ls->mapChanges[i].y * 80) + m_ls->mapChanges[i].x] = 0;
        
        //Update the map data
        m_ls->mapData[m_ls->mapChanges[i].layer]
        [(m_ls->mapChanges[i].y * 80) + m_ls->mapChanges[i].x].visibleTileIndex
          = m_ls->mapChanges[i].tileIndex;

        //If it's a solid change then set the tile index as well
        if (m_ls->mapChanges[i].solid == 1)
        {
            m_ls->mapData[m_ls->mapChanges[i].layer]
             [(m_ls->mapChanges[i].y * 80) + m_ls->mapChanges[i].x].tileIndex
              = m_ls->mapChanges[i].tileIndex;
        }
        
        //Check if there's now a shadow on this block or the blocks around it
        CalculateShadows(m_ls->mapChanges[i].x, m_ls->mapChanges[i].y, 2, 2);

        //Redraw changed block(s). If change affects layer 0 then redraw the three
        //blocks to the down and right and down-right as well if they're visible
        if (m_ls->mapChanges[i].layer == 0)
        {
            for (iy = m_ls->mapChanges[i].y; iy < m_ls->mapChanges[i].y + 2; iy++)
            {
                for (ix = m_ls->mapChanges[i].x; ix < m_ls->mapChanges[i].x + 2; ix++)
                {
                    //Redraw first layer
                    RedrawBlock(ix, iy, 0);
                    
                    //Redraw second layer for these blocks if shadows affect them
                    //(i.e. they're a ladder or top of an open door)
                    if ((m_ls->mapData[1] [(m_ls->mapChanges[i].y * 80)
                     + m_ls->mapChanges[i].x].visibleTileIndex == TILE_LADDER) ||
                      (m_ls->mapData[1] [(m_ls->mapChanges[i].y * 80)
                     + m_ls->mapChanges[i].x].visibleTileIndex == TILE_DOOR_OPEN_TOP))
                    {
                        RedrawBlock(ix, iy, 1);
                    }
                }
            }
        }
        else
        {
            //Just redraw second layer of this block
            RedrawBlock(m_ls->mapChanges[i].x, m_ls->mapChanges[i].y, 1);

            //Certain layer 2 tiles cast a shadow, so also update layer 0
            //in thes cases.
        	if ((m_ls->mapChanges[i].tileIndex = TILE_PLATFORM1) ||
			    (m_ls->mapChanges[i].tileIndex = TILE_PLATFORM2)  ||
			    (m_ls->mapChanges[i].tileIndex = TILE_DOOR_CLOSED_TOP)  ||
			    (m_ls->mapChanges[i].tileIndex = TILE_DOOR_CLOSED_BOTTOM) ||
			    (m_ls->mapChanges[i].tileIndex = TILE_WOOD_FRAME_TOP) ||
			    (m_ls->mapChanges[i].tileIndex = TILE_WOOD_FRAME))
        	{
        		RedrawBlock(m_ls->mapChanges[i].x, m_ls->mapChanges[i].y, 0);
        	}
        }
    }
    
    //Reset m_ls->mapChangeCount so it's ready for the next frame
    m_ls->mapChangeCount = 0;
}

void CMap::RedrawBlock(int x, int y, int layer)
{
    int ix;
    int hTileOffset,vTileOffset;
    int tx, ty;
    int tileCount;
    bool isOnMap;
    int left, top, width, height; //The position of the tiles within the block that are
                                  //within the map in memory. (e.g. left=0, right=4,
                                  //top=0, bottom=1 means the full tile needs to be
                                  //redrawn

    //Check if any part of the block is within the 32t x 32t map memory
    isOnMap = false;
    left = -1;
    top = -1;
    width = 0;
    height = 0;
    hTileOffset = 0;
    vTileOffset = 0;

    //Check x coordinate
    //Subtract the block's rightmost tile coordinate from the viewport - 8
    //If it's above 0 then it's in view on the left

    //Attempt to get block.mapMem_tx (map starts from 1 tile left of the visible
    //viewport vp.x
    left = (x<<2) - (m_ls->vp.x >> 3);


    //Check whether it actually could be in the current map memory
    if ((left >= -3) && (left <= 31))
    {

        //Check if it's partially off of the map (Horizontally)
        if (left < 0)
        {
            width = 4 + left;
            left += (4 - width); //We now want the right of part of a block, rather
                                     //than the whole block
            hTileOffset = 4 - width;

        }
        else if (left > 28)
        {
            width = 32 - left;
            //left += (4 - width); //We now want the left of part of a block, rather
                                     //than the whole block
        }
        else
        {
            width = 4;
        }

        //Now we've found that the block is in the map memory and we've found it's
        //width, we need to find the ACTUAL location in the map memory, since
        //the left side of the map might be in the middle of the map memory etc..
        left += ((m_ls->vp.x >> 3) & 31);

        //Attempt to get block.mamMem_ty map starts 6 tiles above the visible viewport
        //vp.y
        top = (y<<1) - (m_ls->vp.y >> 3);

        //Check whether it actually is in the current map memory
        if ((top >= -1) && (top <= 31))
        {
            //Now we know for sure that the block is (at least partially) within
            //the map memory
            isOnMap = true;

            //Check if it's partially off the the map (Vertically)
            if (top == -1)
            {
                height = 1;
                top += 1; //We now want the top of part of a block, rather
                          //than the top of a whole block
                vTileOffset = 1;
            }
            else if (top == 31 )
            {
                height = 1;
                top -= 1; //We now want the top of part of a block, rather
                          //than the top of a whole block

            }
            else
            {
                height = 2;
            }
            //Now we've found that the block is in the map memory and we've found it's
            //height, we need to find the ACTUAL location in the map memory, since
            //the top edge of the map might be in the middle of the map memory etc..
            top += ((m_ls->vp.y >> 3) & 31);
        }
    }
        
    if (isOnMap == true)
    {
    	getGBATiles((x << 2) + hTileOffset, (y << 1) + vTileOffset, width, height,layer);

        tileCount = 0;
        tx = left&31; //left needs to be & 31'd again
        ty = top&31; //top needs to be & 31'd again
        SCR_ENTRY *dst= m_ls->bg[layer].dst_map;



        //Draw the first row
        for (ix = 0; ix < width; ix++)
        {
            dst[(ty*32)+tx] = m_tileList[tileCount];
            tileCount++;
            tx++;
            tx &= 31;
        }
        //Draw the second row if there is one
        if (height == 2)
        {
            tx = left&31;
            ty++;
            ty &= 31;

            for (ix = 0; ix < width; ix++)
            {
                dst[(ty*32)+tx] = m_tileList[tileCount];
                tileCount++;
                tx++;
                tx &= 31;
            }
        }
    }
}


void CMap::CalculateShadows(int left, int top, int width, int height)
{
    int ix, iy;
    //Any map position that has an empty wall tile could have a shadow if one of the
    //following conditions are met
    //- A wall block is above, above and left, or left of the map position
    //- A closed door block is in the position
    //- A static platform is in the position
    
    //There are only two layer two blocks that can be shadowed. They are: the top of an
    //open door and a ladder
    

    for (iy = top; iy < top + height; iy++)
    {
        for (ix = left; ix < left + width; ix++)
        {
            //First check that the block has no wall on it
            if (m_ls->mapData[0][(iy * 80) + ix].visibleTileIndex == 0)
            {
                //Check for walls that would cause a shadow in this position
                if ((m_ls->mapData[0][((iy - 1) * 80) + ix].visibleTileIndex > 0) ||
                    (m_ls->mapData[0][((iy - 1) * 80) + (ix - 1)].visibleTileIndex > 0) ||
                     (m_ls->mapData[0][(iy * 80) + (ix - 1)].visibleTileIndex > 0))
                {
                    m_ls->shadows[(iy * 80) + ix] = 1;
                }
                //Check for a block behind a door
                else if ((m_ls->mapData[1][(iy * 80) + ix].visibleTileIndex == TILE_DOOR_CLOSED_TOP) ||
                (m_ls->mapData[1][(iy * 80) + ix].visibleTileIndex == TILE_DOOR_CLOSED_BOTTOM))
                {
                    m_ls->shadows[(iy * 80) + ix] = 1;
                }
                //Check for objects that cast a shadow (Platforms, wooden frames)
                else if ((m_ls->mapData[1][(iy * 80) + ix].visibleTileIndex == TILE_PLATFORM1) ||
                	    (m_ls->mapData[1][(iy * 80) + ix].visibleTileIndex == TILE_PLATFORM2) ||
						(m_ls->mapData[1][(iy * 80) + ix].visibleTileIndex == TILE_WOOD_FRAME_TOP) ||
						(m_ls->mapData[1][(iy * 80) + ix].visibleTileIndex == TILE_WOOD_FRAME))
                {
                    m_ls->shadows[(iy * 80) + ix] = 1;
                }
                else
                {
                    m_ls->shadows[(iy * 80) + ix] = 0;
                }
            }
            else
            {
                m_ls->shadows[(iy * 80) + ix] = 0;
            }
        }
    }
    
}

void CMap::ResetMap()
{
	int n;
    int ix, iy;

    m_ls->vp.x = m_ls->playerPos.x>>8; //Start position(Get from map data)
    m_ls->vp.y = m_ls->playerPos.y>>8; //Start position(Get from map data)
    m_ls->mapChangeCount = 0; //No map tile changes yet
    m_tileAnimFrame = 0;
    m_tileAnimCounter = 0;

    //reset urn variables
    m_urnPos.x = -1;
    m_urnPos.y = -1;
    m_urnFlashCount = -1;
    m_urnBreakFrame = -1;
    m_urnHitPoints = 0;

    //Reset the erased tile array for any tiles erased by moving blocks. (Just set types to 0/0
    for (n = 0; n < 20; n++) { m_ls->erasedTiles->type = 0;}

	//Centre the map on the player
	vp_center(&m_ls->vp, m_ls->playerPos.x>>8, m_ls->playerPos.y>>8);
	
	//Setup the wall and fixture layer tiles
	bgt_init(&m_ls->bg[0], 1, BG_CBB(0)|BG_SBB(29)|BG_PRIO(3), map_layer0,
		80, 48, 0); //Layer 0
	bgt_init(&m_ls->bg[1], 2, BG_CBB(1)|BG_SBB(27)|BG_PRIO(2), map_layer1,
		80, 48, 1); //Layer 1
		
	//Calculate the initial map shadows
    //We can ignore the blocks at the very edge of the map
    CalculateShadows(1, 1, 79, 47);
    
    //Setup initial map fixture layer object properties and container contents
    LoadLayer1Properties();

    //Load the contents of any initial map containers
    LoadContainers();

    //Setup all sprite object data for the map
    LoadLayer3Objects();

    //If the player did not start from the initial position then
	//he started from a checkpoint, so remove the checkpoint switch
	//from the map at the tile below the checkpoint position, and
	//load the other checkpoint data
	if ((m_ls->checkpoint.playerStartPos.x != map_mapProperties[((m_ls->levelNum) * 3) + 1]) ||
			(m_ls->checkpoint.playerStartPos.y != map_mapProperties[((m_ls->levelNum) * 3) + 2]))
	{
		m_ls->mapData[1][((m_ls->checkpoint.checkpointPos.y)*80) + m_ls->checkpoint.checkpointPos.x].tileIndex = 0;
		m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
		m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
		m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
		m_ls->mapChanges[m_ls->mapChangeCount].x = m_ls->checkpoint.checkpointPos.x;
		m_ls->mapChanges[m_ls->mapChangeCount].y = m_ls->checkpoint.checkpointPos.y;
		m_ls->mapChangeCount++;

		m_ls->bows = m_ls->checkpoint.bows;
		m_ls->arrows = m_ls->checkpoint.arrows;
		m_ls->keys = m_ls->checkpoint.keys;
		m_ls->seconds = m_ls->checkpoint.seconds;
	}
	else
	{
		m_ls->bows = 0;
		m_ls->arrows = 0;
		m_ls->keys = 0;
		m_ls->seconds = 0;
	}

    //Draw the map tiles on the GBA screen
	int tx, ty;

    //Get the tile position, which is the viewpoint coords shifted right by 3,
    //equivalent to dividing by 8
	tx = m_ls->vp.x>>3;
	ty = m_ls->vp.y>>3;

	//Draw the whole screen by calling the bgt_colcpy function 32 times. It's easier
	//doing it this way because the scroll offsets and stuff are taken care of for you
	for (ix = tx; ix < tx + 32 ; ix++)
	{
	    bgt_colcpy(&m_ls->bg[0], ix, ty);
	    bgt_colcpy(&m_ls->bg[1], ix, ty);
    }
		
	//Draw the black border using the text background
	//First set all to transparent
	for (iy = 0; iy < 256; iy+= 8)
	{
	    for (ix = 0; ix < 256; ix+= 8)
	    {
	        txt_putc(ix, iy, (111 + 32)); //Transparent block
        }
    }
	for (iy = 144; iy < 153; iy+= 8)
    	{
        for (ix = 0; ix < 256; ix+= 8)
    	{
            txt_putc(ix, iy, 32); //Space(Black)
        }
    }
}

void CMap::LoadLayer1Properties()
{
	//This procedure reads map_layer1_properties and map_containers
	//from the level data and loads the data into the mapObjects vector within the levelstate.
	int objDataPos, objDataEnd;
	int n;
	int propCount;
	TMapObject tempObject;

	//Clear the mapObjects vector
	m_ls->mapObjects.clear();

	//Get fixture layer properties (second array offset)
	//Get the start and end position of the data within the array
	objDataPos = map_dataOffsets[(m_ls->levelNum * DATA_OFFSETS) + 1];
	//If there's a set of data offsets after this one, set dataEnd to the position
	//before the next level's data. Otherwise dataEnd is set to the size of the array.
	//Divide by 4 in sizeof because it returns number of bytes. int = 4 bytes.
	if ((sizeof(map_dataOffsets) / 4) > (m_ls->levelNum * DATA_OFFSETS) + DATA_OFFSETS)
	{
		//Next level's data offset - 1.
		objDataEnd = map_dataOffsets[((m_ls->levelNum + 1) * DATA_OFFSETS) + 1] - 1;
	}
	else
	{
		//Last element of fixture layer array
		//Variable size is short
		objDataEnd = (sizeof(map_layer1_properties) / 2) - 1;
	}

	//Set tempObject's layer property to 1 for all ojects in this section
	tempObject.layer = 1;

	//Loop from objDataPos to dataEnd and load the object data into tempObject and
	//from there into the mapObjects vector
	while (objDataPos < objDataEnd)
	{
		//Set tempObject's properties to the next object within the list of data.
		tempObject.x = map_layer1_properties[objDataPos];
		tempObject.y = map_layer1_properties[++objDataPos];
		//Look up the property count and whether it's a container.
		//Look up the object type from mapData[0]
		n = objPropertiesLUT[m_ls->mapData[1][(tempObject.y * 80) + tempObject.x].tileIndex];
		propCount = n & CONTENTS_MASK;

		//First reset all of tempObject's properties to zero
		for (n = 0; n < MAX_PROPERTIES; n++)
		{
			tempObject.properties[n] = 0;
		}

		//Read the property values for this object
		for (n = 0; n < propCount; n++)
		{
			tempObject.properties[n] = map_layer1_properties[++objDataPos];
		}

		//Reset the contents object
		tempObject.Contents.type = 0;
		for (n = 0; n < MAX_PROPERTIES; n++)
		{
			tempObject.Contents.properties[n] = 0;
		}

		//Now add the object to the vector
		m_ls->mapObjects.push_back(tempObject);

		//Increment objDataPos if we're not at the end
		if (objDataPos < objDataEnd) {objDataPos++;}
	}
}

void CMap::LoadContainers()
{

	TMapObject tempObject;
	TContentsObject tempContents;
	u32 contentsDataPos;
	int propCount;
	int x,y,n;
	int objIndex;

	//Get the start position of the contents array for this level (fourth array offset)
	contentsDataPos = map_dataOffsets[(m_ls->levelNum * DATA_OFFSETS) + 3];

	//Reset tempObject properties
	for (n = 0; n < MAX_PROPERTIES; n++)
	{
		tempObject.properties[n] = 0;
	}

	//Loop through the map layer 1 tiles to look for containers
	for(y = 0; y < 48 ; y++)
	{
		for(x = 0; x < 80 ; x++)
		{
			if ((m_ls->mapData[1][(y*80) + x].tileIndex == TILE_CHEST) || (m_ls->mapData[1][(y*80) + x].tileIndex == TILE_URN_TOP) )
			{
				//Skip past the x and y coordinates
				//TODO: In future I may remove the coordinate data from the map_containers array
				contentsDataPos = contentsDataPos + 2;

				//Read the contents type (Contents data will be in the same order as the object property
				//data, so we just read the next one)
				tempContents.type = map_containers[contentsDataPos];

				//Reset the contents properties
				for (n = 0; n < MAX_PROPERTIES; n++)
				{
					tempObject.Contents.properties[n] = 0;
				}

				//Look up the property count
				propCount = objPropertiesLUT[tempContents.type] & CONTENTS_MASK;

				//Read the property values for this contents object
				for (n = 0; n < propCount; n++)
				{
					tempContents.properties[n] =  map_containers[++contentsDataPos];
				}
				//Increment contentsDataPos if we're not at the end.
				//map_containers type = signed short
				if (contentsDataPos < (sizeof(map_containers) / 2) - 1) {contentsDataPos++;}

				//Look up an existing object in mapObjects for this location.
				//If the container had properties(urn) then add the contents to the
				//existing map object, otherwise make a new map object.
				objIndex = LookupMapObject(m_ls,x,y,1);
				if (objIndex >= 0)
				{
					m_ls->mapObjects[objIndex].Contents = tempContents;
				}
				else
				{
					tempObject.x = x;
					tempObject.y = y;
					tempObject.layer = 1;
					tempObject.Contents = tempContents;
					m_ls->mapObjects.push_back(tempObject);
				}
			}
		}
	}
}

void CMap::LoadLayer3Objects()
{
	//Add all sprite objects to the m_ls->mapSprites array, including those
	//that are within containers and within all sequences and containers in
	//sequences.
	//NB: Keep a count of sprites added. If we go over the maximum
	//then don't add any more. We should prevent this from happening by checking
	//for too many layer 3 objects within the level editor.
	unsigned int totalLevels;
	unsigned int objDataPos, objDataEnd;

	//unsigned int contentsDataPos;
	unsigned int spriteCount;
	unsigned int n,m;
	int x,y,type,layer;

	spriteCount = 0; //Counter for the ls->mapSprites array. Stop adding sprite
					 //objects if this reaches MAX_SPRITES - 1.


	//Get the total number of levels to make finding the start and end positions
	//of data easier. Use the size of the data offsets array.
	totalLevels = (sizeof(map_dataOffsets) / 4) / 6;

	//***Add the sprite objects that are active at the start of the level***

	//Get the start and end position of the data in the map_layer3 array.
	//First element in map_dataOffsets.
	objDataPos = map_dataOffsets[(m_ls->levelNum * DATA_OFFSETS)];
	if ((totalLevels - 1) > m_ls->levelNum)
	{
		//Next level's data offset - 1.
		objDataEnd = map_dataOffsets[((m_ls->levelNum + 1) * DATA_OFFSETS)] - 1;
	}
	else
	{
		//Last element of sprite layer array
		//Variable size is char
		objDataEnd = sizeof(map_layer3);
	}

	//Read the sprite data
	while (objDataPos < objDataEnd)
	{
		if (spriteCount < MAX_SPRITES)
		{
			m_ls->mapSprites[spriteCount].type = map_layer3[objDataPos++];
			//Convert tile coords to map coords
			m_ls->mapSprites[spriteCount].x = map_layer3[objDataPos++]*32;
			m_ls->mapSprites[spriteCount].y = map_layer3[objDataPos++]*16;
			m_ls->mapSprites[spriteCount].visible = false;
			m_ls->mapSprites[spriteCount].available = true;
			spriteCount++;
		}
		else
		{
			//Skip to the end of the data since we already have too many.
			objDataPos = objDataEnd;
		}
	}

	//Add the initial map sprite properties.
	//Get the start and end position of the data in the map_layer3_properties array.
	//Third element in map_dataOffsets.
	objDataPos = map_dataOffsets[(m_ls->levelNum * DATA_OFFSETS) + 2];
	if ((totalLevels - 1) > m_ls->levelNum)
	{
		//Next level's data offset - 1.
		objDataEnd = map_dataOffsets[((m_ls->levelNum + 1) * DATA_OFFSETS) + 2] - 1;
	}
	else
	{
		//Last element of sprite layer array
		//Variable size is short
		objDataEnd = (sizeof(map_layer3_properties) / 2);
	}
	//Read the properties. They will be in the same order as the sprite object
	//data from map_layer3
	n = 0;//Counter from 0 to spriteCount
	while (objDataPos < objDataEnd)
	{
		//Skip past x and y coordinates
		objDataPos+=2;

		//Loop through any properties (NB All sprites will have at least 1
		//property.)
		for (x = 0; x < (objPropertiesLUT[m_ls->mapSprites[n].type] & CONTENTS_MASK); x++)
		{
			//Add the properties to this sprite object
			m_ls->mapSprites[n].properties[x] = map_layer3_properties[objDataPos++];
		}
		n++;
		//If n is bigger than spriteCount then stop adding properties.
		if (n > spriteCount) {objDataPos = objDataEnd;}
	}

	//Add any sprites that are in containers at the start of the level.
	//Get the start and end position of the data in the map_containers array.
	//Fourth element in map_dataOffsets.
	objDataPos = map_dataOffsets[(m_ls->levelNum * DATA_OFFSETS) + 3];
	if ((totalLevels - 1) > m_ls->levelNum)
	{
		//Next level's data offset - 1.
		objDataEnd = map_dataOffsets[((m_ls->levelNum + 1) * DATA_OFFSETS) + 3] - 1;
	}
	else
	{
		//Last element of containers array
		//Variable size is short
		objDataEnd = (sizeof(map_containers) / 2) - 1;
	}

	while (objDataPos < objDataEnd)
	{
		//Store the x and y coordinates
		x = map_containers[objDataPos++];
		y = map_containers[objDataPos++];
		//See if the container object type is a sprite
		if ((map_containers[objDataPos] == TILE_SNAKE) ||
			(map_containers[objDataPos] == TILE_SPHINX) ||
			(map_containers[objDataPos] == TILE_MOVING_PLATFORM) ||
			(map_containers[objDataPos] == TILE_BLOCK))
		{
			//Add the sprite if there's room in the array
			if (spriteCount < MAX_SPRITES)
			{
				//Convert tile coords to map coords
				m_ls->mapSprites[spriteCount].x = x*32;
				m_ls->mapSprites[spriteCount].y = y*16;
				m_ls->mapSprites[spriteCount].type = map_containers[objDataPos++];
				m_ls->mapSprites[spriteCount].visible = false;
				m_ls->mapSprites[spriteCount].available = false;
				//Read any properties
				for (x = 0; x < (objPropertiesLUT[m_ls->mapSprites[n].type] & CONTENTS_MASK); x++)
				{
					//Add the properties to this sprite object
					m_ls->mapSprites[spriteCount].properties[x] = map_containers[objDataPos++];
				}
				spriteCount++;
			}
			else
			{
				//Skip to the end of the data since we already have too many.
				objDataPos = objDataEnd;
			}
		}
		else
		{
			//Skip past this object type and any properties it has
			y = objPropertiesLUT[map_containers[objDataPos++]] & CONTENTS_MASK;
			objDataPos+=y;
		}
	}

	//Add any sprites that are in sequences.
	//NB!!: I need to subtract 7 from each item type since it's not easy to
	//      do in the level editor when saving sequences. I may change the
	//      way I save the C file in the editor at some point in the future.

	//Get the start and end position of the data in the map_sequences array.
	//Fifth element in map_dataOffsets.
	objDataPos = map_dataOffsets[(m_ls->levelNum * DATA_OFFSETS) + 4];
	if ((totalLevels - 1) > m_ls->levelNum)
	{
		//Next level's data offset - 1.
		objDataEnd = map_dataOffsets[((m_ls->levelNum + 1) * DATA_OFFSETS) + 4] - 1;
	}
	else
	{
		//Last element of sequences array
		//Variable size is short
		objDataEnd = (sizeof(map_sequences) / 2) - 1;
	}

	while (objDataPos < objDataEnd)
	{
		//Add the sequence length to objDataPos, skip past the sequence properties
		//and loop through the sequence.
		n = objDataPos + map_sequences[objDataPos];
		objDataPos += 4;
		while (objDataPos < n)
		{
			//Get the stage length, Skip past the stage properties and
			//loop through the stage
			m = objDataPos + map_sequences[objDataPos];

			objDataPos += 5;
			while (objDataPos < m)
			{
				//Get the object type , coordinates and layer.
				type = map_sequences[objDataPos++];
				x = map_sequences[objDataPos++];
				y = map_sequences[objDataPos++];
				layer = map_sequences[objDataPos++];
				//Subtract 7 from type if in layer 1 or 3 and type is not 0
				if ((layer>0) && (type > 0)) {type-=7;}

				//If the layer is 3 then the object is a sprite so add it
				if (layer == 3)
				{
					m_ls->mapSprites[spriteCount].type = type;
					//Convert tile coords to map coords
					m_ls->mapSprites[spriteCount].x = x*32;
					m_ls->mapSprites[spriteCount].y = y*16;
					m_ls->mapSprites[spriteCount].visible = false;
					m_ls->mapSprites[spriteCount].available = false;
					//Zero out the property array first
					for (x = 0; x < MAX_PROPERTIES; x++)
					{
						m_ls->mapSprites[spriteCount].properties[x] = 0;
					}
					//Add any property values
					for (x = 0; x < (objPropertiesLUT[type] & CONTENTS_MASK); x++)
					{
						//Add the properties to this sprite object
						m_ls->mapSprites[spriteCount].properties[x] = map_sequences[objDataPos++];
					}
					spriteCount++;
				}
				else
				{
					//Else it was not layer 3. Check if it was a container.
					if ((objPropertiesLUT[type] << CONTENTS_BIT) == 1)
					{
						//Skip past any properties to the contents data
						objDataPos += (objPropertiesLUT[type] & CONTENTS_MASK);

						//Get the contents type (-7 because of cutting out wall tiles )
						type = map_sequences[objDataPos++];

						//See if this is a layer 3 tile
						if ((type == TILE_SNAKE) || (type == TILE_SPHINX) || (type == TILE_MOVING_PLATFORM))
						{
							//Add the tile and the properties, if any
							m_ls->mapSprites[spriteCount].type = type;
							//Convert tile coords to map coords
							m_ls->mapSprites[spriteCount].x = x*32;
							m_ls->mapSprites[spriteCount].y = y*16;
							m_ls->mapSprites[spriteCount].visible = false;
							m_ls->mapSprites[spriteCount].available = false;
							for (x = 0; x < (objPropertiesLUT[type] & CONTENTS_MASK); x++)
							{
								//Add the properties to this sprite object
								m_ls->mapSprites[spriteCount].properties[x] = map_sequences[objDataPos++];
							}
							spriteCount++;
						}
						else
						{
							//Not layer 3 so skip past the contents object properties.
							objDataPos += (objPropertiesLUT[type] & CONTENTS_MASK);

						}
					}
					else
					{
						//Else not a container or a layer 3 object, skip past
						//its properties if any.
						objDataPos += (objPropertiesLUT[type] & CONTENTS_MASK);
					}
				}
			}
		}
	}

	//Set the total sprites in the level state
	m_ls->totalSprites = spriteCount;
}

void CMap::UrnHit()
{
	//Process an urn that was hit with an arrow
	//NB: We only allow for one urn to be processed at a time, so
	//we don't need to allow an arrow to hit an urn at the same
	//time as another is breaking. Perhaps check for this extreme
	//case here.

	int objIndex;

	//Get the urn position from the levelstate.
	m_urnPos = m_ls->urnWasHit;

	//Reset the urnWasHit variables in the levelState
	m_ls->urnWasHit.x = -1;
	m_ls->urnWasHit.y = -1;

	//Look up the urn object data
	objIndex = LookupMapObject(m_ls, m_urnPos.x, m_urnPos.y, 1);
	if (objIndex > -1)
	{
		//Reduce this urn's hitpoints by the number of bows
		//the player has.
		m_ls->mapObjects[objIndex].properties[0] =
		 m_ls->mapObjects[objIndex].properties[0] - m_ls->bows;
		//Save a copy of the urn's hitpoints so we don't have to look
		//it up later
		m_urnHitPoints = m_ls->mapObjects[objIndex].properties[0];

		//Change the urn to the flashing urn
		//Top half
		m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
		m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = TILE_URN_FLASH_TOP;
		m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
		m_ls->mapChanges[m_ls->mapChangeCount].x = m_urnPos.x;
		m_ls->mapChanges[m_ls->mapChangeCount].y = m_urnPos.y;
		m_ls->mapChangeCount++;
		//Bottom half
		m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
		m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = TILE_URN_FLASH_BOTTOM;
		m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
		m_ls->mapChanges[m_ls->mapChangeCount].x = m_urnPos.x;
		m_ls->mapChanges[m_ls->mapChangeCount].y = m_urnPos.y + 1;
		m_ls->mapChangeCount++;

		//Set the flash counter
		m_urnFlashCount = URN_FLASH_FRAMES;
	}
}

void CMap::UrnBroke()
{
	int n;
	int objIndex;
	int spriteIndex;
	OBJ_ATTR *obj= &g_obj_buffer[URN_OAM];

	//Urn it has no hitpoints left, so replace the urn on the
	//map with its contents and place a breaking urn sprite
	//where the urn was.
	objIndex = LookupMapObject(m_ls, m_urnPos.x, m_urnPos.y, 1);
	if (objIndex > -1)
	{
		//If the urn contained a sprite (enemy or moving platform),
		//set the sprite in the mapSprites array in the levelstate to available.
		//It will appear upon the next update call for that object.
		if ((m_ls->mapObjects[objIndex].Contents.type == TILE_SNAKE) ||
			(m_ls->mapObjects[objIndex].Contents.type == TILE_SPHINX) ||
			(m_ls->mapObjects[objIndex].Contents.type == TILE_MOVING_PLATFORM))
		{
			//Convert tile coords to map coords when looking up sprite index
			spriteIndex = LookupMapObject(m_ls, m_urnPos.x*32, m_urnPos.y*16, 3);
			if (spriteIndex > -1)
			{
				m_ls->mapSprites[spriteIndex].available = true;
			}
			//Mark the upper and lowwer half of the urn as changed to a blank tile
			m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
			m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
			m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
			m_ls->mapChanges[m_ls->mapChangeCount].x = m_urnPos.x;
			m_ls->mapChanges[m_ls->mapChangeCount].y = m_urnPos.y;
			m_ls->mapChangeCount++;
			m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
			m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
			m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
			m_ls->mapChanges[m_ls->mapChangeCount].x = m_urnPos.x;
			m_ls->mapChanges[m_ls->mapChangeCount].y = m_urnPos.y + 1;
			m_ls->mapChangeCount++;
		}
		else //It contained something other than an sprite object
		{
			//Replace the urn object with it's contents and properties

			//Replace this object's properties with its contents properties.
			//It doesn't matter how many properties the contents object has.
			for (n = 0; n < MAX_PROPERTIES; n++)
			{
				m_ls->mapObjects[objIndex].properties[n] =
						m_ls->mapObjects[objIndex].Contents.properties[n];
			}

			//Mark the upper half of the urn as changed to a blank tile
			m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
			m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
			m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
			m_ls->mapChanges[m_ls->mapChangeCount].x = m_ls->mapObjects[objIndex].x;
			m_ls->mapChanges[m_ls->mapChangeCount].y = m_ls->mapObjects[objIndex].y;
			m_ls->mapChangeCount++;

			//Change the map object's position to one tile lower, since the
			//urn's position is it's upper half.
			m_ls->mapObjects[objIndex].y++;

			//Mark the map position as changed to the contents object type
			//NEW: Account for walls potentially being in an urn, since this
			//does happen on level 5 of the original game.
			if (m_ls->mapObjects[objIndex].Contents.type < 8)
			{
				//Contents is a wall layer tile, so first make sure the
				//lower urn tile is removed, then set the layer
				//for the contents change to 0.
				m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
				m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].x = m_urnPos.x;
				m_ls->mapChanges[m_ls->mapChangeCount].y = m_urnPos.y + 1;
				m_ls->mapChangeCount++;
				m_ls->mapChanges[m_ls->mapChangeCount].layer = 0;
			}
			else
			{
				//Contents is a fixture layer tile
				m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
			}
			m_ls->mapChanges[m_ls->mapChangeCount].tileIndex =
					m_ls->mapObjects[objIndex].Contents.type;
			m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
			m_ls->mapChanges[m_ls->mapChangeCount].x = m_ls->mapObjects[objIndex].x;
			m_ls->mapChanges[m_ls->mapChangeCount].y = m_ls->mapObjects[objIndex].y;
			m_ls->mapChangeCount++;
		}
	}

	//Place a breaking urn sprite where the urn was
	obj_set_attr(obj,
		 ATTR0_SQUARE |           // Square, regular sprite
		 ATTR0_4BPP |           //16 colours
		 ATTR0_MODE(0),         //Un-hide the sprite
		 ATTR1_SIZE_32,          // 32x32p
		 ATTR2_PALBANK(0) |
		 ATTR2_PRIO(2) |
		 ATTR2_ID(SPRTILES_URN)); //First breaking urn tile. See level.cpp init.

	//Play a sound, cancel all other sounds first otherwise this sound
	//may not play.
	mmEffectCancelAll();
	mmEffect(SFX_URN_BREAK);

	//Set object coords.
	//Convert tile position to screen position.
	obj_set_pos(obj, (m_urnPos.x*32) - m_ls->vp.x, (m_urnPos.y*16) - m_ls->vp.y);

	//Let the map update process know there's an urn currently breaking
	m_urnBreakFrame = 0;
}

void CMap::Teleport()
{
	//Steps:
	//Look up the teleport destination based on the properties of the
	//teleporter at m_teleporterPos.
	//Clean up anything that needs cleaning up (Coin scores maybe)
	//Fade to black
	//Store all in-range sprites in the mapSprites array and deactivate them from their vectors.
	//Change the player and map positions
	//Delete the teleporter from the map.
	//Reset m_teleporterPos x and y to -1
	//Redraw the map
	//Fade in

	int objIndex;
	int ix, tx, ty;

	//Get a reference to the in-range sprite vectors.
    std::vector <CEnemy>& enemies = getEnemies();
    std::vector <CPlatform>& platforms = getPlatforms();
    std::vector <CBlock>& blocks = getBlocks();

	objIndex = LookupMapObject(m_ls, m_ls->teleporterTouched.x,m_ls->teleporterTouched.y,1);
	if (objIndex >= 0)
	{
		//Store the sprite positions.
		for(std::vector<CEnemy>::iterator it = enemies.begin(); it != enemies.end();it++)
		{
			it->active = false;
			m_ls->mapSprites[it->obj_index].visible = false;
			if (m_ls->mapSprites[it->obj_index].available == true)
			{
				//Store position and direction.
				m_ls->mapSprites[it->obj_index].x = ((it->x>>8) + m_ls->vp.x);
				m_ls->mapSprites[it->obj_index].y = ((it->y>>8) + m_ls->vp.y);
				m_ls->mapSprites[it->obj_index].properties[3] = (it->dx > 0) ? 1 : 0;
				
			}
		}
		for(std::vector<CPlatform>::iterator it = platforms.begin(); it != platforms.end();it++)
		{
			it->active = false;
			m_ls->mapSprites[it->obj_index].visible = false;
			if (m_ls->mapSprites[it->obj_index].available == true)
			{
				//Store position and direction.
				m_ls->mapSprites[it->obj_index].x = ((it->x>>8) + m_ls->vp.x);
				m_ls->mapSprites[it->obj_index].y = ((it->y>>8) + m_ls->vp.y);
				m_ls->mapSprites[it->obj_index].properties[3] = (it->dx > 0) ? 1 : 0;
				
			}
		}
		for(std::vector<CBlock>::iterator it = blocks.begin(); it != blocks.end();it++)
		{
			it->active = false;
			m_ls->mapSprites[it->obj_index].visible = false;
			if (m_ls->mapSprites[it->obj_index].available == true)
			{
				//Store position, remaining lifespan and directions.
				m_ls->mapSprites[it->obj_index].x = ((it->x>>8) + m_ls->vp.x);
				m_ls->mapSprites[it->obj_index].y = ((it->y>>8) + m_ls->vp.y);
				m_ls->mapSprites[it->obj_index].properties[1] = it->lastXDir;
				m_ls->mapSprites[it->obj_index].properties[2] = it->lifespanCounter;
				m_ls->mapSprites[it->obj_index].properties[3] = (it->dy > 0) ? 1 : 0;
			}
		}

		//Set player position (Map pixel coords)
		m_ls->playerPos.x = (m_ls->mapObjects[objIndex].properties[0])<<8;
		m_ls->playerPos.y = (m_ls->mapObjects[objIndex].properties[1])<<8;

		//Set the map and viewport positions
		m_ls->mapOffset.x =  m_ls->playerPos.x;
		m_ls->mapOffset.y =  m_ls->playerPos.y;
		m_ls->vp.x = m_ls->playerPos.x>>8; 
		m_ls->vp.y = m_ls->playerPos.y>>8;
		vp_center(&m_ls->vp, m_ls->playerPos.x>>8, m_ls->playerPos.y>>8);

		//Get the tile position, which is the viewpoint coords shifted right by 3,
		//equivalent to dividing by 8
		tx = m_ls->vp.x>>3;
		ty = m_ls->vp.y>>3;

		//If the remove property is set then delete the teleporter from the map
		if (m_ls->mapObjects[objIndex].properties[2] == 1)
		{
			m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
			m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
			m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
			m_ls->mapChanges[m_ls->mapChangeCount].x = m_ls->teleporterTouched.x;
			m_ls->mapChanges[m_ls->mapChangeCount].y = m_ls->teleporterTouched.y;
			m_ls->mapChangeCount++;
		}

		//Draw the whole screen by calling the bgt_colcpy function 32 times. It's easier
		//doing it this way because the scroll offsets and stuff are taken care of for you
		for (ix = tx; ix < tx + 32 ; ix++)
		{
			bgt_colcpy(&m_ls->bg[0], ix, ty);
			bgt_colcpy(&m_ls->bg[1], ix, ty);
		}

		//Reset m_teleporterPos
		m_ls->teleporterTouched.x = -1;
		m_ls->teleporterTouched.y = -1;
	}
}

void ShowCoinScore(T_LEVELSTATE *ls, int x, int y, int type)
{
	int n;
	int freeSprite;
	int tile;

	//Makes a coin score sprite appear at the given screen location

	//Find the first coin score sprite that's free (Has type -1)
	n = 0;
	freeSprite = -1;
	while ((n < 5) && (freeSprite == -1))
	{
		if (ls->coinScoreSprites[n].type < 0)
		{
			//Use this sprite
			freeSprite = n;
		}
		else
		{
			n++;
		}

	}
	//If we couldn't find a free sprite (unlikely), just use the first one.
	if (freeSprite == -1)
	{
		freeSprite = 0;
	}

	//Set the object properties
	ls->coinScoreSprites[n].type = type;
	ls->coinScoreSprites[n].x = x; //Don't need to shift because x will not change.
	ls->coinScoreSprites[n].y = y<<8; //Shift left because we're considering y as fixed point.
	ls->coinScoreSprites[n].delay = COIN_SCORE_DELAY;

	//Set the correct sprite tile based on the type (4 tiles per sprite frame)
	tile = SPRTILES_COINSCORES + (4*type);

	//Show the coin sprite.
	obj_set_attr(&g_obj_buffer[COIN_SCORE_OAM + freeSprite],
				ATTR0_WIDE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_16, ATTR2_PALBANK(0)
				| ATTR2_PRIO(0) | ATTR2_ID(tile));
	obj_set_pos(&g_obj_buffer[COIN_SCORE_OAM + freeSprite],
			    ls->coinScoreSprites[n].x - ls->vp.x,
				(ls->coinScoreSprites[n].y>>8) - ls->vp.y);

}

int LookupMapObject(T_LEVELSTATE *ls, int x, int y, int layer)
{
	int n;

	if (layer == 1)
	//Iterate through the mapObjects vector and find the one that matches
	//x and y.
	//Return the index of the object if found, -1 if not found.
	{
		for(std::vector<TMapObject>::iterator it = ls->mapObjects.begin() ; it != ls->mapObjects.end();)
		{
			if (it->x == x && it->y == y )
			{
				//Return the index of the object
				return it - ls->mapObjects.begin();
			}
			else
			{
				++it;
			}
		}
	}
	else //Else look through the sprite object array
	{
		for(n = 0; n < ls->totalSprites; n++)
		{
			if ((ls->mapSprites[n].x) == x && ((ls->mapSprites[n].y) == y))
			{
				//Return the index of the object
				return n;
			}
		}
	}
	//The object wasn't found so return -1
	return -1;
}

int LookupCheckpoint(T_LEVELSTATE *ls, int x, int y)
{
	unsigned int totalLevels;
	int checkpointDataPos, checkpointEnd, checkpointDataEnd;
	int n;

	//Get the total number of levels to make finding the start and end positions
	//of data easier. Use the size of the data offsets array.
	totalLevels = (sizeof(map_dataOffsets) / 4) / 6;

	checkpointDataPos = map_dataOffsets[(ls->levelNum * DATA_OFFSETS) + 5];
	if ((totalLevels - 1) > ls->levelNum)
	{
		//Next level's data offset - 1.
		checkpointDataEnd = map_dataOffsets[((ls->levelNum + 1) * DATA_OFFSETS) + 5] - 1;
	}
	else
	{
		//Last element of checkpoint array
		//Variable size is short
		checkpointDataEnd = (sizeof(map_checkpoints) / 2) - 1;
	}

	while (checkpointDataPos < checkpointDataEnd)
	{
		//See if x and y match
		if ((x == map_checkpoints[checkpointDataPos + 1]) &&
			(y == map_checkpoints[checkpointDataPos + 2]))
		{
			//Get the end position of this checkpoint's data
			checkpointEnd = checkpointDataPos + map_checkpoints[checkpointDataPos];

			//Set the map's checkpoint data
			//We could omit the y - 1 bit
			ls->checkpoint.checkpointPos.x = x;
			ls->checkpoint.checkpointPos.y = y;
			ls->checkpoint.playerStartPos.x = map_checkpoints[checkpointDataPos + 3];
			ls->checkpoint.playerStartPos.y = map_checkpoints[checkpointDataPos + 4] - 1; //Player location is at the tile above
			//Go to bows value
			checkpointDataPos+=5;
			ls->checkpoint.bows = map_checkpoints[checkpointDataPos++];
			ls->checkpoint.arrows = map_checkpoints[checkpointDataPos++];
			ls->checkpoint.keys = map_checkpoints[checkpointDataPos++];
			ls->checkpoint.seconds = map_checkpoints[checkpointDataPos++];
			n = 0; //Activated sequence count
			while (checkpointDataPos < checkpointEnd)
			{
				if (n < MAX_CHECKPOINT_SEQUENCES - 1)
				{
					ls->checkpoint.sequences[n] = map_checkpoints[checkpointDataPos];
				}
				checkpointDataPos++;
			}
			return 1;
		}
		else
		{
			//Increment checkpointDataPos by this checkpoint's data length value
			checkpointDataPos += map_checkpoints[checkpointDataPos];
		}
	}

	//Checkpoint was not found if we got here
	return 0;

}

// EOF
