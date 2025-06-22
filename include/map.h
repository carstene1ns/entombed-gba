#ifndef MAP_H
#define MAP_H

#include <tonc.h>
#include "gameDefines.h"

//Defines
#define URN_FLASH_FRAMES 2 //Urn flashes for 2 frames
#define URN_ANIM_SPEED 0x040 //Breaking urn animation speed
#define COIN_SCORE_DELAY 60
#define COIN_SCORE_SPEED 0x040


//Class definition for the map class
class CMap
{
private:
    T_LEVELSTATE* m_ls; //Pointer to the level data
    SCR_ENTRY m_tileList[1024]; //Stores the list of 8x8 tiles as calculated by the
                                //getGBATiles function
	char m_str[32];
    
    int m_tileAnimFrame; //The frame of animation that all animated tiles are in
                         //all animated tiles are 4 frames long
    int m_tileAnimCounter; //Counts up to a specified number of frames before updating
                           //m_tileAnimFrame

    //Urn properties
    POINT m_urnPos; //Tile position of the urn currently being processed, if any.
    int m_urnFlashCount;
    FIXED m_urnBreakFrame;
    int m_urnHitPoints;

    //Member functions
    void vp_center(VIEWPORT *vp, FIXED x, FIXED y);
    void vp_set_pos(VIEWPORT *vp, FIXED x, FIXED y);
    void bgt_init(BGTYPE *bgt, int bgnr, u32 ctrl, const unsigned char *map, u32 map_width, u32 map_height, u32 layer);
    void getGBATiles(int left, int top, int width, int height, int layer);
    void bgt_colcpy(BGTYPE *bgt, int tx, int ty);
    void bgt_rowcpy(BGTYPE *bgt, int tx, int ty);
    void bgt_update(BGTYPE *bgt, VIEWPORT *vp);
    void bgt_animate();
    void RedrawBlock(int x, int y, int layer);
    void CalculateShadows(int left, int top, int width, int height);
    void LoadLayer1Properties();
    void LoadContainers();
    void LoadLayer3Objects();
    void UrnHit();
    void UrnBroke();

public:
    //constructor
	CMap();
	
    //Member functions
    void Init(T_LEVELSTATE *ls);
    void Update();
   	void ResetMap();
   	void processMapChanges();
   	void Teleport();
};

//Non-class function prototypes
//Makes a coin score sprite appear at the given screen location
void ShowCoinScore(T_LEVELSTATE *ls, int x, int y, int type);
//Returns the vector index of the matching map object
int LookupMapObject(T_LEVELSTATE *ls, int x, int y, int layer);
//Returns the data position of a checkpoint if found, otherwise -1
int LookupCheckpoint(T_LEVELSTATE *ls, int x, int y);

#endif
