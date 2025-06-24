#include "sequences.h"

//Included code files
#include <tonc.h>
#include <vector>

#include "gameDefines.h" //Global game defines
#include "globalvars.h"
#include "objPropertiesLUT.h"
#include "map.h"
#include "guns.h"
#include "sfx.h"

//Level data
#include "level_data.h"

std::vector <CSequence> sequences; //Sequences class instance vector

CSequence::CSequence(T_LEVELSTATE* ls, int _seqNum, int seqDataLen,
                     int _loop, int _alwaysOn, int _dataPos, bool _instant)
{
	m_active = true;
	//Pointer to global level state
	m_ls = ls;

	//Sequence properties
	m_seqNum = _seqNum;
	m_seqDataEnd = (_dataPos - 4) + seqDataLen;
	m_dataPos = _dataPos;
	m_currentStage = 0;
	m_loop = _loop;
	m_alwaysOn = _alwaysOn;
	m_instant = _instant;

	//Other properties
	m_flashOn = false;
	m_flashCounter = FLASH_FRAMES;

	//Initialise stage properties
	m_stageDataLen = 0;
	m_changeType = 0;
	m_delayCounter = 0;
	m_delayFlash = 0;

	//Read the first stage's properties
	m_stageDataLen = map_sequences[m_dataPos++];
	m_changeType = map_sequences[m_dataPos++];
	m_delayCounter = map_sequences[m_dataPos++]; //Delay frames property
	m_delayFlash = map_sequences[m_dataPos++];
	//Skip past toggle sequence property for now. (Level 5 property)
	m_dataPos++;

	//If instant is set then set delay counter to 0 and flash to false
	if (m_instant == true)
	{
		m_delayCounter = 0;
		m_delayFlash = 0;
	}
}

void CSequence::Update()
{
	int n;

	//If delayFlash is true, reduce the flashCounter.
	if (m_delayFlash)
	{
		m_flashCounter--;
	}

	//If flashCounter is 0 and delayCounter > 1, toggle flashOn, reset flashCounter
	//and read the changes with the finalise property set to false.
	//If m_delayCounter is 1 then it's going to be a solid change next
	//so we don't need to bother processing a flash.
	if ((m_flashCounter == 0) && (m_delayCounter > 1))
	{
		m_flashOn = !m_flashOn;
		m_flashCounter = FLASH_FRAMES;
		ReadChanges(false);
	}

	//If delayCounter is 0, read the changes with the finalise
	//property set to true, otherwise reduce the delayCounter value.
	if (m_delayCounter == 0)
	{
		//First reset the flash counter
		m_flashCounter = FLASH_FRAMES;

		ReadChanges(true);

		//If delayflash is true, play the change sound
		if (m_delayFlash)
		{
			mmEffect(SFX_CHANGE);
		}

		//If there is a stage after this then read the its properties
		if (m_dataPos < m_seqDataEnd)
		{
			m_stageDataLen = map_sequences[m_dataPos++];
			m_changeType = map_sequences[m_dataPos++];
			m_delayCounter = map_sequences[m_dataPos++]; //Delay frames property
			m_delayFlash = map_sequences[m_dataPos++];
			//Skip past toggle sequence property for now. (Level 5 property)
			m_dataPos++;

			//If instant is set then set delay counter to 0 and flash to false
			if (m_instant == true)
			{
				m_delayCounter = 0;
				m_delayFlash = 0;
			}
		}
		else
		{
			//If m_loop is 0, this sequence is now inactive
			if (m_loop == 0)
			{
				m_active = false;
			}
			else
			{
				//Else set the data start position to the start of this
				//level's first stage data and read its properties
				m_dataPos = map_dataOffsets[(m_ls->levelNum * 6) + 4];

				//If sequence is > 0 then find the start of the sequence we want
				if (m_seqNum > 0)
				{
					for (n = 0; n < m_seqNum; n++)
					{
						m_dataPos += map_sequences[m_dataPos];
					}
				}
				//Skip past the sequence properties since they've
				//already been read.
				m_dataPos += 4;

				m_stageDataLen = map_sequences[m_dataPos++];
				m_changeType = map_sequences[m_dataPos++];
				m_delayCounter = map_sequences[m_dataPos++]; //Delay frames property
				m_delayFlash = map_sequences[m_dataPos++];
				//Skip past toggle sequence property for now. (Level 5 property)
				m_dataPos++;
			}
		}
	}
	else
	{
		m_delayCounter--;
	}

}

