#ifndef LEVEL_SELECTOR_H
#define LEVEL_SELECTOR_H

#include <tonc.h>
#include "gameDefines.h"

typedef struct TLevelText
{
	const char* lines[5];
} TLevelText;

class CLevelSelector
{
private:
	//Properties
	std::vector <TLevelText> m_levelText; //Level text vector. 5 lines of 27 characters each.
	//Original game has 35 characters per line (280 pixels) and has 40 pixels left.
	//I need to keep 24 pixels free for the player sprite. That means I have  216 pixels,
	//which is 27 characters per line.

	//Member functions

public:
	//Properties
	int m_highlightedLevel;
	int m_selectedLevel;
	u32 m_selectPos; //Level that the player sprite is next to
	u32 m_page; //Level page number. There are up to 3 levels per page.
	u32 m_endScreenPage;

	//Main loop for when at the level selector
	static void Main();

	//member functions
	//constructor
	CLevelSelector();
	void Init(); //Initialises the level selector
	void Update(); //Updates the level selector
	void InitLevelText();
	void DisplayLevels();
	void ShowEndScreen(int page); //After level 5 is complete, shows the ending text
};

#endif
