#include "level_selector.h"

#include <tonc.h>
#include <vector>

#include "gameDefines.h"
#include "globalvars.h"
#include "text.h"
#include "fader.h"
#include "sfx.h"

//Graphics data
#include "spr_levelselect_gfx.h"

//Declarations

//implementation of member functions

CLevelSelector::CLevelSelector() //Constructor
{
	//Set initial member variables
	m_highlightedLevel = 1;
	m_selectedLevel = -1;
	m_selectPos = 0;
	m_page = 0;
	m_endScreenPage = 0;

}

void CLevelSelector::Init()
{

	//Cancel any sounds that are playing
	mmEffectCancelAll();

	//Load the sprite tiles
	LZ77UnCompVram(spr_levelselect_gfx, tile_mem[4]);

	//If level 5 is not completed yet, show the level selector
	//otherwise we'll be showing the ending screen instead.

	if (g_completedLevels[4] == false)
	{
		//Init but hide the sprites
		//Down arrow
		obj_set_attr(&g_obj_buffer[1], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32,
		             ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(16));
		obj_set_pos(&g_obj_buffer[1], 0, 152);
		obj_hide(&g_obj_buffer[1]);
		//Up arrow
		obj_set_attr(&g_obj_buffer[2], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32,
		             ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(32));
		obj_set_pos(&g_obj_buffer[2], 0, 0);
		obj_hide(&g_obj_buffer[2]);

		//Get the level text for all levels
		InitLevelText();

		//Display the levels
		DisplayLevels();
	}
	else
	{
		ShowEndScreen(0);
	}

	//Fade in
	g_fader->apply(FadeType::IN, 30);
}

void CLevelSelector::Update()
{
	int done = 0;

	while (!done)
	{
		//Check the input and change/begin the selected level.
		key_poll();

		//If levels 1-4 are not all completed then show them and allow the
		//user to select levels.
		if ((g_completedLevels[0] == false) || (g_completedLevels[1] == false) ||
		    (g_completedLevels[2] == false) || (g_completedLevels[3] == false))
		{
			if ((key_is_down(KEY_A)) || (key_is_down(KEY_B)) || (key_hit(KEY_START)))
			{
				//Exit the while loop if this level has not already
				//been completed
				if (g_completedLevels[m_selectPos] == false)
				{
					done = 1;
				}
			}

			if (key_hit(KEY_DOWN))
			{
				//Make sure we're not at the last level,
				if (m_selectPos < m_levelText.size() - 1)
				{
					//If we're at the bottom of a page, scroll to the next page if there
					//are more levels, and move the player sprite to the top position.
					if ((m_selectPos % 3) == 2)
					{
						if (m_selectPos < m_levelText.size())
						{
							m_selectPos++;
							m_page++;
							DisplayLevels();
						}
					}
					else
					{
						m_selectPos++;
						//Change the player sprite position
						obj_set_pos(&g_obj_buffer[0], 0, 24 + ((m_selectPos % 3) * 48));
					}
				}
			}
			if (key_hit(KEY_UP))
			{
				//Make sure we're not at the first level
				if (m_selectPos > 0)
				{
					//If we're at the top of a page, scroll to the next page if this is
					//not the first page, and move the player sprite to the top position.
					if ((m_selectPos % 3 == 0) && (m_page > 0))
					{
						m_selectPos--;
						m_page--;
						DisplayLevels();
					}
					else
					{
						m_selectPos--;
						//Change the player sprite position
						obj_set_pos(&g_obj_buffer[0], 0, 24 + ((m_selectPos % 3) * 48));
					}
				}
			}

			//Set the selected level variable.
			m_selectedLevel = m_selectPos;

		}
		else
		{
			//Else we only need to poll A,B and START
			if ((key_hit(KEY_A)) || (key_hit(KEY_B)) || (key_hit(KEY_START)))
			{
				//If level 5 is not yet completed, begin that level.
				//Otherwise we're at the end screen and we'll check for
				//a high score or go to the title screen.
				if (g_completedLevels[4] == false)
				{
					//Set the selected level
					m_selectedLevel = 4;
					done = 1;
				}
				else
				{
					//We're at the end screen
					if (m_endScreenPage == 0)
					{
						//Fade out
						g_fader->apply(FadeType::OUT, 30);
						m_endScreenPage++;
						ShowEndScreen(m_endScreenPage);
						//Fade in
						g_fader->apply(FadeType::IN, 30);
					}
					else
					{
						//If we got a high score, set the game state to the
						//high score entry
						if (g_score > g_highScores[9].score)
						{
							g_GameState = GS_ENTERHIGHSCORE;
						}
						else
						{
							//else set the game state to the title screen
							g_GameState = GS_TITLEBEGIN;
						}
						m_selectedLevel = 0; //Set to >= 0 so the main loop is exited.
						done = 1;
					}
				}
			}
		}

		VBlankIntrWait(); //Wait for a VBL before updating the screen

		//Copy the OAM data
		oam_copy(oam_mem, g_obj_buffer, 4); //There could be up to 4 sprites on screen
	}
}

