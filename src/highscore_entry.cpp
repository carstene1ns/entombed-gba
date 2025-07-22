#include "highscore_entry.h"

#include <string.h>
#include <tonc.h>

#include "gameDefines.h"
#include "main.h"
#include "text.h"
#include "fader.h"
#include "posprintf.h"

//Declarations

static const char* instructions_text =
    R"(Left/Right: Change character.
A:Add letter, B:Del letter
Start: Accept name.)";

//implementation of member functions

CHighScore_Entry::CHighScore_Entry() //Constructor
{
	//Set initial member variables
	m_keyDelayCount = 0;
	m_highScorePosition = 0;
	m_letterPosition = 0;
	m_selectedLetter = 33; //Position of capital A.
	m_nameEntered = false;
}

void CHighScore_Entry::Init()
{
	int n, m;
	char tempStr[25];

	//Insert the high score in the correct place and push the scores
	//below down.
	n = 0;
	m_highScorePosition = -1;
	while ((n < 10) && (m_highScorePosition < 0))
	{
		if (g_score > g_highScores[n].score)
		{
			//score is higher than at this position, so we'll
			//insert it here.
			if (n < 9)
			{
				for (m = 9; m > n; m--)
				{
					g_highScores[m].score = g_highScores[m - 1].score;
					strcpy(g_highScores[m].name, g_highScores[m - 1].name);
				}
				g_highScores[n].score = g_score;
				m_highScorePosition = n;

			}
			else
			{
				//We got to position 10, so just replace the 10th score
				g_highScores[9].score = g_score;
				m_highScorePosition = 9;
			}
		}
		else
		{
			n++;
		}
	}

	m_letterPosition = 0;
	m_selectedLetter = 65; //'A'
	m_nameEntered = false;

	txt_clear_screen();

	//***Show the scores***
	txt_puts(72, 16, "TOMB TONKERS");
	for (n = 0; n < 10; n++)
	{
		if (n != m_highScorePosition)
		{
			//Print the current score
			posprintf(tempStr, "%06l......%s", g_highScores[n].score, g_highScores[n].name);
		}
		else
		{
			//Print the player's score with a blank name (First letter A)
			posprintf(tempStr, "%06l......A___________", g_highScores[n].score);
		}
		txt_puts(24, 48 + (n * 8), tempStr);
	}

	//Print the high score entry instructions at the bottom
	txt_margin_left(8);
	txt_puts(8, 136, instructions_text);
	txt_default_margins();

	//Fade in
	g_fader->apply(FadeType::IN, 30);
}

int CHighScore_Entry::Update()
{

	int n;

	//Check input
	key_poll();
	if (key_hit(KEY_A)) //Add selected character
	{
		if (m_letterPosition < 11)
		{

			m_enteredName[m_letterPosition] = m_selectedLetter;
			txt_putc(120 + (m_letterPosition * 8), 48 + (m_highScorePosition * 8), m_selectedLetter);

			//Move to next letter and display the selected letter at that
			//position.
			m_letterPosition++;

			//Set the current character position to the selected character
			m_enteredName[m_letterPosition] = m_enteredName[m_letterPosition - 1];
			m_selectedLetter = m_enteredName[m_letterPosition - 1];

			txt_putc(120 + (m_letterPosition * 8), 48 + (m_highScorePosition * 8), m_selectedLetter);
		}
	}
	if (key_hit(KEY_B)) //Backspace
	{
		if (m_letterPosition > 0)
		{
			//Set the current character to _ and go left a character
			m_enteredName[m_letterPosition] = '_';
			txt_putc(120 + (m_letterPosition * 8), 48 + (m_highScorePosition * 8),
			         m_enteredName[m_letterPosition]);
			m_letterPosition--;
			m_selectedLetter = m_enteredName[m_letterPosition];
		}
		else
		{
			m_selectedLetter = m_enteredName[0];
		}
	}
	if (key_hit(KEY_LEFT))
	{
		//Go back a character in the font
		m_selectedLetter--;
		if (m_selectedLetter < FIRST_CHARACTER)
		{
			m_selectedLetter = LAST_CHARACTER;
		}
		//Display the new character and set a delay value
		txt_putc(120 + (m_letterPosition * 8), 48 + (m_highScorePosition * 8), m_selectedLetter);
		m_keyDelayCount = LETTERS_INITIAL_DELAY;

	}
	if (key_hit(KEY_RIGHT))
	{
		//Go forward a character in the font
		m_selectedLetter++;
		if (m_selectedLetter > LAST_CHARACTER)
		{
			m_selectedLetter = FIRST_CHARACTER;
		}
		//Display the new character and set a delay value
		txt_putc(120 + (m_letterPosition * 8), 48 + (m_highScorePosition * 8), m_selectedLetter);
		m_keyDelayCount = LETTERS_INITIAL_DELAY;
	}
	if (key_is_down(KEY_LEFT))
	{
		//Go back a character in the font if delay is 0
		if (m_keyDelayCount <= 0)
		{
			m_selectedLetter--;
			if (m_selectedLetter < FIRST_CHARACTER)
			{
				m_selectedLetter = LAST_CHARACTER;
			}
			//Display the new character and set a delay value
			txt_putc(120 + (m_letterPosition * 8), 48 + (m_highScorePosition * 8), m_selectedLetter);
			m_keyDelayCount = LETTERS_SCROLL_DELAY;
		}
		else
		{
			m_keyDelayCount--;
		}
	}
	if (key_is_down(KEY_RIGHT))
	{
		//Go forward a character in the font if delay is 0
		if (m_keyDelayCount <= 0)
		{
			m_selectedLetter++;
			if (m_selectedLetter > LAST_CHARACTER)
			{
				m_selectedLetter = FIRST_CHARACTER;
			}
			//Display the new character and set a delay value
			txt_putc(120 + (m_letterPosition * 8), 48 + (m_highScorePosition * 8), m_selectedLetter);
			m_keyDelayCount = LETTERS_SCROLL_DELAY;
		}
		else
		{
			m_keyDelayCount--;
		}
	}
	if (key_hit(KEY_START))
	{
		//Set the current character position in the name to the
		//selected character.
		m_enteredName[m_letterPosition] = m_selectedLetter;

		//If there are characters after the letter position, convert
		//them to spaces
		if (m_letterPosition < 11)
		{
			for (n = m_letterPosition + 1; n < 12; n++)
			{
				m_enteredName[n] = ' ';
			}
		}

		//Set the global high score name to the name that was entered
		strcpy(g_highScores[m_highScorePosition].name, m_enteredName);

		//Fade out
		g_fader->apply(FadeType::OUT, 30);

		return 1;
	}
	VBlankIntrWait();

	return 0;
}

