//Lookup table for object property counts and whether the object is a container.
//For fixture and sprite layer objects, where fire-top is set as tile 1
//Based on level editor tile indices (0 to 26)
//Update 7-15-15: Added 4 elements for level 5 objects, teleporter, ball guns, blocks.
//Teleportter has x,y destination coordinates. Ball gun has bullets and interval.
//Block has type, isMoving, startingDir.
const int objPropertiesLUT[35] = {
0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,16,0,17,5,5,2,2,1,0,0,5,0,0,0,0,3,4,4,3
};
