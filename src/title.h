#ifndef TITLE_H
#define TITLE_H

#include <tonc.h>
#include "gameDefines.h"

//Defines
#define SECTION_TITLESCREEN 0
#define SECTION_HIGHSCORES 1
#define SECTION_INSTRUCTIONS 2
#define SECTION_RESET_SCORES 3
#define TITLE_FALL_SPEED 0x060
#define TITLE_ACCELERATION 0x0A
#define SCROLL_SPEED 0x080

class CTitle
{
public:
	//Properties
	bool m_gameStarted;
	int m_titleSection; /*Whether we're on the title screen,
	                     high scores or instructions.*/
	bool m_gotoNextSection;

	bool m_titleBarAppeared;
	FIXED title_y;
	FIXED title_dy; //Title falling speed
	FIXED title_ay; //Falling acceleration
	int m_bounces;
	bool m_animDone;
	int m_waitTime;
	const char* helpText[60];
	const char* blankLine;
	FIXED m_scrollCounter;
	int m_nextLine;
	int m_scrollPos;
	int m_spriteCount;
	int m_cheatModeSeq[8];
	int m_cheatModePos;
	int m_selectHeldFrames;

	//Main loop for when at the title screen
	static void Main();

	//member functions
	//constructor
	CTitle();
	void Init(); //Initialises the titlescreen
	void InitHelpText();
	void Update(); //Updates the titlescreen
	//Update the three title sections
	void UpdateTitleScreen();
	void UpdateHighScoreScreen();
	void UpdateHelpScreen();
	void UpdateScoreResetScreen();
	void PlaceSprite(int oam, int sprite, int x, int y);
	void CheatTest();
};

#endif
