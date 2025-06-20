#ifndef GAMEDEFINES_H
#define GAMEDEFINES_H

#include <tonc.h>
#include <vector>

// GRIT_CPY replacement for CMake building
#define GFX_CPY(dst, name) memcpy16(dst, name##_gfx, name##_gfx_size/2)

//Game states
#define GS_WAIT 0
#define GS_TITLEBEGIN 1
#define GS_LEVELSELECT 2
#define GS_LEVELBEGIN 3
#define GS_SHOWHIGHSCORES 4
#define GS_ENTERHIGHSCORE 5

//Level status defines
#define ST_PLAYING_LEVEL 0
#define ST_LIFE_LOST 1
#define ST_ALL_LIVES_LOST 2
#define ST_LEVEL_COMPLETED 3

//Background numbers
#define TEXTBGNO 0
#define MAPBGNO 1
#define TITLEBGNO 2

//Sprite graphics tile numbers
#define SPRTILES_PROJECTILES 512 //After player tiles. 8 tiles, 4 frames.
                                 //8x8, but arrows use 2 tiles each.
#define SPRTILES_ENEMIES 520 //256 tiles, 16 frames, 32x32
#define SPRTILES_URN 776 //64 tiles, 4 frames, 32x32
#define SPRTILES_PLATFORM 840 //8 tiles, 1 frame, 32x16
#define SPRTILES_GAMEOVER 848 //24 tiles, 3 frames, 32x16.
                              //The 3 frames get combined into one image.
#define SPRTILES_COINSCORES 872 //16 tiles, 4 frames, 32x8
#define SPRTILES_DIGITS 888 //40 tiles, 10 frames, 16x16
#define SPRTILES_BLOCKS 928 //24 tiles, 3 frames, 32x16


//Urn sprite constant
#define URN_OAM 1

//Projectile constants
#define MAX_PROJECTILES 10
#define PROJECTILE_FIRST_OAM (URN_OAM + 1) //First sprite entry for projectiles,
                               //after the breaking urn sprite.Pplayer sprite
                               //uses index 0.

//Sprite object constants
#define MAX_SPRITES 72
//Sprite vector constants
#define MAX_VISIBLE_ENEMIES 10
#define ENEMY_FIRST_OAM (PROJECTILE_FIRST_OAM + MAX_PROJECTILES)
#define MAX_VISIBLE_PLATFORMS 10
#define PLATFORM_FIRST_OAM (ENEMY_FIRST_OAM + MAX_VISIBLE_ENEMIES)
#define GAME_OVER_OAM (PLATFORM_FIRST_OAM + MAX_VISIBLE_PLATFORMS)
#define COIN_SCORE_OAM (GAME_OVER_OAM + 3)
#define MAX_COIN_SCORE_SPRITES 5
#define HOURGLASS_DIGIT_OAM (COIN_SCORE_OAM + MAX_COIN_SCORE_SPRITES)
#define MAX_VISIBLE_BLOCKS 10
#define BLOCK_FIRST_OAM (HOURGLASS_DIGIT_OAM + 2)

//Gun defines
#define MAX_GUNS 64

//Level data defines
#define DATA_OFFSETS 6 //Total number of array data offsets in the level data per level
#define MAX_PROPERTIES 5 //Max number of properties an object can have
#define MAX_CHECKPOINT_SEQUENCES 10 //Max number of sequences that can be activated
                                    //when loading a checkpoint

//Layer 2 tile defines
#define TILE_SPEARS 9
#define TILE_ANKH 10
#define TILE_BOW 11
#define TILE_COIN 12
#define TILE_HOURGLASS 13
#define TILE_KEY 14
#define TILE_QUIVER 15
#define TILE_CHEST 16
#define TILE_DOOR_CLOSED_TOP 17
#define TILE_URN_TOP 18
#define TILE_SNAKE 19 //Sprite layer
#define TILE_SPHINX 20 //Sprite layer
#define TILE_GUN_L 21
#define TILE_GUN_R 22
#define TILE_SWITCH 23
#define TILE_PLATFORM1 24
#define TILE_PLATFORM2 25
#define TILE_MOVING_PLATFORM 26 //Sprite layer
#define TILE_LADDER 27
#define TILE_EXIT_TOP 28
#define TILE_WOOD_FRAME_TOP 29
#define TILE_WOOD_FRAME 30

//Level 5 tile defines
#define TILE_TELEPORTER 31
#define TILE_BALLGUN_L 32
#define TILE_BALLGUN_R 33
#define TILE_BLOCK 34 //Sprite layer

//Misc tiles
#define TILE_DOOR_OPEN_TOP 35
#define TILE_DOOR_CLOSED_BOTTOM 36
#define TILE_URN_BOTTOM 37
#define TILE_EXIT_BOTTOM 38
#define TILE_DOOR_OPEN_BOTTOM 39
#define TILE_URN_FLASH_TOP 40
#define TILE_URN_FLASH_BOTTOM 41

//TYPEDEFS

typedef struct THighScore
{
	unsigned int score;
	char name[13];
} THighScore;

typedef struct POINT_FIXED
{
	FIXED x,y;
} POINT_FIXED;

typedef struct VIEWPORT
{
   int x, xmin, xmax, xpage;
   int y, ymin, ymax, ypage;
} VIEWPORT;


