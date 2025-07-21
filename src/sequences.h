#ifndef SEQUENCES_H
#define SEQUENCES_H

#include <tonc.h>
#include <vector>
#include "gameDefines.h" //Global game defines

// === CONSTANTS ======================================================
#define FLASH_FRAMES 3 //Frames between flashing on/off.

//Class definition for the projectiles class
class CSequence
{
private:
	T_LEVELSTATE* m_ls;
	int m_seqNum;
	int m_seqDataEnd;
	int m_currentStage;
	int m_stageDataLen;
	int m_dataPos; //Stores the current position in the sequence data
	int m_changeType; //Currently unused but still needs to be read
	int m_flashOn; //Whether the change is currently flashing on or off. Toggled with a !.
	int m_flashCounter;
	int m_loop;
	int m_delayFlash;
	//int m_toggleSequence; //Deal with this at a later time

	bool m_instant; /*If true, there will be no delays between stages.
	                  Used when starting from a checkpoint when
	                  sequences need to be pre-set.*/

public:
	bool m_active;
	int m_alwaysOn; /*The hourglass cannot delay always on sequences so these
	                  must be publically known.*/
	int m_delayCounter; //Can be changed when the player used the hourglass.

	//constructor
	CSequence(T_LEVELSTATE* ls, int _seqNum, int seqDataLen, int _loop,
	          int _alwaysOn, int _dataPos, bool _instant);
	void Update();
	void ReadChanges(bool finalise);

protected:
	CSequence() = default;
};

namespace Sequences
{

//Non-class function prototypes
void initiate(T_LEVELSTATE *ls, int seqNum, bool instant);
void update(T_LEVELSTATE *ls);
void initiate_checkpoint(T_LEVELSTATE *ls);
void initiate_always_on(T_LEVELSTATE *ls);
void reset();
void delay(int seconds);
std::vector<CSequence> &get();

}

#endif
