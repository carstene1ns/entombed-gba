#include "player.h"

//Included code files
#include <tonc.h>

#include "gameDefines.h" //Global game defines
#include "main.h"
#include "map.h"
#include "projectiles.h"
#include "enemies.h"
#include "moving_platforms.h"
#include "moving_blocks.h"
#include "sequences.h"
#include "text.h"
#include "sfx.h"
#include "fader.h"

//Lookup table for object property counts
#include "objPropertiesLUT.h"

//Graphics data
#include "spr_player_gfx.h"

//******CPlayer class function implementations******
//Public
CPlayer::CPlayer()
{
	m_ls = 0;
	m_obj_id = 0;
	m_state = 0;
	m_vx = 0;
	m_vy = 0;
	m_dir = 0;
	m_ani_frame = 0;
	m_jumpCounter = 0;
	m_topOfLadder = false;
	m_touchingLadder = false;
	m_lookingDown = false;
	m_lookDownCounter = 0;
	m_soundFrame = 0;
	sndMoveHandle = 0;
}

void CPlayer::Init(T_LEVELSTATE *ls)
{
	OBJ_ATTR *obj = &g_obj_buffer[0];

	//Load the player sprite palette and tiles
	/*Use CBB (Character Base Block) 4 for the sprites. We'll put the player
	  tiles in from the first tile. ([4][0]). The player graphics have 512 tiles*/
	LZ77UnCompVram(spr_player_gfx, tile_mem[4]);

	obj_set_attr(obj,
	             ATTR0_SQUARE |              // Square, regular sprite
	             ATTR0_4BPP |                //16 colours
	             ATTR0_MODE(0),              //Un-hide the sprite
	             ATTR1_SIZE_32,              // 32x32p
	             ATTR2_PALBANK(0) |
	             ATTR2_PRIO(2) |
	             ATTR2_ID(0));

	//Set local levelstate property as pointer to the global levelstate struct
	m_ls = ls;

	//Call ResetPlayer function to set dynamic member variables
	ResetPlayer();
}

void CPlayer::Update()
{
	//If we're not currently dying then check the collisions and the input, otherwise
	//don't bother since collisions and input are ignored while dying.
	if (m_state != PLAYER_STATE_DYING)
	{
		player_test_collisions();
		if (m_state != PLAYER_STATE_DYING)
		{
			player_input();
		}

	}
	//Increment the sound frame if we're walking or climbing
	if ((m_state == PLAYER_STATE_WALK) || (m_state == PLAYER_STATE_CLIMB))
	{
		m_soundFrame += PLAYER_SPEED;
	}
	else
	{
		//Otherwise reset the sound frame and cancel any sound that
		//is playing
		m_soundFrame = 0;
		if (sndMoveHandle != 0)
		{
			//Cancel sound if not jumping
			if (m_state != PLAYER_STATE_JUMP)
			{
				mmEffectCancel(sndMoveHandle);
			}
			sndMoveHandle = 0;
		}
	}

	player_animate();
	player_move();
}

void CPlayer::player_set_state(u32 state)
{
	m_state = state;
	m_ani_frame = 0;
}

void CPlayer::player_input()
{
	m_vx = m_vy = 0;

	key_poll();

	//We can't move while firing an arrow on the ground
	if ((key_is_down(KEY_RIGHT)) && (m_state != PLAYER_STATE_FIRING))
	{
		if ((m_blockedDirs[1] == 0) && (m_blockedByDoor[1] == false))
		{
			m_vx = PLAYER_SPEED;
		}

		//If we're stood on the ground and our status is PLAYER_STATE_STAND then we need
		//to change it to PLAYER_STATE_WALK. This lets you start walking after you just
		//landed on the ground and already had a direction key pressed down
		if ((m_blockedDirs[3] == 1) && (m_state == PLAYER_STATE_STAND))
		{
			player_set_state(PLAYER_STATE_WALK);
		}

		m_dir = LOOK_RIGHT; //CLIMB_RIGHT when on ladder
	}

	//We can't move while firing an arrow on the ground
	else if (key_is_down(KEY_LEFT) && (m_state != PLAYER_STATE_FIRING))
	{
		if ((m_blockedDirs[0] == 0)  && (m_blockedByDoor[0] == false))
		{
			m_vx = -PLAYER_SPEED;
		}

		//If we're stood on the ground and our status is PLAYER_STATE_STAND then we need
		//to change it to PLAYER_STATE_WALK. This lets you start walking after you just
		//landed on the ground and already had a direction key pressed down
		if ((m_blockedDirs[3] == 1) && (m_state == PLAYER_STATE_STAND))
		{
			player_set_state(PLAYER_STATE_WALK);
		}

		m_dir = LOOK_LEFT; //CLIMB_LEFT when on ladder
	}

	if (key_is_down(KEY_DOWN) && (m_state != PLAYER_STATE_FIRING))
	{
		//First check if there's a ladder behind us
		if (m_touchingLadder == true)
		{
			//Set status to climbing if we're not already in that state
			if (m_state != PLAYER_STATE_CLIMB)
			{
				player_set_state(PLAYER_STATE_CLIMB);
			}

			//If below is not blocked the add some downwards velocity
			if (m_blockedDirs[3] == 0)
			{
				m_vy = PLAYER_SPEED * 2;
			}
		}
	}
	else if (key_is_down(KEY_UP) && (m_state != PLAYER_STATE_FIRING))
	{
		//Check if there's a ladder behind us. Also check for cheat mode.
		//With the big jumps cheat enabled, holding B while jumping will ignore ladders.
		if ((m_touchingLadder == true) && ((g_cheatEnabled[2] == false) || (g_cheatEnabled[2]
		                                   && key_is_down(KEY_B) == false)))
		{
			//If above is not blocked AND a top collision point is touching the ladder
			//then add some upwards velocity
			if (m_blockedDirs[2] == 0)
			{

				//if ((m_layer2Collisions[0][0] == TILE_LADDER) || (m_layer2Collisions[1][0] == TILE_LADDER))
				if ((m_layer2Collisions[2][0] == TILE_LADDER) || (m_layer2Collisions[3][0] == TILE_LADDER))
				{
					//Set status to climbing if we're not already in that state
					if (m_state != PLAYER_STATE_CLIMB)
					{
						player_set_state(PLAYER_STATE_CLIMB);

						//Move up a full pixel
						m_vy = -0x100;
					}
					else
					{
						m_vy = -PLAYER_SPEED * 2;
					}
				}
			}
		}
		else
		{
			//Else begin jumping if we're on the ground
			if ((m_blockedDirs[2] == 0) && (m_blockedDirs[3] == 1))
			{
				mmEffectCancel(sndMoveHandle);
				sndMoveHandle = mmEffect(SFX_JUMP);
				player_set_state(PLAYER_STATE_JUMP);
				m_jumpCounter = 24;
			}
			//If the jump cheat is on, increment jump counter
			if (g_cheatEnabled[2] == true)
			{
				m_jumpCounter++;
			}
		}
	}

	if (key_hit(KEY_A))  //Shoot arrow
	{
		//Check whether the player has a bow and at least one arrow,
		//is not climbing, and is not currently firing an arrow.
		if ((m_ls->bows > 0) && (m_ls->arrows > 0) && (m_state != PLAYER_STATE_CLIMB)
		    && (m_state != PLAYER_STATE_FIRING))
		{
			//Reduce the arrow count
			m_ls->arrows--;

			//Update the arrows display
			txt_putc(64 + (m_ls->arrows * 8), 152, 32); //32 = Space tile

			//Set player state to firing
			player_set_state(PLAYER_STATE_FIRING);
			//Play a sound
			mmEffect(SFX_ARROW);
		}
	}

	if (key_is_down(KEY_B))  //Look down
	{
		//Set looking down to true
		m_lookingDown = true;
	}

	if (key_released(KEY_B))   //Stop looking down
	{
		//Set looking down to false
		m_lookingDown = false;
	}

	if (key_hit(KEY_L))
	{
		//If we have more than one hourglass second available increment the selected seconds,
		//unless the selected seconds = the total seconds, in which case set the selected
		//seconds to 1.
		if (m_ls->seconds > 1)
		{
			if (m_ls->selectedSeconds < m_ls->seconds)
			{
				m_ls->selectedSeconds++;
			}
			else
			{
				m_ls->selectedSeconds = 1;
			}
		}
	}

	if (key_hit(KEY_R))
	{
		//If selectedseconds > 0, call the procedure to add some delay to
		//and running sequences that are not always on.
		if (m_ls->selectedSeconds > 0)
		{
			Sequences::delay(m_ls->selectedSeconds);

			//Set secondsCounter and currentSecond to display the seconds digits on screen
			m_ls->secondsCounter = 60;
			m_ls->currentSecond = m_ls->selectedSeconds;

			//Reduce the seconds total and reset the selected seconds value
			m_ls->seconds -= m_ls->selectedSeconds;
			if (m_ls->seconds > 0)
			{
				m_ls->selectedSeconds = 1;
			}
			else
			{
				m_ls->selectedSeconds = 0;
			}

			//Display the seconds on the screen
			//See if first digit is 1 or 0
			if (m_ls->currentSecond == 10)
			{
				BFN_SET(g_obj_buffer[HOURGLASS_DIGIT_OAM].attr2, SPRTILES_DIGITS + (4 * 1), ATTR2_ID); //"1"
				BFN_SET(g_obj_buffer[HOURGLASS_DIGIT_OAM + 1].attr2, SPRTILES_DIGITS, ATTR2_ID); //"0"
			}
			else
			{
				BFN_SET(g_obj_buffer[HOURGLASS_DIGIT_OAM].attr2, SPRTILES_DIGITS, ATTR2_ID); //"0"
				//Set the second digit
				BFN_SET(g_obj_buffer[HOURGLASS_DIGIT_OAM + 1].attr2, (SPRTILES_DIGITS + (4 * m_ls->currentSecond)),
				        ATTR2_ID);
			}
			//Un-hide the sprites
			obj_unhide(&g_obj_buffer[HOURGLASS_DIGIT_OAM], 0);
			obj_unhide(&g_obj_buffer[HOURGLASS_DIGIT_OAM + 1], 0);
		}
	}

	if (key_hit(KEY_START))
	{
		//Pause the game
		m_ls->game_paused = true;
		//Stop all sounds
		//mmFrame();
		mmEffectCancelAll();
	}

	if (!key_is_down(KEY_DIR))
	{
		if (m_state == PLAYER_STATE_WALK)
		{
			player_set_state(PLAYER_STATE_STAND);
		}
	}

	//The following will be moved into each individual direction's code since we can
	//walk, jump or climb depending on a number of factors
	if (((key_hit(KEY_LEFT)) || (key_hit(KEY_RIGHT))) && (m_blockedDirs[3] == 1)
	    && (m_state != PLAYER_STATE_JUMP) && (m_state != PLAYER_STATE_FIRING))
	{
		player_set_state(PLAYER_STATE_WALK);
	}
}

