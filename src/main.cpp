/*********************************************************************************
c * Includes
 ********************************************************************************/
#include "main.h"

#include <tonc.h>
#include <maxmod.h>
#include <string.h>

#include "gameDefines.h"
#include "text.h"
#include "title.h"
#include "level_selector.h"
#include "level.h"
#include "highscore_entry.h"
#include "soundbank_bin.h"

//Background and sprite palettes.
#include "gfx/pal_bg.h"
#include "gfx/pal_oam.h"

/*********************************************************************************
 * Globals
 ********************************************************************************/
bool g_NewFrame = true;
int g_GameState = GS_TITLEBEGIN; //The game initially begins at the title screen
int g_lives;          //These must be set before the game starts and during the game so
unsigned int g_score; //they have to be global
bool g_cheatEnabled[4] = {false, false, false, false};
int g_currentLevel; //The level number currently being played
bool g_completedLevels[5]; //Lets the game know which levels have been completed.
THighScore g_highScores[10]; //List of high scores
OBJ_ATTR g_obj_buffer[128]; //Buffer to store OAM data for sprites, copied to OAM once
                          //per VBL


/*********************************************************************************
 * Program entry point
 ********************************************************************************/
int main(void)
{

	int n;

	//Declare class instances
	CTitle* Title; //Title sequence class instance
	CHighScore *HighScore; //High score entry screen class
	CLevelSelector* LevelSelector; //Level selector class instance
	CLevel* Level; //Game level class instance

	//Set the palettes
	GRIT_CPY(pal_bg_mem, pal_bgPal);
	GRIT_CPY(pal_obj_mem, pal_oamPal);
	GRIT_CPY(pal_bg_mem + 240, entombed_fontPal); //Palbank 16
	//Initialise the text system (background 0, SBB 26, prio 0)
	txt_init(0, 26, 0); //Text

	u16 m_palBuffer[512];
	//Save a copy of the palette in m_paletteBuffer
	u16 * Src = pal_bg_mem;
	u16 * Dest = (u16 *)m_palBuffer;
	for (n = 0; n<512; n++) {
		*(Dest++) = *(Src++);
	}

	//Reset the completed levels global array
	//TODO: In the future allow for more levels
	//perhaps have a total levels value passed
	//from the map and use a vector of bools for
	//completed levels.
	for (n = 0; n < 5; n++)
	{
		g_completedLevels[n] = false;
	}

   //Start VBL Interrupt
	irq_init(NULL);
	irq_add(II_VBLANK, mmVBlank); //Attach maxmod ISR for sound.
	irq_enable(II_VBLANK);

	// Initialise maxmod with soundbank and 4 channels
	mmInitDefault( (mm_addr)soundbank_bin, 4 );

	//Initialise oam
	oam_init(g_obj_buffer, 128);

    //Try to load the high score data from SRAM
	n = LoadScores();

    //Main game loop
    while(true)
    {
 	
        //**********Main game structure here*******
		switch (g_GameState) //Check the global game state
		{
			case GS_TITLEBEGIN:

				//Initialize the title screen class
				Title = new CTitle;

				//Change the game state to GS_WAIT
				g_GameState = GS_WAIT;

				//Run the titlescreen main loop
				TitleMain(Title, m_palBuffer, g_highScores);

				//Delete the title class instance
				delete Title;
			break;

			case GS_LEVELSELECT:

				//Initialize the level selector class
				LevelSelector = new CLevelSelector;

				//Change the game state to GS_WAIT
				g_GameState = GS_WAIT;

				//Run the level selector main loop
				LevelSelectorMain(LevelSelector, m_palBuffer);

				//Delete the level selector class instance
				delete LevelSelector;
			break;

			case GS_LEVELBEGIN:

				//Initialize the level class
				Level = new CLevel;

				//Change the game state to GS_WAIT
				g_GameState = GS_WAIT;

				//Run the game main loop
				LevelMain(Level, g_currentLevel, m_palBuffer);

				//Delete the level class instance
				delete Level;

			break;

			case GS_ENTERHIGHSCORE:

				//Initialize the highscore entry class
				HighScore = new CHighScore;

				//Change the game state to GS_WAIT
				g_GameState = GS_WAIT;

				//Run the highscore entry main loop
				HighScoreMain(HighScore, m_palBuffer);

				//Delete the highscore entry class instance
				delete HighScore;
			break;

			case GS_WAIT:
				//Dummy state. Should never actually be reached.
			break;
		}
    }
    return 0;
}

int LoadScores()
{
	int n, m;
	u8 *src;

	//Start at byte position 5 in sram because the first byte of sram can sometimes
	//become corrupted when powering on/off.

    //Check for magic number
    if ( (*(u8 *) ( sram_mem + 4  ) != 'E') ||
         (*(u8 *) ( sram_mem + 5 ) != 'N') ||
         (*(u8 *) ( sram_mem + 6 ) != 'T') ||
         (*(u8 *) ( sram_mem + 7 ) != 'D') )
    {
        //Magic number not found, so set the high scores to default values and return.
		strcpy(g_highScores[0].name,"KING TUT-TUT"); g_highScores[0].score = 100000;
		strcpy(g_highScores[1].name,"RUBBERTITI"); g_highScores[1].score = 90000;
		strcpy(g_highScores[2].name,"INDY JONES"); g_highScores[2].score = 80000;
		strcpy(g_highScores[3].name,"DOC PHIBES"); g_highScores[3].score = 70000;
		strcpy(g_highScores[4].name,"PTEPIC"); g_highScores[4].score = 60000;
		strcpy(g_highScores[5].name,"RICK O'BRIEN"); g_highScores[5].score = 50000;
		strcpy(g_highScores[6].name,"ALAN PARSONS"); g_highScores[6].score = 40000;
		strcpy(g_highScores[7].name,"ALBERT SPEER"); g_highScores[7].score = 30000;
		strcpy(g_highScores[8].name,"DUB VULTURE"); g_highScores[8].score = 15000;
		strcpy(g_highScores[9].name,"ZOB"); g_highScores[9].score = 5000;
        return 1;
    }

    //If we got here then load place the SRAM data in the relevant places
	//The high score data is saved in in SRAM in the following format: First there are
	//the 10 Name values at 12 bytes each. After that there are the 10 score values at
	//4 bytes each
    src = sram_mem + 8;

	//Load the names
	for (n = 0; n < 10; n++)
	{
		for (m = 0; m < 12; m++)
		{
			g_highScores[n].name[m] = *src++;
			//If the character is out of range for some reason(save corruption?), change it to a space.
			if ((g_highScores[n].name[m] < 32) || (g_highScores[n].name[m] > 90))
			{
				g_highScores[n].name[m] = 32;
			}
		}
	}

	//Load the scores
	for (n = 0; n < 10; n ++)
	{
		//Scores are 32 bit so bit shifting needed
		g_highScores[n].score = 0;
		g_highScores[n].score += (*src++ << 24) ;
		g_highScores[n].score += (*src++ << 16);
		g_highScores[n].score += (*src++ << 8);
		g_highScores[n].score += *src++;

		//If for some reason the score is out of range then put it back in range
		if (g_highScores[n].score > 999999) {g_highScores[n].score = 999999;}
	}
    return 0;
}

/* END OF FILE */

