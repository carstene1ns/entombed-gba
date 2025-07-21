#ifndef GUNS_H
#define GUNS_H

//Included code files
#include "gameDefines.h" //Global game defines

typedef struct TGun
{
	int type; //0 for blowguns, 1 for ball launchers.
	int xPos; //Map x position
	int yPos; //Map y position
	int bullets; //-1 = unlimited
	int interval;
	int lifespan; //For the ball launcher, frames until a launched ball disappears
	int weight; //For the ball launcher. Weight of the ball. 1 = normal, 2 = heavy.
	int timer; //Counts down from interval to zero then shoots
	int dir; //Gun direction
} TGun;

#define GUN_BULLET_SPEED 0x100

namespace Guns
{

void init(T_LEVELSTATE *ls);
void update(T_LEVELSTATE *ls);
void add(TGun gun);
void remove(int x, int y);

}

#endif
