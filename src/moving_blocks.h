#ifndef MOVING_BLOCKS_H
#define MOVING_BLOCKS_H

#include <tonc.h>
#include <vector>
#include "gameDefines.h" //Global game defines

//Defines
enum class BlockType
{
	ROCK = 0,
	HOT,
	COLD
};

//Class definition for the moving blocks class
class CBlock
{
private:
	bool IsInRange(); /*Check whether this block is within range of the player.
	                    If not then it doesn't need to be processed.*/
public:
	T_LEVELSTATE* m_ls;

	FIXED x, y;
	FIXED dx, dy;
	int lastXDir; //The block's previous horizontal direction. 0=left, 1=right.
	BlockType type;
	int lifespan; //frames before it disappears after initial movement.
	int lifespanCounter;
	int oam_index;
	bool active;
	int obj_index; //A reference to the object in the mapSprites array

	//constructor
	CBlock(T_LEVELSTATE* ls, FIXED _x, FIXED _y, BlockType _type, int _startDir,
	       int _lifespan, int _movingDown, int _obj_index);
	void Update();

protected:
	CBlock() = default;
};

namespace Blocks
{

//Non-class function prototypes
void add_visible(T_LEVELSTATE *ls, int x, int y);
void update(T_LEVELSTATE *ls);
void scroll(int x, int y);
void reset(T_LEVELSTATE *ls);
std::vector<CBlock> &get();

}

#endif
