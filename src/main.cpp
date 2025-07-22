/*********************************************************************************
 * Includes
 ********************************************************************************/
#include "main.h"

#include <tonc.h>
#include <string.h>

#include "gameDefines.h"
#include "text.h"
#include "title.h"
#include "level_selector.h"
#include "level.h"
#include "highscore_entry.h"
#include "sfx.h"
#include "fader.h"

//Background and sprite palettes.
#include "pal_bg.h"
#include "pal_oam.h"
#include "pal_font.h"

/*********************************************************************************
 * Globals
 ********************************************************************************/
bool g_NewFrame = true;
GameState g_GameState = GameState::TITLEBEGIN; //The game initially begins at the title screen
int g_lives;          //These must be set before the game starts and during the game so
unsigned int g_score; //they have to be global
bool g_cheatEnabled[4] = {false, false, false, false};
int g_currentLevel; //The level number currently being played
bool g_completedLevels[5]; //Lets the game know which levels have been completed.
THighScore g_highScores[10]; //List of high scores
OBJ_ATTR g_obj_buffer[128]; // Buffer to store OAM data for sprites, copied to OAM once per VBL
std::unique_ptr<CFader> g_fader; //For fading the palette

/*********************************************************************************
 * Program entry point
 ********************************************************************************/
int main(void)
{
	int n;

#if DEBUG
	g_cheatEnabled[0] = true;
#endif

	//Set the palettes
	memcpy16(pal_bg_mem, pal_bg, pal_bg_size / 2);
	memcpy16(pal_obj_mem, pal_oam, pal_oam_size / 2);
	memcpy16(pal_bg_mem + 240, pal_font, pal_font_size / 2); //Palbank 15
	//Initialise the text system (background 0, SBB 26, prio 0)
	txt_init(0, 26, 0); //Text

	u16 m_palBuffer[512];
	//Save a copy of the palette in m_paletteBuffer
	u16 * Src = pal_bg_mem;
	u16 * Dest = (u16*)m_palBuffer;
	for (n = 0; n < 512; n++)
	{
		*(Dest++) = *(Src++);
	}
	g_fader = std::make_unique<CFader>(m_palBuffer);

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
	mmInitDefault((mm_addr)soundbank_bin, 4);

	//Initialise oam
	oam_init(g_obj_buffer, 128);

	//Try to load the high score data from SRAM
	HighScores::Load();

	//Set the display register
	REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_OBJ | DCNT_OBJ_1D;

	//Main game loop
	while (true)
	{
		//**********Main game structure here*******
		switch (g_GameState) //Check the global game state
		{
			case GameState::TITLEBEGIN:
				//Change the game state to GS_WAIT
				g_GameState = GameState::WAIT;

				//Run the titlescreen main loop
				CTitle::Main();
				break;

			case GameState::LEVELSELECT:
				//Change the game state to GS_WAIT
				g_GameState = GameState::WAIT;

				//Run the level selector main loop
				CLevelSelector::Main();
				break;

			case GameState::LEVELBEGIN:
				//Change the game state to GS_WAIT
				g_GameState = GameState::WAIT;

				//Run the game main loop
				CLevel::Main(g_currentLevel);
				break;

			case GameState::ENTERHIGHSCORE:
				//Change the game state to GS_WAIT
				g_GameState = GameState::WAIT;

				//Run the highscore entry main loop
				CHighScore_Entry::Main();
				break;

			case GameState::WAIT:
				//Dummy state. Should never actually be reached.
				break;
		}
	}

	return 0;
}

/* END OF FILE */
