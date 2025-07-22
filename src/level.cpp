#include "level.h"

//Included code files
#include <tonc.h>
#include <vector>
#include <stdlib.h>

#include "gameDefines.h" //Global game defines
#include "main.h"
#include "moving_platforms.h"
#include "player.h"
#include "map.h"
#include "projectiles.h"
#include "enemies.h"
#include "guns.h"
#include "moving_blocks.h"
#include "sequences.h"
#include "text.h"
#include "fader.h"
#include "level_data.h"
#include "sfx.h"

//Sprite data
#include "spr_projectiles_gfx.h"
#include "spr_enemies_gfx.h"
#include "spr_urn_gfx.h"
#include "spr_platform_gfx.h"
#include "spr_gameover_gfx.h"
#include "spr_coinscores_gfx.h"
#include "spr_digits_gfx.h"
#include "spr_blocks_gfx.h"

//GLOBALS
EWRAM_DATA T_LEVELSTATE g_levelState; //Takes up 20kb so put in EWRAM

//implementation of member functions

//******CLevel class function implementations******
CLevel::CLevel()
{
	//Create the Map instance
	Map = std::make_unique<CMap>();

	//Create the Player instance
	Player = std::make_unique<CPlayer>();

	//Initialise member properties
	m_levelEnded = false;

	g_levelState.levelStatus = LevelStatus::PLAYING_LEVEL;

	m_fadedIn = false;
	m_teleported = false;
	m_cheatSelected = 0;
}

void CLevel::Init()
{
	int n;

	//Load the projectile sprite tiles (8 tiles)
	//Use CBB (Character Base Block) 4 for the sprites. We'll put the arrow tiles after the player tiles (pos 512)
	//The player graphics have 512 tiles
	LZ77UnCompVram(spr_projectiles_gfx, &tile_mem[4][SPRTILES_PROJECTILES]);

	//Load the enemy tiles right after the projectile tiles + 1
	LZ77UnCompVram(spr_enemies_gfx, &tile_mem[4][SPRTILES_ENEMIES]);

	//Load the breaking urn sprite after the enemies sprite
	//The enemies sprite has 16 frames = 256 tiles
	LZ77UnCompVram(spr_urn_gfx, &tile_mem[4][SPRTILES_URN]);

	//Load the moving platform sprite after the breaking urn sprite
	// breaking urn sprite has 4 frames = 64 tiles
	LZ77UnCompVram(spr_platform_gfx, &tile_mem[4][SPRTILES_PLATFORM]);

	//Load the game over sprite after the moving platform sprite
	//Moving platform has 1 frame, 8 tiles.
	LZ77UnCompVram(spr_gameover_gfx, &tile_mem[4][SPRTILES_GAMEOVER]);

	//Load the coin scores sprites after the game over sprite
	//Game over sprite has 3 frames, 24 tiles.
	LZ77UnCompVram(spr_coinscores_gfx, &tile_mem[4][SPRTILES_COINSCORES]);

	//Load the hourglas digits sprites (16x16)
	//Start from 888. (coin score sprites have 16 tiles)
	LZ77UnCompVram(spr_digits_gfx, &tile_mem[4][SPRTILES_DIGITS]);

	//Load the moving blocks sprites (32x16)
	//Start from 928. (hourglass digit sprites have 40 tiles)
	LZ77UnCompVram(spr_blocks_gfx, &tile_mem[4][SPRTILES_BLOCKS]);

	//Initialise the level data offsets array (6 array offsets in total)
	for (n = 0; n < 6; n++)
	{
		//If there's only one level, all data offsets will be zero, otherwise
		//they'll be looked up from the level data
		if (g_levelState.levelNum == 0)
		{
			g_levelState.levelDataOffsets[n] = 0;
		}
		else
		{
			g_levelState.levelDataOffsets[n] = map_dataOffsets[(g_levelState.levelNum * 6) + n];
		}
	}

	//Read the wall tile set value from the map properties
	g_levelState.wallTiles = map_mapProperties[g_levelState.levelNum * 3];

	//Set the initial checkpoint data, taking the player start position from
	//map properties (3 map properties per level)
	g_levelState.checkpoint.playerStartPos.x = map_mapProperties[(g_levelState.levelNum * 3) + 1];
	g_levelState.checkpoint.playerStartPos.y = map_mapProperties[(g_levelState.levelNum * 3) + 2];
	g_levelState.checkpoint.arrows = 0;
	g_levelState.checkpoint.bows = 0;
	g_levelState.checkpoint.seconds = 0;
	for (n = 0; n < MAX_CHECKPOINT_SEQUENCES; n++)
	{
		g_levelState.checkpoint.sequences[n] = -1;
	}
	g_levelState.checkpointCount = 0;

	g_levelState.secondsCounter = 0;

	//Call the player initialisation method from player.cpp
	//Pass projectiles, enemies and moving platforms vectors
	Player->Init(&g_levelState);

	//Call the map initialisation method from map.cpp
	Map->Init(&g_levelState);

	//Reset the sprite data
	Projectiles::reset(&g_levelState);
	Enemies::reset(&g_levelState);
	Platforms::reset(&g_levelState);
	Blocks::reset(&g_levelState);

	//Initialise the guns
	Guns::init(&g_levelState);

	//Initiate any sequences that are always on
	Sequences::initiate_always_on(&g_levelState);

	//Fade the palette in from black after the first level update
	m_fadedIn = false;

}