typedef struct BGTYPE
{
	union
	{
		u32 state;			//!< Background state
		struct
		{
			u16 flags;
			u16 cnt;
		};
	};
    // Destination data
    SCR_ENTRY *dst_map;		//!< Screenblock pointer	
    // Source data
    SCR_ENTRY *src_map;		//!< Source map address
    u32 src_map_width;		//!< Source map width
    u32 src_map_height;		//!< Source map height
    FIXED map_x;			//!< X-coord on map (.8f)
    FIXED map_y;			//!< Y-coord on map (.8f)
    u32 layer;              //Map layer number
} BGTYPE;

typedef struct MAPPOS
{
	u8 tileIndex;
	u8 visibleTileIndex; //When a block is in the process ofchanging, this will be set
						 //to whatever it's changing to when it's flashing. Once
						 //the change is done, tileIndex will be set to this.
} MAPPOS;
    
typedef struct MAPCHANGETYPE
{
	u8 layer;
	u8 tileIndex;
	u8 solid;
	u8 x;
	u8 y;
} MAPCHANGETYPE;

typedef struct TContentsObject
{
	int type;						//!< Object type
	int properties[MAX_PROPERTIES];	//!< Array of property values
} TContentsObject;

typedef struct TMapObject
{
	int			x, y;						//!< Position
	int			layer;						//!< & layer
	int			properties[MAX_PROPERTIES];	//!< Array of property values
	TContentsObject	Contents;			//!< Contents object, if any.

} TMapObject;

typedef struct TCheckpoint
{
	POINT checkpointPos;
	POINT playerStartPos;
	int bows;
	int arrows;
	int keys;
	int seconds;
	int sequences[MAX_CHECKPOINT_SEQUENCES];
} TCheckpoint;


//TSprite struct will be used to store the locations of enemies and
//moving platforms. When a sprite goes out of visible range, it's
//position on the map will be stored in one of these structs.
typedef struct TSprite
{
	int		type;						//!< Sprite tile type
	int		x,y;						//!< Map tile position.
	int		properties[MAX_PROPERTIES]; //!< This sprite's properties.
	bool	visible;					//!< True when in visible range
	bool	available;					//!< True=active on the map
										//	 False if dead or in container(enemies)
										//   False if in a sequence that hasn't
										//   been activated yet.
	int		obj_id;						//!< Index of the object within its
										//   visible sprite vector.
} TSprite;

//TCoinScoreSprite struct is used to store data on the score
//sprites that appear when a coin is collected. Used in map.cpp.
typedef struct TCoinScoreSprite
{
	FIXED x,y;
	int type;
	int delay;
} TCoinScoreSprite;

typedef struct TErasedTile
{
	POINT pos; //Erased tile position
	int type; //Erased tile type
} TErasedTile;

typedef struct T_LEVELSTATE
{
	//Shared map data
	bool game_paused;
	unsigned int levelNum; //Array index for level data.
	unsigned int levelDataOffsets[6]; //Starting offsets in level data arrays for this level.
	int wallTiles; //Which wall tile set to use.
	VIEWPORT vp;
	BGTYPE bg[2];
	MAPPOS mapData[2][3840];
	u8 shadows[3840];
	MAPCHANGETYPE mapChanges[256]; //A list of map changes, upto 256 at a time
							//layer, tile index, bx, by
	int mapChangeCount; //How many map changes there are in total this frame
	POINT_FIXED mapOffset; //Position the map is centred on
	std::vector <TMapObject> mapObjects; //Map object vector. Stores properties
	                                     //and contents for map objects.
	TCheckpoint checkpoint;	//Player starting data after loss of life.
	int checkpointCount; //How many checkpoints have been reached. Score
	                     //1000 * this number each time.
	//Shared player data
	POINT playerStartPos; //Start location at beginning of level or from the last
	                      //checkpoint.
	POINT_FIXED playerPos; //Current player position
	POINT urnWasHit; //When an arrow hits an urn. This gets set to the tile position
	                 //of the urn that was hit. It's then processed in map.cpp
	POINT teleporterTouched; //Position of the teleporter that was last touched by the player.
	TCoinScoreSprite coinScoreSprites[MAX_COIN_SCORE_SPRITES]; //Score sprites when a coin is collected
	TErasedTile	erasedTiles[20]; //Keeps track of layer 1 tiles that got erased by blocks so we can modify
			                     //any sequence changes that were affected.
	int bows;
	int arrows;
	int keys;
	int seconds; //Total hourglass seconds
	int selectedSeconds; //Number of hourglass seconds that we've selected to use
	int currentSecond; //The currentky displayed hourglass second.
	int secondsCounter; //For displaying the hourglass seconds digits on scren
	                    //Counted in frames. Adds a few extra frames to keep "00" on screen
	                    //for a short time after the seconds are used.


	//Array of free oam indices for projectiles
	bool projectileOamIsFree[MAX_PROJECTILES];

	//Array of map sprite objects
	TSprite mapSprites[MAX_SPRITES];
	int totalSprites;

	//Array of free oam indices for enemies
	bool enemyOamIsFree[MAX_VISIBLE_ENEMIES];

	//Array of free oam indices for platforms
	bool platformOamIsFree[MAX_VISIBLE_PLATFORMS];

	//Array of free oam indices for moving blocks
	bool blockOamIsFree[MAX_VISIBLE_BLOCKS];

	int levelStatus; //The game status during a level


} T_LEVELSTATE;

#endif