void CPlayer::player_move()
{
	//Check that we're not dying
	if (m_state != PLAYER_STATE_DYING)
	{
		//Do jumping and gravity
		if (m_state != PLAYER_STATE_JUMP)
		{
			//Check if we were climbing, but are no longer touching a ladder OR if we
			//were climbing and are now stood on the ground
			if (((m_state == PLAYER_STATE_CLIMB) && (m_touchingLadder == false))
			    || ((m_state == PLAYER_STATE_CLIMB) && (m_blockedDirs[3] == 1)))
			{
				//Set state to standing
				player_set_state(PLAYER_STATE_STAND);
				//Reset jump counter
				m_jumpCounter = 0;
			}

			//If the player isn't standing on some ground and is not climbing a ladder
			if ((m_blockedDirs[3] == 0) && (m_state != PLAYER_STATE_CLIMB))
			{
				//If we were just walking (i.e. we walked off a ledge) then set the state
				//to standing
				if (m_state == PLAYER_STATE_WALK)
				{
					player_set_state(PLAYER_STATE_STAND);
					//Reset jump counter
					m_jumpCounter = 0;
				}

				//Add some downwards velocity
				m_vy = PLAYER_SPEED * 2;
			}

			//Check if we're firing an arrow while jumping
			if ((m_state == PLAYER_STATE_FIRING) && m_jumpCounter > 0)
			{
				//Check above
				if (m_blockedDirs[2] == 0)
				{
					//Add some upwards velocity
					m_vy = -PLAYER_SPEED * 2;

					//Decrement the jump counter
					m_jumpCounter--;
				}
				else //We hit a ceiling
				{
					m_jumpCounter = 0;
					//player_set_state(PLAYER_STATE_STAND);
				}
			}
		}
		else //Just jumping
		{
			if (m_jumpCounter == 0)
			{
				player_set_state(PLAYER_STATE_STAND);
			}
			else
			{
				//Check above
				if (m_blockedDirs[2] == 0)
				{
					//Add some upwards velocity
					m_vy = -PLAYER_SPEED * 2;

					//Decrement the jump counter
					m_jumpCounter--;
				}
				else //We hit a ceiling
				{
					m_jumpCounter = 0;
					player_set_state(PLAYER_STATE_STAND);
				}
			}
		}

		//Work out the map offset values (The coordinates that the map is centred on) based
		//on a bounding box around the player

		//Don't add to mapOffsetX unless playerSpr.x - vp.x >= 120 when going right
		//same as above for left but now it's if playerSpr.x - vp.x <= 80

		//If vp.x is 0 or 2560 then set mapOffsetX to playerSpr.x. This avoids problems
		//when we're at the very edge of the map.
		if ((m_ls->vp.x == 0) || (m_ls->vp.x == 2560))
		{
			m_ls->mapOffset.x = m_ls->playerPos.x;
		}

		//Same as above for mapOffsetY
		//NB: max goes to 784 to rather than 768 because the bottom of the map
		//needs to come up above the bottom bar, which is 16 tiles high.
		if ((m_ls->vp.y == 0) || (m_ls->vp.y == 784))
		{
			m_ls->mapOffset.y = m_ls->playerPos.y;
		}

		//If we're in the process of looking down or reverting from looking down
		//See if the player y position relative to the map y position is greater than the max
		if ((m_lookingDown == true) && (((m_ls->playerPos.y >> 8) - m_ls->vp.y) >= (80 - LOOK_DOWN_MAX)))
		{
			//Make sure we're not already at the bottom of the map view. (768 map height - 160 screen height - 32 for bottom bar - 1)
			if (m_ls->vp.y < 623)
			{
				m_ls->mapOffset.y += LOOK_DOWN_SPEED;
				m_lookDownCounter++;
			}
		}
		if ((m_lookingDown == false) && (m_lookDownCounter > 0)
		    && (((m_ls->playerPos.y >> 8) - m_ls->vp.y) < 80))
		{
			m_ls->mapOffset.y -= LOOK_DOWN_SPEED;
			m_lookDownCounter--;
		}

		//If going right
		if (m_vx > 0)
		{
			if (((m_ls->playerPos.x >> 8) - m_ls->vp.x) >= 120)
			{

				m_ls->mapOffset.x += m_vx;
			}
		}
		else //going left
		{
			if (((m_ls->playerPos.x >> 8) - m_ls->vp.x) <= 80)
			{
				m_ls->mapOffset.x += m_vx;
			}
		}

		//If going up
		if (m_vy < 0)
		{
			if (((m_ls->playerPos.y >> 8) - m_ls->vp.y) <= 50)
			{

				m_ls->mapOffset.y += m_vy;
				if (m_lookDownCounter > 0)
				{
					m_lookDownCounter--;
				}
			}
		}
		else //going down
		{
			if (((m_ls->playerPos.y >> 8) - m_ls->vp.y) >= 80)
			{
				m_ls->mapOffset.y += m_vy;
			}
		}
	}

	//Else we're dying so do the movements for that
	else
	{
		m_vx = 0;

		//We're re-using the jump counter to time the upward motion in the dying sequence
		if (m_jumpCounter > 0)
		{
			//Add some upwards velocity
			m_vy = -PLAYER_SPEED * 2;

			//Decrement the jump counter
			m_jumpCounter--;
		}
		else
		{
			//Add some downwards velocity
			m_vy = PLAYER_SPEED * 2;
		}
	}

	//Add the velocity value to the player's position values
	m_ls->playerPos.x += m_vx;
	m_ls->playerPos.y += m_vy;

	//If we've moved out of the screen, set the m_ls->levelStatus to ST_LIFE_LOST
	if ((m_ls->playerPos.y >> 8) - m_ls->vp.y > 160)
	{
		m_ls->levelStatus = LevelStatus::LIFE_LOST;
	}
}