void CLevel::Update()
{
	int ix;

	if (g_levelState.game_paused == false)
	{
		//If a teleport was touched, fade the screen call the teleport
		//procedure and let the program know we need to fade in again.
		if ((g_levelState.teleporterTouched.x > -1) && (g_levelState.teleporterTouched.y > -1))
		{
			//Play a sound
			mmEffectCancelAll();
			mmEffect(SFX_TELEPORT);

			g_fader->apply(FadeType::OUT, 10);
			//Set the player to standing and reset jump counter
			Player->m_jumpCounter = 0;
			Player->player_set_state(PLAYER_STATE_STAND);
			Map->Teleport();
			m_teleported = true;
		}

		//Call the player update routine if not game over
		if (g_lives >= 0)
		{
			Player->Update();
		}

		//VBlank before screen-changing functions are called
		mmFrame();
		VBlankIntrWait();

		//Call the map update routine
		Map->Update();

		//Update the projectile movements
		Projectiles::update(&g_levelState);

		//Update the enemies
		Enemies::update(&g_levelState);

		//Update the moving platforms
		Platforms::update(&g_levelState);

		//Update guns
		Guns::update(&g_levelState);

		//Update moving blocks
		Blocks::update(&g_levelState);

		//Update the sequences (Sequence counters etc)
		Sequences::update(&g_levelState);

		//Copy the OAM data
		oam_copy(oam_mem, g_obj_buffer, MAX_SPRITES);

		//Fade the palette in from black during the first level update
		if (m_fadedIn == false)
		{
			m_fadedIn = true;
			//Fade into the level palette data
			g_fader->apply(FadeType::IN, 30);
		}

		//Fade the palette in from black after a teleport
		if (m_teleported == true)
		{
			m_teleported = false;
			//Fade into the level palette data
			g_fader->apply(FadeType::IN, 10);
		}
	}
	else
	{
		//VBlank before screen-changing functions are called
		mmFrame();
		VBlankIntrWait();
		//Display pause menu
		txt_puts(96, 80, "PAUSED");
		//Display cheat text if cheats enabled
		if (g_cheatEnabled[0] == true)
		{
			switch (m_cheatSelected)
			{
				case 0: //Lives
					if (g_cheatEnabled[1] == false)
					{
						txt_puts(96, 72, "INF LIVES OFF");
					}
					else
					{
						txt_puts(96, 72, "INF LIVES ON ");
					}
					break;
				case 1:
					txt_puts(96, 72, "ADD BOW      ");
					break;
				case 2:
					txt_puts(96, 72, "ADD ARROW    ");
					break;
				case 3:
					txt_puts(96, 72, "ADD KEY      ");
					break;
				case 4:
					txt_puts(96, 72, "ADD SECOND   ");
					break;
				case 5: //Big jumps
					if (g_cheatEnabled[2] == false)
					{
						txt_puts(96, 72, "BIG JUMPS OFF");
					}
					else
					{
						txt_puts(96, 72, "BIG JUMPS ON ");
					}
					break;
				case 6:
					if (g_cheatEnabled[3] == false)
					{
						txt_puts(96, 72, "NO HIT OFF   ");
					}
					else
					{
						txt_puts(96, 72, "NO HIT ON    ");
					}
					break;
			}
		}

		//Check input during pause
		key_poll();
		if (key_hit(KEY_START))
		{
			//Restart level if SELECT was pressed along with START.
			//Otherwise just unpause.
			if (key_is_down(KEY_SELECT))
			{
				//Set the level state to life lost.
				g_levelState.levelStatus = LevelStatus::LIFE_LOST;
			}
			//Set the pause menu text to transparent characters
			for (ix = 96; ix <= 200; ix += 8)
			{
				txt_putc(ix, 72, (111 + 32)); //Cheat text
				txt_putc(ix, 80, (111 + 32)); //Paused text
			}
			//Reset the looking down mode in case B was held when pausing.
			Player->m_lookingDown = false;
			g_levelState.game_paused = false;
		}
		if ((key_hit(KEY_L)) && (g_cheatEnabled[0] == true))
		{
			//Move to the next cheat
			m_cheatSelected++;
			if (m_cheatSelected > 6)
			{
				m_cheatSelected = 0;
			}
		}
		if ((key_hit(KEY_R)) && (g_cheatEnabled[0] == true))
		{
			switch (m_cheatSelected)
			{
				case 0: //Lives
					g_cheatEnabled[1] == false ? g_cheatEnabled[1] = true
					    : g_cheatEnabled[1] = false;
					break;
				case 1: //Add Bow
					if (g_levelState.bows < 5)
					{
						g_levelState.bows++;
					}
					break;
				case 2: //Add Arrow
					if (g_levelState.arrows < 10)
					{
						g_levelState.arrows++;
					}
					break;
				case 3: //Add Key
					if (g_levelState.keys < 5)
					{
						g_levelState.keys++;
					}
					break;
				case 4: //Add Second
					if (g_levelState.seconds < 10)
					{
						g_levelState.seconds++;
					}
					break;
				case 5: //Big jumps
					g_cheatEnabled[2] == false ? g_cheatEnabled[2] = true
					    : g_cheatEnabled[2] = false;
					break;
				case 6: //No hit (enemy/spikes/fire/water/bullets don't kill)
					g_cheatEnabled[3] == false ? g_cheatEnabled[3] = true
					    : g_cheatEnabled[3] = false;
					break;
			}
		}
	}
}

