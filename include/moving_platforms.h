#ifndef MOVING_PLATFORMS_H
#define MOVING_PLATFORMS_H

#include <tonc.h>
#include <vector>
#include "gameDefines.h" //Global game defines

//Class definition for the moving platforms class
class CPlatform
{
private:
	bool IsInRange(); //Check whether this platform is within range of the player.
	                  //If not then it doesn't need to be processed.
public:
    T_LEVELSTATE* m_ls;

	FIXED x, y;
	FIXED dx, dy;
	int xMin, xMax;
	int oam_index;
	bool active;
	int obj_index; //A reference to the object in the mapSprites array
	bool playerTouchingPlatform; //True if the player is touching this platform

	//constructor
	CPlatform(T_LEVELSTATE* ls, FIXED _x, FIXED _y, FIXED _dx, FIXED _dy, int _xMin, int _xMax, int _obj_index);
	void Update();
};

//Non-class function prototypes
void add_visible_platform(T_LEVELSTATE *ls, int x, int y);
void update_platforms(T_LEVELSTATE *ls);
void scroll_platforms(int x, int y);
void reset_platforms(T_LEVELSTATE *ls);
std::vector <CPlatform>& getPlatforms();

#endif
