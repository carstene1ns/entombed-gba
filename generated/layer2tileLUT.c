
//Look-up table for matching any given layer2 tile index (a full 4 * 2 tile) to it's
//starting 8x8 tile in CBB[1]. Tile 432 is a blank tile.

const unsigned short layer2tileLUT[42] = {
0, //Tile index zero is blank so it uses the clear tile 0 after the exit tile gfx
8, 40, 72, 104, //Fire and water
136, 144, 152, 160, 168, //Spikes and spears
232, //Ankh
240, //Bow
200, //Coin
248, //Hourglass
256, //Key
264, //Quiver
272, //Chest
280, //Door closed (Top half)
312, //Urn(Top half) NB:The urn breaking animation will be a sprite.
0, 0, //These are enemy tiles so skip past them by setting them as blank
344, 352, //Gun L, Gun R
360, //Switch
368, //Platform(Static)-1
376, //Platform(Static) 2
0, //Moving platform, skip past the same as we did with the enemies
384, //Ladder
392, //Exit(Top half)

//Wooden frame tiles (29 and 30)
408, //Wooden frame top
416, //Wooden frame

//Level 5 Tiles
424, //Teleporter, 4 frames (32 tiles)
456, 464, //Ball gun L, Ball gun R
0, //Moving block (Sprite so use blank tile here)
296, //Door open (Top half)

//4 entries for the bottom halves of the door, urn, exit and open door. (These tiles wil be added
//to the map data in bg_init) The open door tile is used as a starting map tile A SINGLE TIME right
//at the end of level 5. I had to re-arrange my tiles at the very end just for that one exception to the rules!
288,
320,
400,
304,

//Urn being hit tiles
328,
336,

};
    
