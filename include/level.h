#ifndef LEVEL_H
#define LEVEL_H

//Included code files
#include <tonc.h>
#include <vector>
#include "gameDefines.h" //Global game defines
#include "player.h"
#include "map.h"
#include "projectiles.h"

//Defines
#define GAME_OVER_FALL_SPEED 0x100

//Class definition for the level class
class CLevel
{
private:

    //Properties
	CPlayer* Player; //Player class instance
	CMap* Map; //Game map class instance
    
    //Member functions
	void SetupLevel(int mapNum);
    
public:

    //Properties
    u16 *m_paletteBuffer;
    bool m_fadedIn;
    bool m_teleported;
    bool m_levelEnded;
    int m_cheatSelected;
    std::vector <CProjectile> projectiles; //Projectiles class instance vector

    //constructor
	CLevel();
	//destructor
	~CLevel();
	
    //Member functions
    void Init();
    void Update();
   	void DeInit();
   	void Reset();
};

//Non-class function prototypes
int LevelMain(CLevel* Level, int mapNum, u16 *palBuffer); //Main loop for when in the game

#endif
