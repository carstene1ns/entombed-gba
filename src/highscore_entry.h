#ifndef HIGHSCORE_ENTRY_H
#define HIGHSCORE_ENTRY_H

#include <tonc.h>
#include "gameDefines.h"

//Defines
#define LETTERS_INITIAL_DELAY 20 /*Frames to wait before beginning to scroll through letters
                                   when up/down is held.*/
#define LETTERS_SCROLL_DELAY 5 /*Frames to wait before scroling to the next letter when
                                 up/down was held.*/
#define FIRST_CHARACTER 32
#define LAST_CHARACTER 90

class CHighScore_Entry
{
public:
	//Properties
	int m_highScorePosition;
	int m_letterPosition;
	int m_keyDelayCount;
	char m_selectedLetter;
	char m_enteredName[13];
	bool m_nameEntered;

	//Main loop for when at the high score entry screen
	static void Main();

	//member functions
	//constructor
	CHighScore_Entry();
	void Init(); //Initialises the high score entry screen
	int Update(); //Updates the high score entry screen
};

//Non-class function prototypes

namespace HighScores
{

bool Load();
void Reset();
void Save();

}

#endif
