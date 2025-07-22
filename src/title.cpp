#include "title.h"

#include <string.h>
#include <tonc.h>

#include "gameDefines.h"
#include "main.h"
#include "highscore_entry.h"
#include "text.h"
#include "fader.h"
#include "posprintf.h"
#include "sfx.h"

//Graphics data
#include "spr_title_gfx.h"

//Declarations

static const char* author_text =
    R"(  (c) 1991 Nick Concannon
Conversion 2015 Stephen Earl
 Additions 2025 carstene1ns)";

static const char* reset_highscore_text =
    R"(DO YOU WANT TO RESET THE
HIGH SCORES?
L - NO           R - YES)";

//implementation of member functions

CTitle::CTitle() //Constructor
{
	//Set initial member variables
	m_titleSection = SECTION_TITLESCREEN;
	m_gotoNextSection = false;
	m_gameStarted = false;

	m_titleBarAppeared = false;
	title_y = -32 << 8;
	title_dy = 0;
	title_ay = TITLE_ACCELERATION;
	m_bounces = 0;
	m_animDone = false;
	m_waitTime = 0;
	m_scrollCounter = 0;
	m_nextLine = 0;
	m_scrollPos = 0;
	blankLine = "";
	m_spriteCount = 0;
	m_selectHeldFrames = 0; /*Frames that select is held for. Used for
	                          resetting the high score table.*/

	//Cheat sequence array (UUDDLRLR)
	m_cheatModeSeq[0] = 0;
	m_cheatModeSeq[1] = 0;
	m_cheatModeSeq[2] = 1;
	m_cheatModeSeq[3] = 1;
	m_cheatModeSeq[4] = 2;
	m_cheatModeSeq[5] = 3;
	m_cheatModeSeq[6] = 2;
	m_cheatModeSeq[7] = 3;
	m_cheatModePos = 0;
}

void CTitle::Init()
{
	//Load sprite data for the intro sequence
	LZ77UnCompVram(spr_title_gfx, tile_mem[4]);

	//Load the help text
	InitHelpText();
}

void CTitle::Update()
{
	int n;

	//Check input
	key_poll();

	//Test for cheat code combination
	CheatTest();

	//Start the game if A, B or START is pressed
	if ((key_is_down(KEY_A)) || (key_is_down(KEY_B)) || (key_hit(KEY_START)))
	{
		//Cancel any sounds that are playing
		mmEffectCancelAll();
		m_gameStarted = true;

		//If start was pressed and the chead code was entered correctly,
		//play a sound and enable the cheat mode.
		if ((key_is_down(KEY_START)) && (m_cheatModePos == 8))
		{
			g_cheatEnabled[0] = true;
			mmEffect(SFX_COIN);
		}
	}
	else
	{
		//Holding select for a given time will allow you to reset the high scores.
		if (key_is_down(KEY_SELECT))
		{
			m_selectHeldFrames++;
		}
		if (key_released(KEY_SELECT))
		{
			//If select was held for 600 frames or more (10 seconds) then
			//allow the player to reset the high scores.
			if (m_selectHeldFrames >= 600)
			{
				//Remove any sprites
				for (n = 0; n < 18; n++)
				{
					obj_hide(&g_obj_buffer[n]);
				}
				oam_copy(oam_mem, g_obj_buffer, 18);

				//Set scroll position to initial values
				REG_BG_OFS[0].x = 0;
				REG_BG_OFS[0].y = 9;

				txt_clear_screen();

				//Display the reset scores text
				txt_margin_left(16);
				txt_puts(16, 64, reset_highscore_text);
				txt_default_margins();

				m_titleSection = SECTION_RESET_SCORES;
			}
			m_selectHeldFrames = 0;
		}

		mmFrame();
		VBlankIntrWait(); //Wait for a VBL before updating the screen

		//Check what title section we're in
		switch (m_titleSection)
		{
			case SECTION_TITLESCREEN:
				UpdateTitleScreen();
				break;
			case SECTION_HIGHSCORES:
				UpdateHighScoreScreen();
				break;
			case SECTION_INSTRUCTIONS:
				UpdateHelpScreen();
				break;
			case SECTION_RESET_SCORES:
				UpdateScoreResetScreen();
				break;
		}
	}

	//Copy the OAM data
	oam_copy(oam_mem, g_obj_buffer, 50); //First 50 objects (Should be more than enough)
}