void CLevel::Reset()
{
	int n;
	bool sequencesDone;
	//Get a reference to the sequences vector so we can
	//process any checkpoint sequences.
	std::vector<CSequence> &sequences = Sequences::get();

	//Fade the palette to black
	g_fader->apply(FadeType::OUT, 30);

	//Reset sprite data
	Projectiles::reset(&g_levelState);
	Enemies::reset(&g_levelState);
	Platforms::reset(&g_levelState);
	Blocks::reset(&g_levelState);

	//Reset the coin score sprites
	for (n = 0; n < 5; n++)
	{
		g_levelState.coinScoreSprites[n].type = -1;
	}

	//Reset the player
	Player->ResetPlayer();

	//Reset the map
	Map->ResetMap();

	//Initiate any sequences activated by the current checkpoint
	if (g_levelState.checkpoint.sequences[0] > -1)
	{
		Sequences::initiate_checkpoint(&g_levelState);

		//If any checkpoint sequences were initiated, update them
		//until all stages for each one are done.
		sequencesDone = false; /*Set initially to false to we go through
		                         the while loop at least once*/
		while (sequencesDone == false)
		{
			sequencesDone = true; /*If at least one active sequence is found this will
			                        be reset to false*/
			if (sequences.size() > 0)
			{
				Sequences::update(&g_levelState);
				Map->processMapChanges();
				sequencesDone = false;
			}
		}
	}

	//Reset the guns
	Guns::init(&g_levelState);

	//Initiate any sequences that are always on
	Sequences::initiate_always_on(&g_levelState);

	//Copy the sprite objects
	mmFrame();
	VBlankIntrWait();
	oam_copy(oam_mem, g_obj_buffer, MAX_SPRITES);

	//Fade the palette in from black after the first level update
	m_fadedIn = false;
}

