#ifndef ENEMIES_H
#define ENEMIES_H

//Included code files
#include <tonc.h>
#include <vector>
#include "gameDefines.h" //Global game defines

//Defines
#define SHOOT_DELAY 180 //3 seconds between shots for snake by default, but can vary.
#define SNAKE_BULLET_SPEED 0x100 //Speed of snake's bullet
#define ENEMY_ANI_SPEED_WALK 0x020
#define ENEMY_ANI_SPEED_DIE 0x020
#define ENEMY_FLASH_FRAMES 2 //Flash for 2 frames when hit

//Bounding box coordinates
#define ENEMY_BBOX_LEFT 2
#define ENEMY_BBOX_RIGHT 29
#define ENEMY_BBOX_TOP 9

//Class definition for the enemies class
class CEnemy
{
private:
	bool IsInRange(); //Check whether this enemy is within range of the player.
	                  //If not then it doesn't need to be processed.

	int type;
public:
    T_LEVELSTATE* m_ls;
	FIXED x, y;
	FIXED dx, dy;
	int xMin, xMax;
	int oam_index;
	bool active;
	int obj_index; //A reference to the object in the mapSprites array
	int hitPoints;
	int fireRate;
	int shootCounter; //For snakes, when this gets down to zero it can shoot.
	FIXED animFrame;
	int flashCounter;
	bool isDying;
	int dyingMotionCounter; //Counts down from a value to zero each frame when enemy
							//is dying. Enemy has upward motion when above 0 and downward
							//motion when it reaches 0.
	//constructor
	CEnemy(T_LEVELSTATE* ls, FIXED _x, FIXED _y, FIXED _dx, FIXED _dy, int _type, int _xMin, int _xMax, int _hitPoints, int _fireRate, int _obj_index);
	void Update();
};

//Non-class function prototypes
void update_enemies(T_LEVELSTATE *ls);
void scroll_enemies(int x, int y);
void reset_enemies(T_LEVELSTATE *ls);
std::vector <CEnemy>& getEnemies();

#endif