void CTitle::UpdateTitleScreen()
{
	int n;
	char tempStr[25];

	//Fades from black with bar near top of screen.
	if (!m_titleBarAppeared)
	{
		//reset palette
		g_fader->clear();

		txt_clear_screen();

		//Place the title underline sprites on the screen
		obj_set_attr(&g_obj_buffer[0], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32,
		             ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(112));
		for (n = 1; n <= 6; n++)
		{
			obj_set_attr(&g_obj_buffer[n], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32,
			             ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(128));
		}
		obj_set_attr(&g_obj_buffer[7], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32,
		             ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(144));
		for (n = 0; n <= 7; n++)
		{
			obj_set_pos(&g_obj_buffer[n], 24 + (n * 24), 48);
		}
		//Copy the OAM data for the title underline
		oam_copy(oam_mem, g_obj_buffer, 8);

		//Fade in
		g_fader->apply(FadeType::IN, 30);

		//Place the title sprites just out of the top of the screen
		//Loop for ENTOMB, then E, D.
		for (n = 0; n < 7; n++)
		{
			obj_set_attr(&g_obj_buffer[n + 8], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32,
			             ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(0 + (16 * n)));
		}
		obj_set_attr(&g_obj_buffer[14], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32,
		             ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(0));
		obj_set_attr(&g_obj_buffer[15], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32,
		             ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(96));
		for (n = 0; n < 8; n++)
		{
			obj_set_pos(&g_obj_buffer[n + 8], 24 + (n * 24), title_y >> 8);
		}

		m_titleBarAppeared = true;
		title_dy = TITLE_FALL_SPEED;
	}

	if ((m_titleBarAppeared) && (!m_animDone) && (m_waitTime == 0))
	{
		title_dy += title_ay;
		title_y += title_dy;
		//Update the title falling animation
		for (n = 8; n < 16; n++)
		{
			BFN_SET(g_obj_buffer[n].attr0, title_y >> 8, ATTR0_Y);
		}
		if ((title_y >> 8) > 24)
		{
			title_dy = 0 - ((title_dy / 5) * 4);
			m_bounces++;
			//Play title bounce sound
			mmEffect(SFX_TITLE);
			title_y = 24 << 8;
			BFN_SET(g_obj_buffer[n].attr0, title_y >> 8, ATTR0_Y);

			if (m_bounces == 6)
			{

				m_animDone = true;

				//Show the pic and copyright and set the timer
				obj_set_attr(&g_obj_buffer[16], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32,
				             ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(160));
				obj_set_attr(&g_obj_buffer[17], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32,
				             ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(176));
				obj_set_pos(&g_obj_buffer[16], 104, 70);
				obj_set_pos(&g_obj_buffer[17], 104, 94);

				//Write the text at the bottom
				txt_margin_left(8);
				txt_puts(8, 136, author_text);
				txt_default_margins();

				m_waitTime = 300;
			}
		}
	}

	if (m_animDone)
	{
		if (m_waitTime > 0)
		{
			//Decrement the wait timer
			m_waitTime--;
		}
		else
		{
			//Title animation has ended and the timer has counted down
			//Fade out and set up the high score display
			g_fader->apply(FadeType::OUT, 30);

			//Remove the sprites
			for (n = 0; n < 18; n++)
			{
				//Hide the object
				obj_hide(&g_obj_buffer[n]);
			}

			oam_copy(oam_mem, g_obj_buffer, 18);

			//Blank out the text from the title screen
			txt_clear_rect(8, 136, 224, 24);

			//***Setup the high score screen***
			txt_puts(72, 16, "TOMB TONKERS");
			for (n = 0; n < 10; n++)
			{
				posprintf(tempStr, "%06l......%s", g_highScores[n].score, g_highScores[n].name);
				txt_puts(24, 48 + (n * 8), tempStr);
			}

			//Fade in
			g_fader->apply(FadeType::IN, 30);

			m_waitTime = 300;
			m_titleSection = SECTION_HIGHSCORES;
		}
	}
}

void CTitle::UpdateHighScoreScreen()
{
	int n;
	if (m_waitTime > 0)
	{
		//Decrement the wait timer
		m_waitTime--;
	}
	else
	{
		//Fade out and setup the help screen
		g_fader->apply(FadeType::OUT, 30);

		// Clear whole text background
		txt_clear_bg();

		//Set scroll position to initial values
		REG_BG_OFS[0].x = 0;
		REG_BG_OFS[0].y = 9;

		//Fill the whole screen block except the top row with text (31 lines)
		for (n = 1; n < 32; n++)
		{
			txt_puts(40, n * 8, helpText[n - 1]);
		}

		//Place the initial sprites
		PlaceSprite(0, TITLE_SPRITE_BOW, 8, 56);
		PlaceSprite(1, TITLE_SPRITE_QUIVER, 8, 120);
		PlaceSprite(2, TITLE_SPRITE_KEY, 8, 152);
		oam_copy(oam_mem, g_obj_buffer, 3);

		//Fade in
		g_fader->apply(FadeType::IN, 30);

		m_nextLine = 31; //Used for the scrolling help text.
		m_scrollCounter = 0; //Controls the text scoll speed
		m_scrollPos = 8; //Stores the scroll position (Just after the first screenblock row)
		m_spriteCount = 3;
		m_titleSection = SECTION_INSTRUCTIONS;
	}
}