void CPlayer::player_animate()
{

	switch (m_state)
	{
		case PLAYER_STATE_STAND:
		case PLAYER_STATE_JUMP:
			player_ani_stand();
			break;
		case PLAYER_STATE_WALK:
			if (m_soundFrame >> 8 >= SOUND_FRAME_WALK)
			{
				mmEffectCancel(sndMoveHandle);
				sndMoveHandle = mmEffect(SFX_WALK);
				m_soundFrame = 0;
			}

			m_ani_frame += ANI_SPEED_WALK;
			player_ani_walk();
			break;
		case PLAYER_STATE_CLIMB:
			//Only update the climbing animation frame if we're moving
			if ((m_vx != 0) || (m_vy != 0))
			{
				if (m_soundFrame >> 8 >= SOUND_FRAME_CLIMB)
				{
					mmEffectCancel(sndMoveHandle);
					sndMoveHandle = mmEffect(SFX_CLIMB);
					m_soundFrame = 0;
				}
				m_ani_frame += ANI_SPEED_CLIMB;
			}
			player_ani_climb();
			break;
		case PLAYER_STATE_FIRING:
			m_ani_frame += ANI_SPEED_FIRE_BOW;
			player_ani_fireArrow();
			break;
		case PLAYER_STATE_DYING:
			m_ani_frame += ANI_SPEED_DIE;
			player_ani_die();
			break;
	}
}

void CPlayer::player_ani_stand()
{

	POINT pt = { (m_ls->playerPos.x >> 8) - m_ls->vp.x, (m_ls->playerPos.y >> 8) - m_ls->vp.y };
	OBJ_ATTR *obj = &g_obj_buffer[m_obj_id];

	int dir = m_dir;

	// Set coords
	obj_set_pos(obj, pt.x, pt.y);

	//Set tile index
	if (dir == LOOK_LEFT)
	{
		if (m_ls->bows == 0)
		{
			BFN_SET(obj->attr2,  0, ATTR2_ID); //First frame
		}
		else
		{
			BFN_SET(obj->attr2,  8 * 16, ATTR2_ID); //Ninth frame
		}
	}
	else
	{
		if (m_ls->bows == 0)
		{
			BFN_SET(obj->attr2,  4 * 16, ATTR2_ID); //Fifth frame
		}
		else
		{
			BFN_SET(obj->attr2,  12 * 16, ATTR2_ID); //Thirteenth frame
		}
	}
}

void CPlayer::player_ani_walk()
{
	int dir = m_dir;
	int frame = (m_ani_frame >> 8) & 3; //Loop through 4 frames

	POINT pt = { (m_ls->playerPos.x >> 8) - m_ls->vp.x, (m_ls->playerPos.y >> 8) - m_ls->vp.y };
	OBJ_ATTR *obj = &g_obj_buffer[m_obj_id];

	// Set coords
	obj_set_pos(obj, pt.x, pt.y);

	//Set tile index
	if (dir == LOOK_LEFT)
	{
		if (m_ls->bows == 0)
		{
			BFN_SET(obj->attr2,  frame * 16, ATTR2_ID);

		}
		else
		{
			BFN_SET(obj->attr2, (frame + 8) * 16, ATTR2_ID);
		}
	}
	else
	{
		if (m_ls->bows == 0)
		{
			BFN_SET(obj->attr2, (frame + 4) * 16, ATTR2_ID);
		}
		else
		{
			BFN_SET(obj->attr2, (frame + 12) * 16, ATTR2_ID);
		}
	}
}

void CPlayer::player_ani_climb()
{
	int dir = m_dir;
	int frame = (m_ani_frame >> 8) & 1; //Loop through 2 frames

	POINT pt = { (m_ls->playerPos.x >> 8) - m_ls->vp.x, (m_ls->playerPos.y >> 8) - m_ls->vp.y };
	OBJ_ATTR *obj = &g_obj_buffer[m_obj_id];

	// Set coords
	obj_set_pos(obj, pt.x, pt.y);

	//Set tile index
	if (dir == LOOK_LEFT)
	{
		if (m_ls->bows == 0)
		{
			BFN_SET(obj->attr2, (frame + 16) * 16, ATTR2_ID);
		}
		else
		{
			BFN_SET(obj->attr2, (frame + 20) * 16, ATTR2_ID);
		}
	}
	else
	{
		if (m_ls->bows == 0)
		{
			BFN_SET(obj->attr2, (frame + 18) * 16, ATTR2_ID);
		}
		else
		{
			BFN_SET(obj->attr2, (frame + 22) * 16, ATTR2_ID);
		}
	}
}

void CPlayer::player_ani_fireArrow()
{
	int dir = m_dir;
	//We'll go through 3 frames. Walk frame 1, bow frame, walk frame 1.
	int frame = (m_ani_frame >> 8);

	POINT pt = { (m_ls->playerPos.x >> 8) - m_ls->vp.x, (m_ls->playerPos.y >> 8) - m_ls->vp.y };
	OBJ_ATTR *obj = &g_obj_buffer[m_obj_id];

	// Set coords
	obj_set_pos(obj, pt.x, pt.y);

	if ((frame == 1)  || (frame == 3))
	{
		//Set tile index to walk frame 1
		if (dir == LOOK_LEFT)
		{
			BFN_SET(obj->attr2,  9 * 16, ATTR2_ID);
		}
		else
		{
			BFN_SET(obj->attr2,  13 * 16, ATTR2_ID);
		}
	}
	//The second frame has the bow launch frame
	else
	{
		if (frame == 2)
		{
			if (m_dir == LOOK_LEFT)
			{
				BFN_SET(obj->attr2,  24 * 16, ATTR2_ID);
			}
			else //RIGHT
			{
				BFN_SET(obj->attr2,  25 * 16, ATTR2_ID);
			}
		}
	}
	//At the 4th frame, launch the arrow and return to the standing state.
	if (frame == 4)
	{
		if (m_dir == LOOK_LEFT)
		{
			Projectiles::add(m_ls, ((m_ls->playerPos.x >> 8) - m_ls->vp.x) - 8,
			                 ((m_ls->playerPos.y >> 8) - m_ls->vp.y) + 20, -ARROW_SPEED, 0, 0, -1, 1);
		}
		else //RIGHT
		{
			Projectiles::add(m_ls, ((m_ls->playerPos.x >> 8) - m_ls->vp.x) + 24,
			                 ((m_ls->playerPos.y >> 8) - m_ls->vp.y) + 20, ARROW_SPEED, 0, 1, -1, 1);
		}
		player_set_state(PLAYER_STATE_STAND);
	}
}

void CPlayer::player_ani_die()
{
	int frame = (m_ani_frame >> 8) & 1; //Loop through 2 frames

	POINT pt = { (m_ls->playerPos.x >> 8) - m_ls->vp.x, (m_ls->playerPos.y >> 8) - m_ls->vp.y };
	OBJ_ATTR *obj = &g_obj_buffer[m_obj_id];

	// Set coords
	obj_set_pos(obj, pt.x, pt.y);

	BFN_SET(obj->attr2, (frame + 26) * 16, ATTR2_ID);
}