int LevelSelectorMain(CLevelSelector* LevelSelector)
{
	int done = 0;

	VBlankIntrWait();

	LevelSelector->Init();

	while (!done)
	{
		//Update the level selector
		LevelSelector->Update();

		//Check if a level was selected
		if (LevelSelector->m_selectedLevel >= 0)
		{
			//If level 5 is completed, we'll be returning to the
			//high score table or the title after this.
			if (g_completedLevels[4] == false)
			{
				g_currentLevel = LevelSelector->m_selectedLevel;
			}

			//End the loop
			done = 1;
		}
	}

	//Begin the selected level. If selected level is -1 then that means
	//we completed all 5 levels and we'll be going to the high score
	//table or the title screen.
	if ((LevelSelector->m_selectedLevel >= 0) && (g_completedLevels[4] == false))
	{
		//Set the game state to begin the selected level
		g_GameState = GS_LEVELBEGIN;
	}

	//Fade to black
	g_fader->apply(FadeType::OUT, 30);

	return 0;
}

void CLevelSelector::InitLevelText()
{
	TLevelText tempText;

	//Clear the vector
	m_levelText.clear();

	//If first 4 levels are not complete, show the text for those
	if ((g_completedLevels[0] == false) || (g_completedLevels[1] == false) ||
	    (g_completedLevels[2] == false) || (g_completedLevels[3] == false))
	{
		//Level 1
		tempText.lines[0] = "Tomb 1:                    ";
		tempText.lines[1] = "The tomb Of Hrxmyoply II   ";
		tempText.lines[2] = "This tomb is an early      ";
		tempText.lines[3] = "design and is best         ";
		tempText.lines[4] = "described as 'tame'.       ";
		m_levelText.push_back(tempText);
		//Level 2
		tempText.lines[0] = "Tomb 2:                    ";
		tempText.lines[1] = "The Tomb Of Bathsheba      ";
		tempText.lines[2] = "A Tomb which contains some ";
		tempText.lines[3] = "lovely Ancient Egyptian    ";
		tempText.lines[4] = "plumbing.                  ";
		m_levelText.push_back(tempText);
		//Level 3
		tempText.lines[0] = "Tomb 3:                    ";
		tempText.lines[1] = "The Tomb Of Skewr          ";
		tempText.lines[2] = "Festooned with lots of     ";
		tempText.lines[3] = "pointy bits, impaling for  ";
		tempText.lines[4] = "the use of.                ";
		m_levelText.push_back(tempText);
		//Level 4
		tempText.lines[0] = "Tomb 4:                    ";
		tempText.lines[1] = "The Tomb Of Nazti          ";
		tempText.lines[2] = "Almost as much fun as being";
		tempText.lines[3] = "bitten by an asp, but not  ";
		tempText.lines[4] = "quite.                     ";
		m_levelText.push_back(tempText);
		//Level 5 (When cheat mode enabled)
		if (g_cheatEnabled[0] == true)
		{
			tempText.lines[0] = "Tomb 5:                    ";
			tempText.lines[1] = "                           ";
			tempText.lines[2] = "                           ";
			tempText.lines[3] = "                           ";
			tempText.lines[4] = "                           ";
			m_levelText.push_back(tempText);
		}
	}
}

