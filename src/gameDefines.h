#ifndef GAMEDEFINES_H
#define GAMEDEFINES_H

#include <tonc.h>
#include <vector>
#include <memory>

//Game states
enum class GameState
{
	WAIT = 0,
	TITLEBEGIN,
	LEVELSELECT,
	LEVELBEGIN,
	ENTERHIGHSCORE
};

//Level status defines
enum class LevelStatus
{
	PLAYING_LEVEL = 0,
	LIFE_LOST,
	ALL_LIVES_LOST,
	LEVEL_COMPLETED
};

//Background numbers
enum
{
	BGNO_TEXT = 0,
	BGNO_MAP_BG,
	BGNO_MAP_FG
};

//Sprite graphics tile numbers
#define SPRTILES_PROJECTILES 512 /*After player tiles. 8 tiles, 4 frames.
                                   8x8, but arrows use 2 tiles each.*/
#define SPRTILES_ENEMIES 520 //256 tiles, 16 frames, 32x32
#define SPRTILES_URN 776 //64 tiles, 4 frames, 32x32
#define SPRTILES_PLATFORM 840 //8 tiles, 1 frame, 32x16
#define SPRTILES_GAMEOVER 848 /*24 tiles, 3 frames, 32x16.
                                The 3 frames get combined into one image.*/
#define SPRTILES_COINSCORES 872 //16 tiles, 4 frames, 32x8
#define SPRTILES_DIGITS 888 //40 tiles, 10 frames, 16x16
#define SPRTILES_BLOCKS 928 //24 tiles, 3 frames, 32x16

enum
{
	HUD_TILE_EMPTY = 32,
	HUD_TILE_LIFE = HUD_TILE_EMPTY + 96,
	HUD_TILE_BOW,
	HUD_TILE_ARROW,
	HUD_TILE_KEY
};

enum
{
	TITLE_SPRITE_BOW = 12,
	TITLE_SPRITE_QUIVER,
	TITLE_SPRITE_KEY,
	TITLE_SPRITE_COIN,
	TITLE_SPRITE_HOURGLASS,
	TITLE_SPRITE_CHEST,
	TITLE_SPRITE_URN,
	TITLE_SPRITE_ANKH
};

//Urn sprite constant
#define URN_OAM 1

//Projectile constants
#define MAX_PROJECTILES 10
//First sprite entry for projectiles, after the breaking urn sprite.
//Player sprite uses index 0.
#define PROJECTILE_FIRST_OAM (URN_OAM + 1)

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
#define MAX_CHECKPOINT_SEQUENCES 10 /*Max number of sequences that can be activated
                                      when loading a checkpoint*/

//Layer 2 tile defines
enum Layer2Tiles
{
	TILE_SPEARS = 9,
	TILE_ANKH,
	TILE_BOW,
	TILE_COIN,
	TILE_HOURGLASS,
	TILE_KEY,
	TILE_QUIVER,
	TILE_CHEST,
	TILE_DOOR_CLOSED_TOP,
	TILE_URN_TOP,
	TILE_SNAKE, //Sprite layer
	TILE_SPHINX, //Sprite layer
	TILE_GUN_L,
	TILE_GUN_R,
	TILE_SWITCH,
	TILE_PLATFORM1,
	TILE_PLATFORM2,
	TILE_MOVING_PLATFORM, //Sprite layer
	TILE_LADDER,
	TILE_EXIT_TOP,
	TILE_WOOD_FRAME_TOP,
	TILE_WOOD_FRAME,
	//Level 5 tile defines
	TILE_TELEPORTER,
	TILE_BALLGUN_L,
	TILE_BALLGUN_R,
	TILE_BLOCK, //Sprite layer
	//Misc tiles
	TILE_DOOR_OPEN_TOP,
	TILE_DOOR_CLOSED_BOTTOM,
	TILE_URN_BOTTOM,
	TILE_EXIT_BOTTOM,
	TILE_DOOR_OPEN_BOTTOM,
	TILE_URN_FLASH_TOP,
	TILE_URN_FLASH_BOTTOM
};