void CPlayer::player_test_collisions()
{
	//The player has a bounding box  from the centre going down do the bottom of his
	//feet, to the top of the white part of his head. Then 8 pixels from either side
	//of his centre.

	//If the player isn't standing on something solid then set the player state to
	//PLAYER_STATE_JUMP and set the jump upwards counter to zero (This is set to the height
	//of the jump when the player is actually jumping and it counts down every frame.
	//While it is above zero, the player will move up every frame, otherwise he'll move
	//down instead)

	//If there's a wall one pixel to the left or to the right, make a note of it. Then
	//when we come to the movement routine we can nudge the player back one pixel if
	//he tried to move into a wall.

	//If we're touching something that kills us then set the player state to SPR_DYING
	int collisionPoint[3];
	int bx, by;
	int bulCentreX, bulCentreY; //Centre position of the various bullets.
	//Used in projectile collision testing

	std::vector<CProjectile> &projectiles = Projectiles::get();
	std::vector<CEnemy> &enemies = Enemies::get();
	std::vector<CPlatform> &platforms = Platforms::get();
	std::vector<CBlock> &blocks = Blocks::get();

	//Walls collision checks
	//First reset the m_blockedDirs[4] values to zero
	m_blockedDirs[0] = 0; //Left
	m_blockedDirs[1] = 0; //Right
	m_blockedDirs[2] = 0; //Up
	m_blockedDirs[3] = 0; //Down

	//Also reset the m_blockedByDoor[2] values to false
	m_blockedByDoor[0] = false;
	m_blockedByDoor[1] = false;

	//If either tile underneath the player (There will only 2 tiles since the
	//player's bounding box is 16 pixels wide) is a wall,
	//I can find the pixel directly underneath the left of
	//the player's bounding box and then do a <<3 on it to find the left tile
	//I can then do the same thing for the pixel under the right side of the bounding
	//box, if either is a wall then set m_blockedDirs[3] to 1

	//For above walls, top corners

	//For left and right, three points to the side, 0y, 8y and 16y,

	//I need to find the tile within the visible map that the player is touching

	//If I know whereabouts on the whole map the player is(player.x, player.y) I can
	//use the map data (blocks) to find the blocks next to the player by shifting the
	//player coordinates by another 2 places for x and 1 for y

	//maxLeft = x + 10
	//maxRight = x + 21
	//maxUp = y + 8
	//maxDown = y + 31

	//Check left (1 pixel further left of player)
	//Must shift player.x and y right by 8 to get the pixel coordinate then by another
	//3 to get the tile coordinate, 2 to get the block coordinate 13 in total
	collisionPoint[0] = ((((m_ls->playerPos.y >> 8) + 8) >> 4) * 80) + (((m_ls->playerPos.x >> 8) +
	                    (PLAYER_BBOX_LEFT - 1)) >> 5);
	collisionPoint[1] = ((((m_ls->playerPos.y >> 8) + 16) >> 4) * 80) + (((m_ls->playerPos.x >> 8) +
	                    (PLAYER_BBOX_LEFT - 1)) >> 5);
	collisionPoint[2] = ((((m_ls->playerPos.y >> 8) + 31) >> 4) * 80) + (((m_ls->playerPos.x >> 8) +
	                    (PLAYER_BBOX_LEFT - 1)) >> 5);

	//Check for wall tiles
	if ((m_ls->mapData[0][collisionPoint[0]].tileIndex > 0) ||
	    (m_ls->mapData[0][collisionPoint[1]].tileIndex > 0)	||
	    (m_ls->mapData[0][collisionPoint[2]].tileIndex > 0))
	{
		m_blockedDirs[0] = 1;
	}
	//Check for layer 1 blocking tiles (Wooden frame)
	if ((m_ls->mapData[1][collisionPoint[0]].tileIndex == TILE_WOOD_FRAME)
	    || (m_ls->mapData[1][collisionPoint[0]].tileIndex == TILE_WOOD_FRAME_TOP) ||
	    (m_ls->mapData[1][collisionPoint[1]].tileIndex == TILE_WOOD_FRAME)
	    || (m_ls->mapData[1][collisionPoint[1]].tileIndex == TILE_WOOD_FRAME_TOP) ||
	    (m_ls->mapData[1][collisionPoint[2]].tileIndex == TILE_WOOD_FRAME)
	    || (m_ls->mapData[1][collisionPoint[2]].tileIndex == TILE_WOOD_FRAME_TOP))
	{
		m_blockedDirs[0] = 1;
	}

	//Check right (1 pixel further right of player)
	collisionPoint[0] = ((((m_ls->playerPos.y >> 8) + 8) >> 4) * 80) + (((m_ls->playerPos.x >> 8) +
	                    (PLAYER_BBOX_RIGHT + 1)) >> 5);
	collisionPoint[1] = ((((m_ls->playerPos.y >> 8) + 16) >> 4) * 80) + (((m_ls->playerPos.x >> 8) +
	                    (PLAYER_BBOX_RIGHT + 1)) >> 5);
	collisionPoint[2] = ((((m_ls->playerPos.y >> 8) + 31) >> 4) * 80) + (((m_ls->playerPos.x >> 8) +
	                    (PLAYER_BBOX_RIGHT + 1)) >> 5);

	//Check for wall tiles
	if ((m_ls->mapData[0][collisionPoint[0]].tileIndex > 0) ||
	    (m_ls->mapData[0][collisionPoint[1]].tileIndex > 0)	||
	    (m_ls->mapData[0][collisionPoint[2]].tileIndex > 0))
	{
		m_blockedDirs[1] = 1;
	}
	//Check for layer 1 blocking tiles (Wooden frame)
	if ((m_ls->mapData[1][collisionPoint[0]].tileIndex == TILE_WOOD_FRAME)
	    || (m_ls->mapData[1][collisionPoint[0]].tileIndex == TILE_WOOD_FRAME_TOP) ||
	    (m_ls->mapData[1][collisionPoint[1]].tileIndex == TILE_WOOD_FRAME)
	    || (m_ls->mapData[1][collisionPoint[1]].tileIndex == TILE_WOOD_FRAME_TOP) ||
	    (m_ls->mapData[1][collisionPoint[2]].tileIndex == TILE_WOOD_FRAME)
	    || (m_ls->mapData[1][collisionPoint[2]].tileIndex == TILE_WOOD_FRAME_TOP))
	{
		m_blockedDirs[1] = 1;
	}

	//For above and below, we only need to check if we're 1 pixel below or 1 pixel above
	//a block boundary

	//Check up (1 pixel above player)
	collisionPoint[0] = ((((m_ls->playerPos.y >> 8) + 7) >> 4) * 80) + (((m_ls->playerPos.x >> 8) +
	                    (PLAYER_BBOX_LEFT)) >> 5);
	collisionPoint[1] = ((((m_ls->playerPos.y >> 8) + 7) >> 4) * 80) + (((m_ls->playerPos.x >> 8) +
	                    (PLAYER_BBOX_RIGHT)) >> 5);

	//Check that we're just below the block boundary (y + 8 % 16 == 0 )
	if (((m_ls->playerPos.y >> 8) + 8) % 16 == 0)
	{
		//Check for wall tiles
		if ((m_ls->mapData[0][collisionPoint[0]].tileIndex > 0)
		    || (m_ls->mapData[0][collisionPoint[1]].tileIndex > 0))
		{
			m_blockedDirs[2] = 1;
		}
		//Check for layer 1 blocking tiles (Wooden frame)
		if ((m_ls->mapData[1][collisionPoint[0]].tileIndex == TILE_WOOD_FRAME)
		    || (m_ls->mapData[1][collisionPoint[1]].tileIndex == TILE_WOOD_FRAME) ||
		    (m_ls->mapData[1][collisionPoint[0]].tileIndex == TILE_WOOD_FRAME_TOP)
		    || (m_ls->mapData[1][collisionPoint[1]].tileIndex == TILE_WOOD_FRAME_TOP))
		{
			m_blockedDirs[2] = 1;
		}
	}

	//Check down (1 pixel below player)
	collisionPoint[0] = ((((m_ls->playerPos.y >> 8) + 32) >> 4) * 80) + (((m_ls->playerPos.x >> 8) +
	                    (PLAYER_BBOX_LEFT)) >> 5);
	collisionPoint[1] = ((((m_ls->playerPos.y >> 8) + 32) >> 4) * 80) + (((m_ls->playerPos.x >> 8) +
	                    (PLAYER_BBOX_RIGHT)) >> 5);

	//Check that we're just above the block boundary (y % 16 == 0 )
	if ((m_ls->playerPos.y >> 8) % 16 == 0)
	{
		//Check for wall tiles
		if ((m_ls->mapData[0][collisionPoint[0]].tileIndex > 0)
		    || (m_ls->mapData[0][collisionPoint[1]].tileIndex > 0))
		{
			m_blockedDirs[3] = 1;
		}
		//Check for layer 1 blocking tiles (Wooden frame)
		if ((m_ls->mapData[1][collisionPoint[0]].tileIndex == TILE_WOOD_FRAME)
		    || (m_ls->mapData[1][collisionPoint[1]].tileIndex == TILE_WOOD_FRAME) ||
		    (m_ls->mapData[1][collisionPoint[0]].tileIndex == TILE_WOOD_FRAME_TOP)
		    || (m_ls->mapData[1][collisionPoint[1]].tileIndex == TILE_WOOD_FRAME_TOP))
		{
			m_blockedDirs[3] = 1;
		}
	}

	//Bug Fix: There's a bug in the game where if you fall diagonally exactly onto
	//the corner of a wall block, you'll go into the block by one pixel. The
	//collision check routine will therefore kill you as you're "inside" the block
	//so it thinks the block appeared on top of you. To fix this, I'll set the
	//player's x velocity to zero if they would go one pixel inside the block.

	//Check left
	if (((((m_ls->playerPos.x) >> 8) + PLAYER_BBOX_LEFT) % 32 == 31) && (m_blockedDirs[0] == 1))
	{
		m_ls->playerPos.x += 1 << 8;
	}

	//Check right
	if (((((m_ls->playerPos.x) >> 8) + PLAYER_BBOX_RIGHT) % 32 == 0) && (m_blockedDirs[1] == 1))
	{
		m_ls->playerPos.x -= 1 << 8;
	}

	//Check if we've been killed by a block appearing on us. This will be true if a
	//direction is blocked by a wall and the player's bounding box is one or more pixels
	//past a block edge (i.e. for left/right this means playerSpr.x is not exactly
	//divisible by 32) We only need to check the x direction here.
	//Check left
	if (((((m_ls->playerPos.x) >> 8) + PLAYER_BBOX_LEFT) % 32 != 0) && (m_blockedDirs[0] == 1))
	{
		player_set_state(PLAYER_STATE_DYING);
		m_jumpCounter = 24;
		mmEffect(SFX_PLAYER_DIE);
	}

	//Check right
	if (((((m_ls->playerPos.x) >> 8) + PLAYER_BBOX_RIGHT) % 32 != 31) && (m_blockedDirs[1] == 1))
	{
		player_set_state(PLAYER_STATE_DYING);
		m_jumpCounter = 24;
		mmEffect(SFX_PLAYER_DIE);
	}

	//Now check down for a static platform (Platform 1 or 2)
	//First check that we're one pixel above a block boundary
	if ((m_ls->playerPos.y >> 8) % 16 == 0)
	{
		if ((m_ls->mapData[1][((((m_ls->playerPos.y >> 8) + 32) >> 4) * 80) +
		                      (((m_ls->playerPos.x >> 8) + PLAYER_BBOX_LEFT) >> 5)].tileIndex == TILE_PLATFORM1) ||
		    (m_ls->mapData[1][((((m_ls->playerPos.y >> 8) + 32) >> 4) * 80) +
		                      (((m_ls->playerPos.x >> 8) + PLAYER_BBOX_RIGHT) >> 5)].tileIndex == TILE_PLATFORM1) ||
		    (m_ls->mapData[1][((((m_ls->playerPos.y >> 8) + 32) >> 4) * 80) +
		                      (((m_ls->playerPos.x >> 8) + PLAYER_BBOX_LEFT) >> 5)].tileIndex == TILE_PLATFORM2) ||
		    (m_ls->mapData[1][((((m_ls->playerPos.y >> 8) + 32) >> 4) * 80) +
		                      (((m_ls->playerPos.x >> 8) + PLAYER_BBOX_RIGHT) >> 5)].tileIndex == TILE_PLATFORM2))
		{
			m_blockedDirs[3] = 1;
		}
	}

	//Check against layer 2 objects
	//I need to check 6 points that the player sprite covers and see what is
	//behind each one. Potentially the player could be touching upto 4 things at once
	//(e.g. on a ladder, and goes diagonally down left, we would be touching the ladder,
	//and possible the tiles to the left, down-left and down)
	//.  . x=10, 21 y = 8, 8
	//.  . x=10, 21 y = 16, 16
	//.  . x=10, 21 y = 31, 31

	//Check collision points
	//Top left
	bx = ((m_ls->playerPos.x >> 8) + PLAYER_BBOX_LEFT) >> 5;
	by = ((m_ls->playerPos.y >> 8) + 8) >> 4;
	m_layer2Collisions[0][0] = m_ls->mapData[1][(by * 80) + bx].tileIndex;
	m_layer2Collisions[0][1] = bx;
	m_layer2Collisions[0][2] = by;
	//Top right
	bx = ((m_ls->playerPos.x >> 8) + PLAYER_BBOX_RIGHT) >> 5;
	by = ((m_ls->playerPos.y >> 8) + 8) >> 4;
	m_layer2Collisions[1][0] = m_ls->mapData[1][(by * 80) + bx].tileIndex;
	m_layer2Collisions[1][1] = bx;
	m_layer2Collisions[1][2] = by;
	//Middle left
	bx = ((m_ls->playerPos.x >> 8) + PLAYER_BBOX_LEFT) >> 5;
	by = ((m_ls->playerPos.y >> 8) + 16) >> 4;
	m_layer2Collisions[2][0] = m_ls->mapData[1][(by * 80) + bx].tileIndex;
	m_layer2Collisions[2][1] = bx;
	m_layer2Collisions[2][2] = by;
	//Middle right
	bx = ((m_ls->playerPos.x >> 8) + PLAYER_BBOX_RIGHT) >> 5;
	by = ((m_ls->playerPos.y >> 8) + 16) >> 4;
	m_layer2Collisions[3][0] = m_ls->mapData[1][(by * 80) + bx].tileIndex;
	m_layer2Collisions[3][1] = bx;
	m_layer2Collisions[3][2] = by;
	//Bottom left
	bx = ((m_ls->playerPos.x >> 8) + PLAYER_BBOX_LEFT) >> 5;
	by = ((m_ls->playerPos.y >> 8) + 31) >> 4;
	m_layer2Collisions[4][0] = m_ls->mapData[1][(by * 80) + bx].tileIndex;
	m_layer2Collisions[4][1] = bx;
	m_layer2Collisions[4][2] = by;
	//Bottom right
	bx = ((m_ls->playerPos.x >> 8) + PLAYER_BBOX_RIGHT) >> 5;
	by = ((m_ls->playerPos.y >> 8) + 31) >> 4;
	m_layer2Collisions[5][0] = m_ls->mapData[1][(by * 80) + bx].tileIndex;
	m_layer2Collisions[5][1] = bx;
	m_layer2Collisions[5][2] = by;

	//Check if we're colliding with any layer 2 object by adding all
	//m_layer2Collisions[i][0] elements together, if the result is above zero then call
	//the processLayer2Collisions function
	if (m_layer2Collisions[0][0] + m_layer2Collisions[1][0] + m_layer2Collisions[2][0]
	    + m_layer2Collisions[3][0] + m_layer2Collisions[5][0] + m_layer2Collisions[5][0]
	    > 0)
	{
		ProcessLayer2Collisions();
	}
	else
		//Make sure m_touchingLadder gets set to zero
	{
		m_touchingLadder = 0;
	}

	//Check projectile collisions
	for (auto& projectile : projectiles)
	{
		//Check if the projectile is a bullet (gun/snake/ball)
		if (projectile.type > 1)
		{
			//Get the bullet centre for this bullet type
			if (projectile.type == 2) //Gun
			{
				bulCentreX = 4;
				bulCentreY = 1;
			}
			else
			{
				if (projectile.type == 3) //Snake bullet
				{
					bulCentreX = 1;
					bulCentreY = 1;
				}
				else //Bouncing ball (L5 projectile)
				{
					//Not sure what the ball centre position is yet. Assume 4,4
					bulCentreX = 4;
					bulCentreY = 4;
				}
			}
			//See if the bullet intersects the player's bounding box
			//If so then set the player state to dying and remove the bullet.
			//NB: projectile positions are relative to the screen, the player
			//position is relative to the map so we subtract the viewport value.
			//Check for collisions with the centre of the bullet rather than
			//the bullet's bounding box to make things easier. I may check against
			//the bullet's bounding box in the future.
			if ((((projectile.x >> 8) + bulCentreX) >= (m_ls->playerPos.x >> 8) - m_ls->vp.x + PLAYER_BBOX_LEFT)
			    &&
			    (((projectile.x >> 8) + bulCentreX) <= (m_ls->playerPos.x >> 8) - m_ls->vp.x + PLAYER_BBOX_RIGHT) &&
			    (((projectile.y >> 8) + bulCentreY) >= (m_ls->playerPos.y >> 8) - m_ls->vp.y + 9) && //Top
			    (((projectile.y >> 8) + bulCentreY) <= (m_ls->playerPos.y >> 8) - m_ls->vp.y + 32)) //Bottom
			{
				//This bullet intersects the player's bounding box so set the player
				//state to dying and deactivate the bullet.
				//Ignore if no hit cheat is on
				if (g_cheatEnabled[3] == false)
				{
					player_set_state(PLAYER_STATE_DYING);
					m_jumpCounter = 24;
					projectile.active = false;
					mmEffect(SFX_PLAYER_DIE);
				}
			}
		}
	}

	//Check for collisions with visible enemies
	for (const auto& enemy : enemies)
	{
		//Make sure the enemy is not in the process of dying
		if (enemy.isDying == false)
		{
			//See if the enemy's bounding box intersects the player's bounding box.
			//If so then set the player state to dying.
			//NB: Enemy positions are relative to the screen, the player
			//position is relative to the map so we subtract the viewport value.
			if ((((enemy.x >> 8) + ENEMY_BBOX_RIGHT) >= ((m_ls->playerPos.x >> 8) - m_ls->vp.x) +
			     PLAYER_BBOX_LEFT) &&
			    (((enemy.x >> 8) + ENEMY_BBOX_LEFT) <= ((m_ls->playerPos.x >> 8) - m_ls->vp.x) + PLAYER_BBOX_RIGHT)
			    &&
			    (((enemy.y >> 8) + 30) >= ((m_ls->playerPos.y >> 8) - m_ls->vp.y) + 7) &&
			    (((enemy.y >> 8) + ENEMY_BBOX_TOP) <= ((m_ls->playerPos.y >> 8) - m_ls->vp.y) + 31))
			{
				//This enemy intersects the player's bounding box so set the player
				//state to dying.
				//Ignore if no hit cheat is on
				if (g_cheatEnabled[3] == false)
				{
					player_set_state(PLAYER_STATE_DYING);
					m_jumpCounter = 24;
					mmEffect(SFX_PLAYER_DIE);
				}
			}
		}
	}

	//Check for collisions with visible moving blocks
	for (const auto& block : blocks)
	{
		//See if the block's bounding box intersects the player's bounding box.
		//If so then set the player state to dying.
		//NB: Block positions are relative to the screen, the player
		//position is relative to the map so we subtract the viewport value.
		if ((((block.x >> 8) + 32) >= ((m_ls->playerPos.x >> 8) - m_ls->vp.x) + PLAYER_BBOX_LEFT) &&
		    ((block.x >> 8) <= ((m_ls->playerPos.x >> 8) - m_ls->vp.x) + PLAYER_BBOX_RIGHT) &&
		    (((block.y >> 8) + 16) >= ((m_ls->playerPos.y >> 8) - m_ls->vp.y) + 7) &&
		    ((block.y >> 8) <= ((m_ls->playerPos.y >> 8) - m_ls->vp.y) + 31))
		{
			//This block intersects the player's bounding box so set the player
			//state to dying.
			//Ignore if no hit cheat is on
			if (g_cheatEnabled[3] == false)
			{
				player_set_state(PLAYER_STATE_DYING);
				m_jumpCounter = 24;
				mmEffect(SFX_PLAYER_DIE);
			}
		}
	}

	//Check moving platform collisions
	for (auto& platform : platforms)
	{
		//If the player is not currently dying.
		//See if the player is stood on this platform. Horizontal position of
		//players bounding box must intersect the platform and the bottom of the
		//player must be exactly in line with the platform's y position.
		//TODO: If you're stood at the edge of a block and a platform arrives, the
		//platform will push you slightly then take you with it. I need to prevent this
		//so that you only go on a moving platform if you're not on a block already.
		//Fixed with checking that down direction isn't blocked, but still maybe not
		//perfect.
		if (((platform.x >> 8) <= (m_ls->playerPos.x >> 8) - m_ls->vp.x + PLAYER_BBOX_RIGHT) &&
		    (((platform.x >> 8) + 32) >= (m_ls->playerPos.x >> 8) - m_ls->vp.x + PLAYER_BBOX_LEFT) &&
		    ((platform.y >> 8) == (m_ls->playerPos.y >> 8) - m_ls->vp.y + 32) && (m_state != PLAYER_STATE_DYING)
		    && (m_blockedDirs[3] == 0))
		{
			//Set the down direction to blocked, and set the player as touching
			//this platform.
			m_blockedDirs[3] = 1;
			platform.playerTouchingPlatform = true;
		}
		else
		{
			//The player is not touching this platform
			platform.playerTouchingPlatform = false;
		}
	}
}

