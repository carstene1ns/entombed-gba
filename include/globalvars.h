#include <tonc.h>
#include "gameDefines.h"

/**********************************************************************************
 * Globals
 ********************************************************************************/
extern int g_GameState; //The game initially begins at the title screen
extern int g_lives;
extern unsigned int g_score; /*These must be set before the game starts and during the game so
                               they have to be global*/
extern bool g_cheatEnabled[4];
extern int g_currentLevel; //The level number currently being played
extern bool g_completedLevels[5]; //Lets the game know which levels have been completed.
extern THighScore g_highScores[10]; //List of high scores
extern OBJ_ATTR g_obj_buffer[128]; /*Buffer to store OAM data for sprites,
                                     copied to OAM once per VBL*/