void CLevelSelector::DisplayLevels()
{

	u32 n, m;
	int x;
	int ix, iy;
	std::vector<TLevelText>::iterator it;

	//Set all text layer tiles to black (space characters)
	for (iy = 0; iy < 256; iy += 8)
	{
		for (ix = 0; ix < 256; ix += 8)
		{
			txt_putc(ix, iy, 32); //Space(Black)
		}
	}

	//Show level 1 to 4 if they're not all completed
	if ((g_completedLevels[0] == false) || (g_completedLevels[1] == false) ||
	    (g_completedLevels[2] == false) || (g_completedLevels[3] == false))
	{
		txt_puts(0, 0, "   Select The Tomb You Want   ");
		txt_puts(0, 8, "          To Die In           ");

		//Display the current "page" of levels. A page is
		//a set of 3 levels. I do it this way to allow for
		//having lots of extra levels in the future.
		n = m_page * 3;
		m = n + 3;
		it = m_levelText.begin() + n;

		while ((n < m) && n < m_levelText.size()) //We can fit 3 level texts on screen at once
		{
			for (x = 0; x < 5; x++)
			{
				//Display regular text if the level is not yet complete, or
				//faded text if it is complete.
				if (g_completedLevels[n] == false)
				{
					txt_puts(24, 24 + ((n % 3) * 48) + (x * 8), it->lines[x]);
				}
				else
				{
					txt_puts_faded(24, 24 + ((n % 3) * 48) + (x * 8), it->lines[x]);
				}
			}
			n++;
			it = m_levelText.begin() + n;
		}

		//If there's more levels below than can be shown, show the down arrow
		if (m_levelText.size() > n)
		{
			obj_unhide(&g_obj_buffer[1], 0);
		}
		else
		{
			obj_hide(&g_obj_buffer[1]);
		}

		//If there are more levels above than can be shown, show the up arrow.
		if (n > 3)
		{
			obj_unhide(&g_obj_buffer[2], 0);
		}
		else
		{
			obj_hide(&g_obj_buffer[2]);
		}

		//Display the player sprite
		obj_set_attr(&g_obj_buffer[0], ATTR0_SQUARE | ATTR0_4BPP | ATTR0_MODE(0), ATTR1_SIZE_32,
		             ATTR2_PALBANK(0) | ATTR2_PRIO(0) | ATTR2_ID(0));
		obj_set_pos(&g_obj_buffer[0], 0, 24 + ((m_selectPos % 3) * 48));

		//Copy the OAM data
		oam_copy(oam_mem, g_obj_buffer, 4); //There could be up to 4 sprites on screen
	}
	else
	{
		//Show level 5 text only
		txt_puts(0, 0,   " So This Is The Tomb You Want ");
		txt_puts(0, 8,   "          To Die In           ");
		txt_puts(0, 16,  "                              ");
		txt_puts(0, 24,  "Tomb 5: The Tomb of B'Stard   ");
		txt_puts(0, 32,  "The tomb that put the         ");
		txt_puts(0, 40,  "'Aaargh!'  into  architecture,");
		txt_puts(0, 48,  "this  underground  pyramid and");
		txt_puts(0, 56,  "associated rooms of certain - ");
		txt_puts(0, 64,  "or at  least  highly  likely -");
		txt_puts(0, 72,  "death  is a delight  for those");
		txt_puts(0, 80,  "lucky enough  not to be in it.");
		txt_puts(0, 88,  "Tomb Trivia: This building was");
		txt_puts(0, 96,  "the  winner  of  the 1873 B.C.");
		txt_puts(0, 104, "Ideal Tomb Exhibition.        ");
		txt_puts(0, 112, "Hints:  Rocks  destroy spikes,");
		txt_puts(0, 120, "hot bricks  burn  through wood");
		txt_puts(0, 128, "and  cool down  in water, cool");
		txt_puts(0, 136, "bricks heat up in fire.       ");
		txt_puts(0, 144, "Spikes obstruct hot and cold  ");
		txt_puts(0, 152, "bricks.                       ");
	}
}

void CLevelSelector::ShowEndScreen(int page)
{
	if (page == 0)
	{
		txt_puts(0, 0,   "   EXTREMELY WELL DONE!!!!!   ");
		txt_puts(0, 8,   " (Or as the Ancient Egyptians ");
		txt_puts(0, 16,  " would have put it, 'Y JMMY   ");
		txt_puts(0, 24,  " SD!')                        ");
		txt_puts(0, 32,  "                              ");
		txt_puts(0, 40,  "So, you've battled your way   ");
		txt_puts(0, 48,  "through five levels so        ");
		txt_puts(0, 56,  "cunning you could cleave      ");
		txt_puts(0, 64,  "coconuts with them and what do");
		txt_puts(0, 72,  "you get for your efforts? Just");
		txt_puts(0, 80,  "two pages of text.            ");
		txt_puts(0, 88,  "                              ");
		txt_puts(0, 96,  "Ah well, that's life. You put ");
		txt_puts(0, 104, "fifty quid in a fruit machine ");
		txt_puts(0, 112, "and all you get is a yellow   ");
		txt_puts(0, 120, "plastic token. And yellow     ");
		txt_puts(0, 128, "isn't even your favourite     ");
		txt_puts(0, 136, "colour.                       ");
		txt_puts(0, 144, "                              ");
		txt_puts(0, 152, "          NEXT PAGE           ");
	}
	else
	{
		txt_puts(0, 0,   "                              ");
		txt_puts(0, 8,   "  It's at times like this when");
		txt_puts(0, 16,  "I'm sure we can all appreciate");
		txt_puts(0, 24,  "that old Egyptian saying: 'The");
		txt_puts(0, 32,  "man who sleeps in a stone     ");
		txt_puts(0, 40,  "pryamid may get a sore back,  ");
		txt_puts(0, 48,  "but at least his razor will be");
		txt_puts(0, 56,  "sharp in the morning.'        ");
		txt_puts(0, 64,  "                              ");
		txt_puts(0, 72,  "Anyroad, onto the high-score  ");
		txt_puts(0, 80,  "table now - and hey, lets keep");
		txt_puts(0, 88,  "it clean. NC");
		txt_puts(0, 96,  "                              ");
		txt_puts(0, 104, "                              ");
		txt_puts(0, 112, "                              ");
		txt_puts(0, 120, "                              ");
		txt_puts(0, 128, "                              ");
		txt_puts(0, 136, "                              ");
		txt_puts(0, 144, "                              ");
		txt_puts(0, 152, "                              ");
	}
}