void CPlayer::ProcessLayer2Collisions()
{
	int i, x, objIndex;
	int passNum, temp[3]; //Bubble sort variables
	int sortedCollisions[6][3];
	int lastCollision[2];

	//If any layer 2 collision is with a killer tile then simply kill the player and
	//exit
	for (i = 0; i < 6; i++)
	{
		if ((m_layer2Collisions[i][0] >= 1) && (m_layer2Collisions[i][0] <= 9))
		{
			//Kill the player
			//Ignore if no hit cheat is on
			if (g_cheatEnabled[3] == false)
			{
				player_set_state(PLAYER_STATE_DYING);
				m_jumpCounter = 24;
				mmEffect(SFX_PLAYER_DIE);
				return;
			}
		}
	}

	//Check for ladders
	//Set  m_touchingLadder to false initially
	m_touchingLadder = false;

	//Only check against the top two collision points. If either of these points is not
	//touching a ladder then you're not on a ladder. This prevents the player from
	//being above the top of a ladder.
	//NB: Updated so top 2 collision points are not classed as touching a ladder
	/*if ( (m_layer2Collisions[0][0] == TILE_LADDER) || (m_layer2Collisions[1][0] == TILE_LADDER) ||
	(m_layer2Collisions[2][0] == TILE_LADDER) || (m_layer2Collisions[3][0] == TILE_LADDER) ||
	(m_layer2Collisions[4][0] == TILE_LADDER) || (m_layer2Collisions[5][0] == TILE_LADDER) )*/
	if ((m_layer2Collisions[2][0] == TILE_LADDER) || (m_layer2Collisions[3][0] == TILE_LADDER) ||
	    (m_layer2Collisions[4][0] == TILE_LADDER) || (m_layer2Collisions[5][0] == TILE_LADDER))
	{
		m_touchingLadder = true;
	}

	//Do a bubble sort (ascending) on the collision list to eliminate any duplicates
	//First put the collision list into a temporary variable
	for (i = 0; i < 6; i++)
	{
		sortedCollisions[i][0] = m_layer2Collisions[i][0];
		sortedCollisions[i][1] = m_layer2Collisions[i][1];
		sortedCollisions[i][2] = m_layer2Collisions[i][2];
	}

	for (passNum = 1; passNum <= 5; passNum++)
	{
		for (i = 0; i < 6 - passNum; i++)
		{
			if (sortedCollisions[i][0] > sortedCollisions[i + 1][0])
			{
				temp[0] = sortedCollisions[i][0];
				temp[1] = sortedCollisions[i][1];
				temp[2] = sortedCollisions[i][2];

				sortedCollisions[i][0] = sortedCollisions[i + 1][0];
				sortedCollisions[i][1] = sortedCollisions[i + 1][1];
				sortedCollisions[i][2] = sortedCollisions[i + 1][2];

				sortedCollisions[i + 1][0] = temp[0];
				sortedCollisions[i + 1][1] = temp[1];
				sortedCollisions[i + 1][2] = temp[2];
			}
		}
	}

	//Loop through the sorted collision list and do whatever's necessary for each
	//collision

	//Keep track of last collision coordinates so we don't process the same object more than once.
	lastCollision[0] = -1;
	lastCollision[1] = -1;

	for (i = 0; i < 6; i++)
	{
		//Check that this collision is not the same as the last one
		//If the x or y coordinate is different than the previous one
		//then go through with the collision check
		if ((sortedCollisions[i][1] != lastCollision[0]) ||
		    (sortedCollisions[i][2] != lastCollision[1]))
		{
			//Save the collision coordinates
			lastCollision[0] = sortedCollisions[i][1];
			lastCollision[1] = sortedCollisions[i][2];

			//Process the collision
			switch (sortedCollisions[i][0])
			{
				case TILE_LADDER: //Ladder (N/A: ladders processed further up)
					break;

				case TILE_KEY: //Key
					//Check that we don't have too many keys (say 5 is max)
					if (m_ls->keys < 5)
					{
						//Check that we're actually touching the key, rather than just the
						//block that the key is in.
						if (((((m_ls->playerPos.x >> 8) + PLAYER_BBOX_RIGHT) >=
						      (sortedCollisions[i][1] << 5) + 1)
						     && (((m_ls->playerPos.x >> 8) + PLAYER_BBOX_LEFT) <=
						         (sortedCollisions[i][1] << 5) + 30)))
						{
							//Mark the map position as changed
							m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
							m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
							m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
							m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
							m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2];
							m_ls->mapChangeCount++;

							//Add a key to the player
							m_ls->keys += 1;

							//Play a sound
							mmEffect(SFX_KEY);
						}
					}
					break;

				case TILE_ANKH: //Ankh

					//Check that we don't have too many lives (6 is max)
					if (g_lives < 6)
					{
						//Check that we're actually touching the ankh, rather than just the
						//block that the ankh is in.
						if (((((m_ls->playerPos.x >> 8) + PLAYER_BBOX_RIGHT) >=
						      (sortedCollisions[i][1] << 5) + 8)
						     && (((m_ls->playerPos.x >> 8) + PLAYER_BBOX_LEFT) <=
						         (sortedCollisions[i][1] << 5) + 23)))
						{
							//Mark the map position as changed
							m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
							m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
							m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
							m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
							m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2];
							m_ls->mapChangeCount++;

							//Add a life to the player
							g_lives += 1;

							//Play a sound
							mmEffect(SFX_ANKH);
						}
					}
					break;

				case TILE_COIN: //Coin

					//Check that we're actually touching the coin, rather than just the
					//block that the coin is in.
					if (((((m_ls->playerPos.x >> 8) + PLAYER_BBOX_RIGHT) >=
					      (sortedCollisions[i][1] << 5) + 4)
					     && (((m_ls->playerPos.x >> 8) + PLAYER_BBOX_LEFT) <=
					         (sortedCollisions[i][1] << 5) + 28)))
					{

						//Mark the map position as changed
						m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
						m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
						m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
						m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
						m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2];
						m_ls->mapChangeCount++;

						//Display the coin score sprite for a short time and set it to move
						//upwards slowly during that time

						//Look up the index of the coin's object data in the mapObjects vector
						objIndex = LookupMapObject(m_ls, sortedCollisions[i][1], sortedCollisions[i][2], 1);
						//If the object data was found, add the coin's score to the player
						if (objIndex > -1)
						{
							//Show the correct coin score sprite based on the coin score
							switch (m_ls->mapObjects[objIndex].properties[0])
							{
								case 250:
									i = 0;
									break;
								case 500:
									i = 1;
									break;
								case 750:
									i = 2;
									break;
								case 1000:
									i = 3;
									break;
								default:
									i = 0;
									break;
							}
							//Convert map tile position to map pixel position
							ShowCoinScore(m_ls, (m_ls->mapObjects[objIndex].x * 32),
							              (m_ls->mapObjects[objIndex].y * 16), i);

							//Play a sound
							mmEffect(SFX_COIN);

							//Increment the player's score
							g_score += m_ls->mapObjects[objIndex].properties[0];

							//Delete the object from the vector
							m_ls->mapObjects.erase(m_ls->mapObjects.begin() + objIndex);
						}
					}

					break;

				case TILE_HOURGLASS: //Hourglass

					//Check that we don't have too many seconds already (say 30 is max)
					if (m_ls->seconds < 30)
					{
						//Check that we're actually touching the ankh, rather than just the
						//block that the coin is in.
						if (((((m_ls->playerPos.x >> 8) + PLAYER_BBOX_RIGHT) >=
						      (sortedCollisions[i][1] << 5) + 8)
						     && (((m_ls->playerPos.x >> 8) + PLAYER_BBOX_LEFT) <=
						         (sortedCollisions[i][1] << 5) + 23)))
						{
							//Mark the map position as changed
							m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
							m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
							m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
							m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
							m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2];
							m_ls->mapChangeCount++;

							//Look up the index of the hourglass object data in the mapObjects vector
							objIndex = LookupMapObject(m_ls, sortedCollisions[i][1], sortedCollisions[i][2], 1);
							//If the object data was found, add the seconds to the player up to
							//the max amount (30 for now)
							if (objIndex > -1)
							{
								m_ls->seconds += m_ls->mapObjects[objIndex].properties[0];
								if (m_ls->seconds > 30)
								{
									m_ls->seconds = 30;
								}

								//Delete the object from the vector
								m_ls->mapObjects.erase(m_ls->mapObjects.begin() + objIndex);

								//Play a sound(Same as for the ankh)
								mmEffect(SFX_ANKH);

								//Set selected seconds to 1 if it was previously 0
								if (m_ls->selectedSeconds == 0) m_ls->selectedSeconds = 1;
							}
						}
					}
					break;

				case TILE_BOW: //Bow
					//Check that we don't have too many bows (say 5 is max)
					if (m_ls->bows < 5)
					{
						//Mark the map position as changed
						m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
						m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
						m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
						m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
						m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2];
						m_ls->mapChangeCount++;

						//Add a bow to the player
						m_ls->bows += 1;

						//Play a sound
						mmEffect(SFX_BOW);
					}
					break;

				case TILE_QUIVER: //Quiver
					//Check that we don't have too many arrows (12 is max)
					if (m_ls->arrows < 12)
					{
						//Mark the map position as changed
						m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
						m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
						m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
						m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
						m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2];
						m_ls->mapChangeCount++;

						//Find out how many arrows this quiver contains by checking it's
						//x and y block value against a pre-made list.
						//Add the arrows to the player.
						//If the player now has more than the maximum arrows then set
						//his arrows to the maximum number.

						//Look up the index of the quiver object data in the mapObjects vector
						objIndex = LookupMapObject(m_ls, sortedCollisions[i][1], sortedCollisions[i][2], 1);
						//If the object data was found, add the quiver's arrows to the player
						if (objIndex > -1)
						{
							m_ls->arrows += m_ls->mapObjects[objIndex].properties[0];
							if (m_ls->arrows > 12)
							{
								m_ls->arrows = 12;
							}

							//Delete the object from the vector
							m_ls->mapObjects.erase(m_ls->mapObjects.begin() + objIndex);

							//Play a sound(Same as for the bow)
							mmEffect(SFX_BOW);
						}
					}
					break;

				case TILE_CHEST: //Chest
					if (m_ls->keys > 0)
					{
						//Look up the index of the chest object data in the mapObjects vector
						objIndex = LookupMapObject(m_ls, sortedCollisions[i][1], sortedCollisions[i][2], 1);
						//If the object data was found,
						if (objIndex > -1)
						{
							//Replace this object's properties with its contents properties.
							//It doesn't matter how many properties the contents object has.
							for (x = 0; x < MAX_PROPERTIES; x++)
							{
								m_ls->mapObjects[objIndex].properties[x] =
								    m_ls->mapObjects[objIndex].Contents.properties[x];
							}

							//Mark the map position as changed to the contents object type
							m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
							m_ls->mapChanges[m_ls->mapChangeCount].tileIndex =
							    m_ls->mapObjects[objIndex].Contents.type;
							m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
							m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
							m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2];
							m_ls->mapChangeCount++;

							//Reduce the players keys by 1
							m_ls->keys--;

							//Update the keys display
							txt_putc(144 + (m_ls->keys * 8), 144, 32); //Space tile
						}
					}
					break;

				//case TILE_DOOR_CLOSED_TOP: //Closed door (top) (only need to know about touching the bottom)
				case TILE_DOOR_CLOSED_BOTTOM: //Closed door (Bottom)
					//Check that we're actually touching the door, rather than just the
					//block that the door is in.
					if ((((m_ls->playerPos.x >> 8) + PLAYER_BBOX_RIGHT) >=
					     (sortedCollisions[i][1] << 5)
					     && (((m_ls->playerPos.x >> 8) + PLAYER_BBOX_LEFT) <=
					         (sortedCollisions[i][1] << 5) + 8)))
					{
						//If the player has a key then the door will open and a key
						//will be removed from the player
						if (m_ls->keys > 0)
						{
							//Mark the map positions as changed
							//If we touched the top then change this block and the
							//one below
							if (sortedCollisions[i][0] == TILE_DOOR_CLOSED_TOP)
							{
								m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
								m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = TILE_DOOR_OPEN_TOP;
								m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
								m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
								m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2];
								m_ls->mapChangeCount++;

								m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
								m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = TILE_DOOR_OPEN_BOTTOM;
								m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
								m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
								m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2] + 1;
								m_ls->mapChangeCount++;

							}
							else //Else change this block and the one above
							{
								m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
								m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = TILE_DOOR_OPEN_BOTTOM;
								m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
								m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
								m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2] - 1;
								m_ls->mapChangeCount++;

								m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
								m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = TILE_DOOR_OPEN_TOP;
								m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
								m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
								m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2];
								m_ls->mapChangeCount++;

							}

							//Remove a key from the player
							m_ls->keys--;

							//Play a sound
							mmEffect(SFX_DOOR);

							//Update the keys display
							txt_putc(144 + (m_ls->keys * 8), 144, 32); //Space tile
						}
						else //Otherwise the door blocks the player
						{
							//If the player's x coordinate is in the same block as the
							//door then the door is to the player's left, else it's to
							//his right
							//>>8 for pixel >>5 for block
							if (((m_ls->playerPos.x >> 8) + 8) >> 5 == sortedCollisions[i][1])
							{
								m_blockedByDoor[0] = true;
							}
							else if ((m_ls->playerPos.x >> 8) >> 5 != sortedCollisions[i][1])
							{
								m_blockedByDoor[1] = true;
							}

						}

					}

					break;

				//case TILE_EXIT_TOP: //Exit (Top) (only need to know about touching the bottom)
				case TILE_EXIT_BOTTOM: //Exit (Bottom)

					//Play a sound
					mmEffect(SFX_EXIT);

					//Set the level as cleared
					m_ls->levelStatus = LevelStatus::LEVEL_COMPLETED;

					//Delay until the sound has stopped (2 seconds)
					for (i = 0; i < 120; i++)
					{
						VBlankIntrWait();
						mmFrame();
					}

					break;

				case TILE_SWITCH: //Switch
					//Look up the index of the switch object data in the mapObjects vector
					objIndex = LookupMapObject(m_ls, sortedCollisions[i][1], sortedCollisions[i][2], 1);
					//If the object data was found, either initiate the switch's sequence or do
					//the checkpoint stuff if the sequence property was -1
					if (objIndex > -1)
					{
						if (m_ls->mapObjects[objIndex].properties[0] >= 0)
						{
							Sequences::initiate(m_ls, m_ls->mapObjects[objIndex].properties[0], false);

							//Set the switch's property to -1 since we've activated it
							//In the future could have another property indicating whether
							//the switch is active or not. When I come to the L5 toggle
							//switches, I can reset a switch to active after a delay.
							m_ls->mapObjects[objIndex].properties[0] = -1;
						}
						else
						{
							//Else it's either a checkpoint or a deactivated switch
							//Try and look up the checkpoint data from map_checkpoints.
							//That function sets the map's checkpoint data if one
							//was found and returns 1. Otherwise returns -1 if a
							//checkpoint was not found.
							if (LookupCheckpoint(m_ls, sortedCollisions[i][1], sortedCollisions[i][2]))
							{
								//Remove the checkpoint switch from the map
								m_ls->mapChanges[m_ls->mapChangeCount].layer = 1;
								m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = 0;
								m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
								m_ls->mapChanges[m_ls->mapChangeCount].x = sortedCollisions[i][1];
								m_ls->mapChanges[m_ls->mapChangeCount].y = sortedCollisions[i][2];
								m_ls->mapChangeCount++;

								//Increment the checkpoint counter and add that 1000 * this number
								//to the score
								m_ls->checkpointCount++;
								g_score += (1000 * m_ls->checkpointCount);

								//Play a sound (Same as coin sound)
								mmEffect(SFX_COIN);
							}
						}
					}
					break;

				case TILE_TELEPORTER: //Teleporter
					//Check that we're actually touching the teleporter, rather than just the
					//block that the teleporter is in.
					if (((((m_ls->playerPos.x >> 8) + PLAYER_BBOX_RIGHT) >=
					      (sortedCollisions[i][1] << 5) + 8)
					     && (((m_ls->playerPos.x >> 8) + PLAYER_BBOX_LEFT) <=
					         (sortedCollisions[i][1] << 5) + 23)))
					{
						m_ls->teleporterTouched.x = sortedCollisions[i][1];
						m_ls->teleporterTouched.y = sortedCollisions[i][2];
					}
					break;
			}
		}
	}
}