void CSequence::ReadChanges(bool finalise)
{
	int n, m;
	int type;
	int x, y, layer;
	int objIndex;
	TMapObject tempObject;
	TGun gun;

	//Read any map changes for current stage
	//If finalise is false, we'll just be adding wall and fixture tile
	//changes with their solid property set to 0 or 1 depending on the
	//flashOn value. Otherwise we'll add all object properties and sprites
	//and the solid state will be 1 for map changes.
	//m_dataPos is assumed to be positioned at the start
	//of this stage's change data.

	n = ((m_dataPos - 5) + m_stageDataLen);
	//Read data until we get to the end of the stage data
	//(m_dataPos - 5) because that's the start of this stage's data.
	while (m_dataPos < n)
	{
		//Read the next object type, position and layer
		type = map_sequences[m_dataPos++];
		x = map_sequences[m_dataPos++];
		y = map_sequences[m_dataPos++];
		layer = map_sequences[m_dataPos++];

		//Now we figure out what to do with it
		//If layer >= 1 and type !=  then subtract 7 because
		//we start past the wall tiles on layer 1 & 3.
		if ((layer >= 1) && (type > 0))
		{
			type -= 7;
		}

		//If this is a tile that can be erased by a moving block, check the
		//erasedTiles array to see if it has been erased.
		//Applies to spikes/spears and ladders.
		if ((layer == 1) && ((type >= 5 && type <= 9) || (type == TILE_PLATFORM1) ||
		                     (type == TILE_PLATFORM2) || (type == TILE_LADDER)))
		{
			m = 0;
			while ((m_ls->erasedTiles[m].type > 0) && (m < 20))
			{
				if ((m_ls->erasedTiles[m].pos.x == x) && (m_ls->erasedTiles[m].pos.y == y) &&
				    (m_ls->erasedTiles[m].type == type))
				{
					type = 0;
				}
				else
				{
					m++;
				}
			}
		}

		//If finalise is false, we'll only care about wall and fixture tile changes
		if (finalise == false)
		{
			if (layer <= 1)
			{
				//If flashOn is false, read any wall or fixture layer tiles and add a
				//non-solid change at that position to that tile's current tile index.
				if (!m_flashOn)
				{
					m_ls->mapChanges[m_ls->mapChangeCount].layer = layer;
					m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = m_ls->mapData[layer][(y * 80) + x].tileIndex;
					m_ls->mapChanges[m_ls->mapChangeCount].solid = 0;
					m_ls->mapChanges[m_ls->mapChangeCount].x = x;
					m_ls->mapChanges[m_ls->mapChangeCount].y = y;
					m_ls->mapChangeCount++;

					//If the change was a double-height tile, set the tile below
					//as well. (Door, urn, exit)
					if ((type == TILE_DOOR_CLOSED_TOP) || (type == TILE_URN_TOP) || (type == TILE_EXIT_TOP))
					{
						m_ls->mapChanges[m_ls->mapChangeCount].layer = layer;
						m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = m_ls->mapData[layer][((
						        y + 1) * 80) + x].tileIndex;
						m_ls->mapChanges[m_ls->mapChangeCount].solid = 0;
						m_ls->mapChanges[m_ls->mapChangeCount].x = x;
						m_ls->mapChanges[m_ls->mapChangeCount].y = (y + 1);
						m_ls->mapChangeCount++;
					}
				}
				else
				{
					//m_flashOn is true so change the tile at that position to type (non-solid)
					m_ls->mapChanges[m_ls->mapChangeCount].layer = layer;
					m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = type;
					m_ls->mapChanges[m_ls->mapChangeCount].solid = 0;
					m_ls->mapChanges[m_ls->mapChangeCount].x = x;
					m_ls->mapChanges[m_ls->mapChangeCount].y = y;
					m_ls->mapChangeCount++;
					//If the change was a double-height tile, set the tile below
					//as well. (Door, urn, exit)
					m = 0; //Stores the bottom half tile, if any
					switch (type)
					{
						case TILE_DOOR_CLOSED_TOP: //Door(Closed) top
							m = TILE_DOOR_CLOSED_BOTTOM;
							break;
						case TILE_URN_TOP: //Urn top
							m = TILE_URN_BOTTOM;
							break;
						case TILE_EXIT_TOP: //Exit top
							m = TILE_EXIT_BOTTOM;
							break;
					}
					if (m > 0)
					{
						m_ls->mapChanges[m_ls->mapChangeCount].layer = layer;
						m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = m;
						m_ls->mapChanges[m_ls->mapChangeCount].solid = 0;
						m_ls->mapChanges[m_ls->mapChangeCount].x = x;
						m_ls->mapChanges[m_ls->mapChangeCount].y = (y + 1);
						m_ls->mapChangeCount++;
					}
				}
			}
			if (layer >= 1)
			{
				//Skip up and past any sprite objects and properties
				m_dataPos += objPropertiesLUT[type] & CONTENTS_MASK;
				//Skip past any contents data and properties, if any
				if ((objPropertiesLUT[type]) >> CONTENTS_BIT == 1)
				{
					//Lookup the property count of the contents type  which m_dataPos
					//is now at. Only look it up if the type is >7, meaning it's a
					//layer 1 object. Urns can hold wall tiles as well.
					if (map_sequences[m_dataPos] > 7)
					{
						//Skip past any properties.
						m_dataPos += (objPropertiesLUT[map_sequences[m_dataPos] - 7]);
					}
					//Skip past the contents type value as well.
					m_dataPos++;
				}
			}
		}
		else
		{
			//Else finalise so we'll be taking care of map objects/properties and
			//sprites as well as applying solid map tile changes.
			if (layer <= 1)
			{
				//Add the solid map tile change
				m_ls->mapChanges[m_ls->mapChangeCount].layer = layer;
				m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = type;
				m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
				m_ls->mapChanges[m_ls->mapChangeCount].x = x;
				m_ls->mapChanges[m_ls->mapChangeCount].y = y;
				m_ls->mapChangeCount++;

				//If this was a double-height tile, add the lower half
				m = 0; //Stores the bottom half tile, if any
				switch (type)
				{
					case TILE_DOOR_CLOSED_TOP: //Door(Closed) top
						m = TILE_DOOR_CLOSED_BOTTOM;
						break;
					case TILE_URN_TOP: //Urn top
						m = TILE_URN_BOTTOM;
						break;
					case TILE_EXIT_TOP: //Exit top
						m = TILE_EXIT_BOTTOM;
						break;
				}
				if (m > 0)
				{
					//Add the solid map tile change for the lower half
					m_ls->mapChanges[m_ls->mapChangeCount].layer = layer;
					m_ls->mapChanges[m_ls->mapChangeCount].tileIndex = m;
					m_ls->mapChanges[m_ls->mapChangeCount].solid = 1;
					m_ls->mapChanges[m_ls->mapChangeCount].x = x;
					m_ls->mapChanges[m_ls->mapChangeCount].y = (y + 1);
					m_ls->mapChangeCount++;
				}

				//Process layer 1 object properties and/or contents
				if (layer == 1)
				{
					//If this location had properties previously,
					//look up and remove the entry in m_ls->mapObjects
					objIndex = LookupMapObject(m_ls, x, y, 1);
					if (objIndex > -1)
					{
						m_ls->mapObjects.erase(m_ls->mapObjects.begin() + objIndex);

						//If this was a gun, remove it from the guns vector
						if ((type == TILE_GUN_L) || (type == TILE_GUN_R))
						{
							delete_gun(x, y);
						}
					}

					//If this tile type has properties/contents add them to
					//the mapObjects vector.
					if (((objPropertiesLUT[type] & CONTENTS_MASK) > 0) ||
					    ((objPropertiesLUT[type]) >> CONTENTS_BIT == 1))
					{
						//First clear the tempObject properties and contents data
						tempObject.Contents.type = 0;
						for (m = 0; m < MAX_PROPERTIES; m++)
						{
							tempObject.properties[m] = 0;
							tempObject.Contents.properties[m] = 0;
						}

						//Set the tempObjects data
						tempObject.x = x;
						tempObject.y = y;
						tempObject.layer = 1;
						//Read the object properties, if any.
						for (m = 0; m < (objPropertiesLUT[type] & CONTENTS_MASK); m++)
						{
							tempObject.properties[m] = map_sequences[m_dataPos++];
						}
						//Read the object contents, if any.
						if ((objPropertiesLUT[type]) >> CONTENTS_BIT == 1)
						{
							tempObject.Contents.type = map_sequences[m_dataPos++];
							//Subtract 7 if above 7, since it would be a layer 1 tile
							if (tempObject.Contents.type > 7)
							{
								tempObject.Contents.type -= 7;
							}
							//Read the contents object properties, if any
							//(Contents type at previous dataPos.)
							for (m = 0; m < (objPropertiesLUT[map_sequences[m_dataPos - 1]] & CONTENTS_MASK); m++)
							{
								tempObject.Contents.properties[m] = map_sequences[m_dataPos++];
							}
						}
						//Add tempObject to the mapObjects vector
						m_ls->mapObjects.push_back(tempObject);

						//If this was a gun, add it to the guns vector so
						//it can fire if necessary.
						if ((type == TILE_GUN_L) || (type == TILE_GUN_R)
						    || (type == TILE_BALLGUN_L) || (type == TILE_BALLGUN_R))
						{
							//Convert from tile coordinates to map coordinates
							gun.xPos = x * 32;
							gun.yPos = y * 16;
							gun.bullets = tempObject.properties[0];
							gun.interval = tempObject.properties[1];
							gun.timer = tempObject.properties[1];
							gun.dir = (type == TILE_GUN_L) ? 0 : 1;
							gun.type = ((type == TILE_GUN_L) || (type == TILE_GUN_R)) ? 0 : 1; //0 = blowgun, 1 = ball gun.
							gun.lifespan = (gun.type == 1) ? tempObject.properties[2] : 0;
							gun.weight = (gun.type == 1) ? tempObject.properties[3] : 1;
							add_gun(gun);
						}
					}
				}
			}
			else
			{
				//Else it was a sprite object

				//The sprite data is already stored in the
				//mapSprites array, so look it up and
				//make it available.
				//NB: mapSprites array stores locations in
				//map coordinates, so multiply x*32 and y*16
				objIndex = LookupMapObject(m_ls, x * 32, y * 16, 3);
				if (objIndex > -1)
				{
					m_ls->mapSprites[objIndex].available = true;
				}
				//Skip past any properties
				m_dataPos += objPropertiesLUT[type];
			}
		}
	}
	//If finalise was false, we'll be reading the same stage
	//next time, so reset the dataPos to the start of the stage
	if (finalise == false)
	{
		m_dataPos -= m_stageDataLen; //Go back to the start of the stage data
		m_dataPos += 5; //Go forward to the start of the change data.
	}

}
void initiate_sequence(T_LEVELSTATE *ls, int seqNum, bool instant)
{
	//The instant parameter is used when starting from a checkpoint
	//with sequences that need to be pre-initiated.

	int n;
	int seqDataLen;
	int dataPos;
	bool loop;
	int alwaysOn;

	//Set the data start position to the start of this
	//level's sequence data
	dataPos = map_dataOffsets[(ls->levelNum * 6) + 4];

	//If sequence is > 0 then find the start of the sequence we want
	if (seqNum > 0)
	{
		for (n = 0; n < seqNum; n++)
		{
			dataPos += map_sequences[dataPos];
		}
	}

	//Read the sequence header
	seqDataLen = map_sequences[dataPos++];
	dataPos++; //unused dependsOnSeq = map_sequences[dataPos++];
	loop = map_sequences[dataPos++];
	alwaysOn = map_sequences[dataPos++];

	//If the instant parameter is true then this method was called by
	//CMap::ProcessCheckpointSequences. In that case, do not add this sequence to
	//the vector if it's an always-on sequence or loops. These cases should be prevented by the
	//level editor but may not be.
	//(To add a sequence, either instant or (alwaysOn & loop) must be false)
	if ((instant == false) || ((alwaysOn == false) && (loop == false)))
	{
		sequences.push_back(CSequence(ls, seqNum, seqDataLen, loop, alwaysOn, dataPos, instant));
	}
}

