#ifndef MOVING_BLOCKS_H
#define MOVING_BLOCKS_H

#include <tonc.h>
#include <vector>
#include "gameDefines.h" //Global game defines


//Defines
#define BLOCK_ROCK 0
#define BLOCK_HOT 1
#define BLOCK_COLD 2

//Class definition for the moving blocks class
class CBlock
{

private:
	bool IsInRange(); //Check whether this block is within range of the player.
	                  //If not then it doesn't need to be processed.
public:
    T_LEVELSTATE* m_ls;

	FIXED x, y;
	FIXED dx, dy;
	int lastXDir; //The block's previous horizontal direction. 0=left, 1=right.
	int type;
	int lifespan; //frames before it disappears after initial movement.
	int lifespanCounter;
	int oam_index;
	bool active;
	int obj_index; //A reference to the object in the mapSprites array

	//constructor
	CBlock(T_LEVELSTATE* ls, FIXED _x, FIXED _y, int _type, int _startDir, int _lifespan, int _movingDown, int _obj_index);
	//destructor
	~CBlock();
	void Update();
};

//Non-class function prototypes
void add_visible_block(T_LEVELSTATE *ls, int x, int y);
void update_blocks(T_LEVELSTATE *ls);
void scroll_blocks(int x, int y);
void reset_blocks(T_LEVELSTATE *ls);
std::vector <CBlock>& getBlocks();

#endif