void CTitle::UpdateHelpScreen()
{
	int n, m;

	if ((m_scrollCounter >> 8) >= 1)
	{
		m_scrollCounter = 0;

		//As the text scrolls up and out of the screen, remove the top line that just went of
		//the screen, add a line just off the bottom of the screen, scroll one tile height (8 pixels)
		//then shift all tiles down by one tile. And repeat.
		//NB: We do mod 264 here instead of mod 256 because we're scrolling from tile position 1
		//to tile "33". i.e. the top line of the map is always just out of the top of the screen
		// and we load the next line into that one.
		if (m_nextLine < 84)
		{
			if (m_scrollPos % 8 == 0)
			{
				n = m_scrollPos % 264;

				//Clear the line
				txt_clear_rect(40, n - 8, 184, 8);

				if (m_nextLine <= 59)
				{
					//Add the next line to the screenblock, if lines are available
					txt_puts(40, n - 8, helpText[m_nextLine]);
				}
				m_nextLine++;
			}

			//Scroll up a pixel
			REG_BG_OFS[0].y = m_scrollPos;
			m_scrollPos++;

			//Scroll the sprites
			for (n = 0; n < m_spriteCount; n++)
			{
				BFN_SET(g_obj_buffer[n].attr0, BFN_GET(g_obj_buffer[n].attr0 - 1, ATTR0_Y), ATTR0_Y);
			}

			//If the top sprite went off the screen, move all further
			//sprites back by one object if there are any.
			//(There will be)
			if (BFN_GET(g_obj_buffer[0].attr0, ATTR0_Y) == (256 - 32))
			{
				if (m_spriteCount > 1)
				{
					for (m = 0; m < m_spriteCount - 1; m++)
					{
						oam_copy(&g_obj_buffer[m], &g_obj_buffer[m + 1], 1);
					}
					// and hide the last sprite
					obj_hide(&g_obj_buffer[m_spriteCount - 1]);
				}
				m_spriteCount--;
			}
			else
			{
				//Else just copy all the sprite data.
				oam_copy(oam_mem, g_obj_buffer, m_spriteCount);
			}

			//Use a switch to find when the next sprites need to appear
			switch (m_scrollPos)
			{
				case 40:
					PlaceSprite(m_spriteCount, TITLE_SPRITE_COIN, 8, 160);
					m_spriteCount++;
					break;
				case 72:
					PlaceSprite(m_spriteCount, TITLE_SPRITE_HOURGLASS, 8, 160);
					m_spriteCount++;
					break;
				case 128:
					PlaceSprite(m_spriteCount, TITLE_SPRITE_CHEST, 8, 160);
					m_spriteCount++;
					break;
				case 168:
					PlaceSprite(m_spriteCount, TITLE_SPRITE_URN, 8, 160);
					m_spriteCount++;
					break;
				case 224:
					PlaceSprite(m_spriteCount, TITLE_SPRITE_ANKH, 8, 160);
					m_spriteCount++;
					break;
			}
		}
		else
		{
			//Return to the title screen
			g_fader->apply(FadeType::OUT, 25);
			m_titleBarAppeared = false;
			m_animDone = false;
			m_waitTime = 0;
			title_y = -32 << 8;
			m_bounces = 0;
			REG_BG_OFS[0].y = 0;
			m_titleSection = SECTION_TITLESCREEN;
		}
	}
	else
	{
		m_scrollCounter += SCROLL_SPEED;
	}
}

void CTitle::UpdateScoreResetScreen()
{
	key_poll();

	if (key_hit(KEY_L))
	{
		//Return to the title screen
		g_fader->apply(FadeType::OUT, 25);
		m_titleBarAppeared = false;
		m_animDone = false;
		m_waitTime = 0;
		title_y = -32 << 8;
		m_bounces = 0;
		REG_BG_OFS[0].y = 0;
		m_titleSection = SECTION_TITLESCREEN;
	}

	if (key_hit(KEY_R))
	{
		//Reset the high scores and return to the title screen
		HighScores::Reset();
		g_fader->apply(FadeType::OUT, 25);
		m_titleBarAppeared = false;
		m_animDone = false;
		m_waitTime = 0;
		title_y = -32 << 8;
		m_bounces = 0;
		REG_BG_OFS[0].y = 0;
		m_titleSection = SECTION_TITLESCREEN;
	}
}