void update_sequences(T_LEVELSTATE *ls)
{
	(void) ls;

	//Iterate through the vector of active sequences, call the update method for each one.
	for (std::vector<CSequence>::iterator it = sequences.begin(); it != sequences.end();)
	{
		//Check whether this sequence is active
		if (it->m_active == true)
		{
			it->Update();
			++it;
		}
		else
		{
			//Erase the sequence from the vector
			it = sequences.erase(it);
		}
	}
}

void initiate_checkpoint_sequences(T_LEVELSTATE *ls)
{
	//This procedure will do the following:
	//1. Look up the sequence data, if any, for the given checkpoint from the level data.
	//2. Loop through the sequences given in the level data for the checkpoint.
	//3.   Call a procedure in sequences.cpp to initiate the current sequence with the
	//     instant parameter set to true.
	//   end loop
	//4. While there are active non-always-on sequences
	//     Call the CMap::Update method
	//   end while
	//5. End.

	//There can be up to MAX_CHECKPOINT_SEQUENCES (10) checkpoint sequences.
	//m_ls->checkpoint.sequences[n];

	int n;
	bool lastSequenceFound;

	//First clear any residual sequences
	sequences.clear();

	n = 0;
	lastSequenceFound = false;
	while ((n < MAX_CHECKPOINT_SEQUENCES) && lastSequenceFound == false)
	{
		if (ls->checkpoint.sequences[n] >= 0)
		{
			initiate_sequence(ls, ls->checkpoint.sequences[n], true);
			n++;
		}
		else
		{
			lastSequenceFound = true;
		}
	}

}