void CLevel::Main(int mapNum)
{
	auto Level = std::make_unique<CLevel>();

	int done = 0;
	int n;
	int gameover_y = -32 << 8;

	//Set the level number in levelstate
	g_levelState.levelNum = mapNum;

	VBlankIntrWait();

	// Enable map backgrounds
	REG_DISPCNT |= DCNT_BG1 | DCNT_BG2;

	//Call the init procedure
	Level->Init();

	//Main game loop
	while (!done)
	{
		switch (g_levelState.levelStatus)
		{
			case LevelStatus::PLAYING_LEVEL:
				//Update the level
				Level->Update();
				break;

			case LevelStatus::LIFE_LOST:
				//Reduce the player's lives if the cheat is disabled
				if (g_cheatEnabled[1] == false)
				{
					g_lives -= 1;
				}
				else
				{
					g_lives = 6;
				}

				//Check if the player has any lives left
				//If not then end the game (High score check, return to title screen)
				if (g_lives < 0)
				{
					g_levelState.levelStatus = LevelStatus::ALL_LIVES_LOST;
				}
				else
				{
					//Otherwise return to the last checkpoint
					Level->Reset();
					g_levelState.levelStatus = LevelStatus::PLAYING_LEVEL;
				}
				break;

			case LevelStatus::LEVEL_COMPLETED:
				//Mark the level as completed in the array
				g_completedLevels[g_levelState.levelNum] = true;

				//Set the game state to the level selector
				g_GameState = GameState::LEVELSELECT;

				//End the loop
				done = 1;
				break;

			case LevelStatus::ALL_LIVES_LOST:

				//Show the game over sprite
				for (n = 0; n < 3; n++)
				{
					obj_set_attr(&g_obj_buffer[GAME_OVER_OAM + n],
					             ATTR0_WIDE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32, ATTR2_PALBANK(0)
					             | ATTR2_PRIO(0) | ATTR2_ID(SPRTILES_GAMEOVER + (n * 8)));
					obj_set_pos(&g_obj_buffer[n + GAME_OVER_OAM],
					            72 + (n * 32), gameover_y >> 8);
				}

				//Move the game over sprite down the screen
				while ((gameover_y >> 8) < 64)
				{
					gameover_y += GAME_OVER_FALL_SPEED;
					for (n = 0; n < 3; n++)
					{
						obj_set_pos(&g_obj_buffer[n + GAME_OVER_OAM],
						            72 + (n * 32), gameover_y >> 8);
					}
					Level->Update();
				}

				//Wait a bit
				n = 120;
				while (n > 0)
				{
					Level->Update();
					n--;
				}

				// Hide the game over sprite
				for (n = 0; n < 3; n++)
				{
					obj_hide(&g_obj_buffer[n + GAME_OVER_OAM]);
				}
				oam_copy(oam_mem, g_obj_buffer, MAX_SPRITES);

				//Fade out
				g_fader->apply(FadeType::OUT, 30);

				//If we got a high score, set the game state to the
				//high score entry
				if (g_score > g_highScores[9].score)
				{
					g_GameState = GameState::ENTERHIGHSCORE;
				}
				else
				{
					//else set the game state to the title screen
					g_GameState = GameState::TITLEBEGIN;
				}

				done = 1;

				break;
		}

		//Check if the game was ended
		if (Level->m_levelEnded == true)
		{
			//I'll check if we lost all lives or we completed the level
			//If all lives are lost then we'll check for a high score.
			//If no high score then return to the tile screen.
			//Otherwise, if the level is complete, check if all levels are
			//complete. If so then show an end sequence. Otherwise go to the
			//level selector.

			g_GameState = GameState::TITLEBEGIN;

			//Delete the class instances (In deconstructor later)

			//End the loop
			done = 1;
		}
	}

	// disable map backgrounds
	REG_DISPCNT &= ~(DCNT_BG1 | DCNT_BG2);
}