//TYPEDEFS

typedef struct THighScore
{
	unsigned int score;
	char name[13];
} THighScore;

typedef struct POINT_FIXED
{
	FIXED x, y;
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
		u32 state;          //!< Background state
		struct
		{
			u16 flags;
			u16 cnt;
		};
	};
	// Destination data
	SCR_ENTRY* dst_map;     //!< Screenblock pointer
	// Source data
	SCR_ENTRY* src_map;     //!< Source map address
	u32 src_map_width;      //!< Source map width
	u32 src_map_height;     //!< Source map height
	FIXED map_x;            //!< X-coord on map (.8f)
	FIXED map_y;            //!< Y-coord on map (.8f)
	u32 layer;              //Map layer number
} BGTYPE;

typedef struct MAPPOS
{
	u8 tileIndex;
	u8 visibleTileIndex; /*When a block is in the process of changing, this will be set
	                       to whatever it's changing to when it's flashing. Once
	                       the change is done, tileIndex will be set to this.*/
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
	int type;                       //!< Object type
	int properties[MAX_PROPERTIES]; //!< Array of property values
} TContentsObject;

typedef struct TMapObject
{
	int			x, y;                       //!< Position
	int			layer;                      //!< & layer
	int			properties[MAX_PROPERTIES]; //!< Array of property values
	TContentsObject	Contents;               //!< Contents object, if any.

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
	int type;                       //!< Sprite tile type
	int x, y;                       //!< Map tile position.
	int properties[MAX_PROPERTIES]; //!< This sprite's properties.
	bool visible;                   //!< True when in visible range
	bool available;                 /*!< True=active on the map
	                                     False if dead or in container(enemies)
	                                     False if in a sequence that hasn't
	                                     been activated yet.*/
	int obj_id;                     /*!< Index of the object within its
	                                     visible sprite vector.*/
} TSprite;

//TCoinScoreSprite struct is used to store data on the score
//sprites that appear when a coin is collected. Used in map.cpp.
typedef struct TCoinScoreSprite
{
	FIXED x, y;
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
	MAPCHANGETYPE mapChanges[256]; /*A list of map changes, upto 256 at a time
	                                 layer, tile index, bx, by*/
	int mapChangeCount; //How many map changes there are in total this frame
	POINT_FIXED mapOffset; //Position the map is centred on
	std::vector <TMapObject> mapObjects; /*Map object vector. Stores properties
	                                       and contents for map objects.*/
	TCheckpoint checkpoint;	//Player starting data after loss of life.
	int checkpointCount; /*How many checkpoints have been reached. Score
	                       1000 * this number each time.*/
	//Shared player data
	POINT playerStartPos; /*Start location at beginning of level or from the last
	                        checkpoint.*/
	POINT_FIXED playerPos; //Current player position
	POINT urnWasHit; /*When an arrow hits an urn. This gets set to the tile position
	                   of the urn that was hit. It's then processed in map.cpp*/
	POINT teleporterTouched; //Position of the teleporter that was last touched by the player.
	TCoinScoreSprite coinScoreSprites[MAX_COIN_SCORE_SPRITES]; //Score sprites when a coin is collected
	TErasedTile
	erasedTiles[20]; /*Keeps track of layer 1 tiles that got erased by blocks so we can modify
	                   any sequence changes that were affected.*/
	int bows;
	int arrows;
	int keys;
	int seconds; //Total hourglass seconds
	int selectedSeconds; //Number of hourglass seconds that we've selected to use
	int currentSecond; //The currentky displayed hourglass second.
	int secondsCounter; /*For displaying the hourglass seconds digits on screen
	                      Counted in frames. Adds a few extra frames to keep "00" on screen
	                      for a short time after the seconds are used.*/

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

	LevelStatus levelStatus; //The game status during a level
} T_LEVELSTATE;

class CFader;

#endif