void initiate_always_on_sequences(T_LEVELSTATE *ls)
{

	//Called at the start of a level. Initiates all
	//sequences that have their always-on property set
	//to true.
	u32 totalLevels;
	int dataPos;
	int dataEnd;
	int seqLength;
	int seqNum = 0;

	//First clear any residual sequences
	sequences.clear();

	totalLevels = (sizeof(map_dataOffsets) / 4) / 6;

	//Set the data start position to the start of this
	//level's sequence data
	dataPos = map_dataOffsets[(ls->levelNum * 6) + 4];
	if ((totalLevels - 1) > ls->levelNum)
	{
		//Next level's data offset - 1.
		dataEnd = map_dataOffsets[(((ls->levelNum + 1) * DATA_OFFSETS)) + 4] - 1;
	}
	else
	{
		//Last element of sequence array
		//Variable size is short
		dataEnd = sizeof(map_sequences) / 2;
	}

	//map_sequences type is short so /2 on sizeof
	if ((sizeof(map_sequences) / 2) > 0)
	{
		while (dataPos < dataEnd)
		{
			//Read the next sequence data length value
			seqLength = map_sequences[dataPos];
			if (map_sequences[dataPos + 3])
			{
				initiate_sequence(ls, seqNum, false);
			}
			//Go to the start of the next sequence data
			dataPos += seqLength;
			seqNum++;
		}
	}
}

void delay_sequences(int seconds)
{
	for (auto& sequence : sequences)
	{
		if (!sequence.m_alwaysOn)
		{
			sequence.m_delayCounter += (seconds * 60);
		}
	}
}

std::vector <CSequence> &getSequences()
{
	return sequences;
}
