#ifndef PROJECTILES_H
#define PROJECTILES_H

//Included code files
#include <tonc.h>
#include <vector>
#include "gameDefines.h" //Global game defines

// === CONSTANTS ======================================================
#define BALL_ACCELERATION 0x0E

//Class definition for the projectiles class
class CProjectile 
{
public:
    T_LEVELSTATE* m_ls;

	FIXED x, y;
	FIXED dx, dy;
	int oam_index;
	bool active;
	int type;
	int lifespan; //Lifespan of the projectile, applies to the ball launcher.
	int weight; //Weight of the ball from a ball launcher. 1 = normal. 2 = heavy.

	//constructor
	CProjectile(T_LEVELSTATE* ls, FIXED _x, FIXED _y, FIXED _dx, FIXED _dy, int _type, int _lifespan, int _weight);
	void Update();
};

//Non-class function prototypes
void add_projectile(T_LEVELSTATE *ls, int x, int y, int dx, int dy, int type, int lifespan, int _weight);
void update_projectiles(T_LEVELSTATE *ls);
void scroll_projectiles(int x, int y);
void reset_projectiles(T_LEVELSTATE *ls);
std::vector <CProjectile>& getProjectiles();

#endif