void CHighScore_Entry::Main()
{
	auto HighScore_Entry = std::make_unique<CHighScore_Entry>();
	int done = 0;

	VBlankIntrWait();

	HighScore_Entry->Init();

	while (!done)
	{

		//Update the high score entry screen, if the update routine returns true then the loop
		//will end
		if (HighScore_Entry->Update())
		{
			done = 1;
		}
	}

	//Save the high scores
	HighScores::Save();

	//Set the game state to the title screen
	g_GameState = GameState::TITLEBEGIN;
}

namespace HighScores
{

bool Load()
{
	int n, m;

	//Start at byte position 5 in sram because the first byte of sram can sometimes
	//become corrupted when powering on/off.

	//Check for magic number
	if ((*(u8*)(sram_mem + 4) != 'E') ||
	    (*(u8*)(sram_mem + 5) != 'N') ||
	    (*(u8*)(sram_mem + 6) != 'T') ||
	    (*(u8*)(sram_mem + 7) != 'D'))
	{
		//Magic number not found, so set the high scores to default values and return.
		Reset();

		return false;
	}

	//If we got here then load place the SRAM data in the relevant places
	//The high score data is saved in in SRAM in the following format: First there are
	//the 10 Name values at 12 bytes each. After that there are the 10 score values at
	//4 bytes each
	u8 *src = sram_mem + 8;

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
		g_highScores[n].score += (*src++ << 24);
		g_highScores[n].score += (*src++ << 16);
		g_highScores[n].score += (*src++ << 8);
		g_highScores[n].score += *src++;

		//If for some reason the score is out of range then put it back in range
		if (g_highScores[n].score > 999999)
		{
			g_highScores[n].score = 999999;
		}
	}

	return true;
}

void Reset()
{
	//Use default values
	strcpy(g_highScores[0].name, "KING TUT-TUT");
	g_highScores[0].score = 100000;
	strcpy(g_highScores[1].name, "RUBBERTITI");
	g_highScores[1].score = 90000;
	strcpy(g_highScores[2].name, "INDY JONES");
	g_highScores[2].score = 80000;
	strcpy(g_highScores[3].name, "DOC PHIBES");
	g_highScores[3].score = 70000;
	strcpy(g_highScores[4].name, "PTEPIC");
	g_highScores[4].score = 60000;
	strcpy(g_highScores[5].name, "RICK O'BRIEN");
	g_highScores[5].score = 50000;
	strcpy(g_highScores[6].name, "ALAN PARSONS");
	g_highScores[6].score = 40000;
	strcpy(g_highScores[7].name, "ALBERT SPEER");
	g_highScores[7].score = 30000;
	strcpy(g_highScores[8].name, "DUB VULTURE");
	g_highScores[8].score = 15000;
	strcpy(g_highScores[9].name, "ZOB");
	g_highScores[9].score = 5000;

	//Overwrite sram
	Save();
}

void Save()
{
	//Save the high score table to the sram
	int n, m;

	//Start at byte position 5 in sram because the first byte of sram can sometimes
	//become corrupted when powering on/off.
	u8 *dst = sram_mem + 4;

	//First write the magic number
	*dst++ = 'E';
	*dst++ = 'N';
	*dst++ = 'T';
	*dst++ = 'D';

	//Write the names
	for (n = 0; n < 10; n++)
	{
		for (m = 0; m < 12; m++)
		{
			*dst++ = g_highScores[n].name[m];
		}
	}

	//Write the scores
	//Type is unsigned integer so bit shifting is needed
	for (n = 0; n < 10; n++)
	{
		*dst++ = (u8)(g_highScores[n].score >> 24) & 0xff;
		*dst++ = (u8)(g_highScores[n].score >> 16) & 0xff;
		*dst++ = (u8)(g_highScores[n].score >> 8) & 0xff;
		*dst++ = (u8)g_highScores[n].score & 0xff;
	}
}

}
