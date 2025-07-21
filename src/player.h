#ifndef PLAYER_H
#define PLAYER_H

//Included code files
#include <tonc.h>
#include <maxmod.h>
#include <vector>
#include "gameDefines.h" //Global game defines
#include "main.h"

// === CONSTANTS ======================================================

//NB the following speed defines are based on 24.8 fixed point maths, so the speed gets
//added each frame and when it reaches 0x100 that means 1 pixel or 1 frame change
//Player movement speed (up/down is double this)
#define PLAYER_SPEED 0x080

//Animation speeds
#define ANI_SPEED_WALK 0x020
#define ANI_SPEED_CLIMB 0x040
#define ANI_SPEED_DIE 0x020
#define ANI_SPEED_FIRE_BOW 0x030

//Bounding box coordinates
#define PLAYER_BBOX_LEFT 10
#define PLAYER_BBOX_RIGHT 21

//Arrow speed
#define ARROW_SPEED 0x1B0

//Defines for for "Look down" function.
#define LOOK_DOWN_SPEED 0x080
#define LOOK_DOWN_MAX 40 //40 pixels further down than usual

enum ELookDir
{
	LOOK_LEFT = 0, LOOK_RIGHT, CLIMB_LEFT, CLIMB_RIGHT
};

#define PLAYER_STATE_STAND	0x0100
#define PLAYER_STATE_WALK	0x0200
#define PLAYER_STATE_CLIMB	0x0300
#define PLAYER_STATE_JUMP	0x0400
#define PLAYER_STATE_FIRING	0x0500
#define PLAYER_STATE_DYING	0x0600

#define SOUND_FRAME_WALK	15
#define SOUND_FRAME_CLIMB	3

const OBJ_ATTR m_playerObj[1] = { {0, ATTR1_SIZE_32, 0, 0} };

//Class definition for the player class
class CPlayer
{
private:

	//Properties
	T_LEVELSTATE* m_ls; /*Stores the player position since this needs to be
	                      shared with other modules*/

	FIXED		m_vx, m_vy;		//!< Velocity
	u16			m_state;		//!< Sprite state
	u8			m_dir;		//!< Look direction
	u8			m_obj_id;		//!< Object index
	FIXED		m_ani_frame;	//!< Animation frame counter
	FIXED		m_soundFrame;   //Frame counter for playing movement samples

	int m_blockedDirs[4]; //Left, right, up, down
	bool m_blockedByDoor[2]; //If a door is stopping us from moving left or right
	int m_layer2Collisions[6][3]; /*Which layer 2 tiles 6 points of the player are
	                                touching. tile index, bx and by are stored.*/
	bool m_touchingLadder; //True if we're currently touching a ladder
	bool m_topOfLadder;    //True if we're at the top of a ladder
	int m_lookDownCounter;

	mm_sfxhand sndMoveHandle;

	//Member functions
	void player_input();
	void player_move();
	void player_animate();

	//Animation types
	void player_ani_stand();
	void player_ani_walk();
	void player_ani_climb();
	void player_ani_fireArrow();
	void player_ani_die();

	//Collision testing functions
	void player_test_collisions();
	void ProcessLayer2Collisions();

public:

	//Properties
	bool m_lookingDown; //Whether the 'Look down' button is currently held.
	int m_jumpCounter; /*Counts down from the desired height of a jump (also used in
	                     the death sequence)*/

	//Member functions
	void player_set_state(u32 state);

	//constructor
	CPlayer();

	void Init(T_LEVELSTATE *ls);
	void Update();
	void ResetPlayer();
};

#endif