void CPlayer::ResetPlayer()
{
	//Set player start position from map data. Convert checkpoint
	//start position, which is in map pixel coords.
	m_ls->playerPos.x = (m_ls->checkpoint.playerStartPos.x) << 8;
	m_ls->playerPos.y = (m_ls->checkpoint.playerStartPos.y) << 8;

	m_ls->mapOffset.x =  m_ls->playerPos.x;
	m_ls->mapOffset.y =  m_ls->playerPos.y;
	m_vx = 0;
	m_vy = 0;
	player_set_state(PLAYER_STATE_STAND);
	m_dir = LOOK_RIGHT; //Get this from the map data
	player_ani_stand();

	m_blockedDirs[0] = 0; //Left
	m_blockedDirs[1] = 0; //Right
	m_blockedDirs[2] = 0; //Up
	m_blockedDirs[3] = 0; //Down
	m_jumpCounter = 0;

	m_ls->keys = 0;
	m_ls->bows = 0; //Will be set from map checkpoint data
	m_ls->arrows = 0; //Will be set from map checkpoint data
	m_ls->seconds = 0;
	m_ls->selectedSeconds = 0;
	//Reset level 5 teleporter variables
	m_ls->teleporterTouched.x = -1;
	m_ls->teleporterTouched.y = -1;
}

// EOF
