#include "highscore_entry.h"

#include <string.h>
#include <tonc.h>

#include "gameDefines.h"
#include "globalvars.h"
#include "text.h"
#include "fader.h"
#include "posprintf.h"

//Declarations

//implementation of member functions

CHighScore::CHighScore() //Constructor
{
	//Set initial member variables
	m_keyDelayCount = 0;
	m_highScorePosition = 0;
	m_letterPosition = 0;
	m_selectedLetter = 33; //Position of capital A.
	m_nameEntered = false;
}

void CHighScore::Init()
{
	int ix, iy;
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

	//Set all text layer tiles to black (space characters)
	for (iy = 0; iy < 256; iy += 8)
	{
		for (ix = 0; ix < 256; ix += 8)
		{
			txt_putc(ix, iy, 32); //Space(Black)
		}
	}

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
	txt_puts(8, 136, "Left/Right: Change character.");
	txt_puts(8, 144, "A:Add letter, B:Del letter");
	txt_puts(8, 152, "Start: Accept name.");

	//Fade in
	g_fader->apply(FadeType::IN, 30);
}

int CHighScore::Update()
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

int CHighScore::SaveScores()
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

	return 0;
}

int HighScoreMain(CHighScore* HighScore)
{
	int done = 0;

	VBlankIntrWait();

	HighScore->Init();

	while (!done)
	{

		//Update the high score entry screen, if the update routine returns true then the loop
		//will end
		if (HighScore->Update())
		{
			done = 1;
		}
	}

	//Save the high scores
	HighScore->SaveScores();

	//Set the game state to the title screen
	g_GameState = GS_TITLEBEGIN;

	return 0;
}
