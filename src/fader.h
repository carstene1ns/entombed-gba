#ifndef FADER_H
#define FADER_H

#include <tonc.h>

enum class FadeType
{
	IN,
	OUT
};

class CFader
{
public:
	explicit CFader(COLOR* palette);
	void set_palette(COLOR* palette);

	void apply(FadeType type, int frames);
	void clear();
	void restore();

private:
	CFader() = default;
	COLOR* m_palette;
};

#endif