void CTitle::Main()
{
	auto Title = std::make_unique<CTitle>();

	int done = 0;

	mmFrame();
	VBlankIntrWait();

	Title->Init();

	while (!done)
	{
		//Update the titlescreen, if the update routine returns true then the loop
		//will end
		Title->Update();

		//Check if the game was started
		if (Title->m_gameStarted)
		{

			//Set global variables, 6 lives and 5 levels.
			g_lives = 6;
			g_score = 0;
			for (int n = 0; n < 5; n++)
			{
				g_completedLevels[n] = false;
			}

			//Fade to black
			g_fader->apply(FadeType::OUT, 30);

			//Reset the background scroll register
			REG_BG_OFS[0].y = 0;

			//Hide all sprites
			obj_hide_multi(g_obj_buffer, MAX_SPRITES);
			mmFrame();
			VBlankIntrWait();
			oam_copy(oam_mem, g_obj_buffer, MAX_SPRITES);

			//End the loop
			done = 1;
		}
	}

	if (Title->m_gameStarted)
	{
		//Set the game state to the level selector
		g_GameState = GameState::LEVELSELECT;
	}
}

void CTitle::InitHelpText()
{
	//Slightly modified from the original
	//text to fit on the GBA screen.
	for (int i = 0; i < 60; i++)
	{
		helpText[i] = blankLine;
	}

	helpText[2]  = "TREASURES OF THE TOMBS ";

	helpText[7]  = " Bow:";
	helpText[8]  = "  Collect  these so you";
	helpText[9]  = "  can fire  arrows. The";
	helpText[10] = "  more   bows collected";
	helpText[11] = "  the greater your shot";
	helpText[12] = "  power.";

	helpText[15] = " Quiver:";
	helpText[16] = "  Contains arrows.";

	helpText[19] = " Key:";
	helpText[20] = "  For  unlocking  doors";
	helpText[21] = "  and chests.";

	helpText[24] = "Gold coin:";
	helpText[25] = "  Collect  for  points.";

	helpText[28] = " Hourglass:";
	helpText[29] = "  Provides  you  with a";
	helpText[30] = "  few seconds that  you";
	helpText[31] = "  can use to delay tomb";
	helpText[32] = "  changes.";

	helpText[35] = " Chest:";
	helpText[36] = "  May contain something";
	helpText[37] = "  of use.";

	helpText[40] = " Urn:";
	helpText[41] = "  Fire  arrows at these";
	helpText[42] = "  to break  them  open.";
	helpText[43] = "  May contain something";
	helpText[44] = "  nice or nasty.";

	helpText[47] = " Ankh:";
	helpText[48] = "  Extra life.";

	helpText[51] = "CONTROLS:";
	helpText[52] = "D-Pad: Move/Climb/Jump";
	helpText[53] = "A: Shoot arrow";
	helpText[54] = "B: Look down";
	helpText[55] = "L: Select hourglass";
	helpText[56] = "   seconds.";
	helpText[57] = "R: Use hourglass";
	helpText[58] = "   Seconds.";
	helpText[59] = "Start: Pause/Unpause";
}

void CTitle::PlaceSprite(int oam, int sprite, int x, int y)
{
	obj_set_attr(&g_obj_buffer[oam], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0),
	             ATTR1_SIZE_32, ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(sprite * 16));
	obj_set_pos(&g_obj_buffer[oam], x, y);
}

void CTitle::CheatTest()
{
	int key = -1;

	//Check for cheat mode key combination
	if (key_released(KEY_UP))
	{
		key = 0;
	}
	if (key_released(KEY_DOWN))
	{
		key = 1;
	}
	if (key_released(KEY_LEFT))
	{
		key = 2;
	}
	if (key_released(KEY_RIGHT))
	{
		key = 3;
	}

	if (key >= 0)
	{
		if (m_cheatModePos < 8)
		{
			if (m_cheatModeSeq[m_cheatModePos] == key)
			{
				m_cheatModePos++;
			}
			else
			{
				m_cheatModePos = 0;
			}
		}
		else
		{
			m_cheatModePos = 0;
		}
	}
}
