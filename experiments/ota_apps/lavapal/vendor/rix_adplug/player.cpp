/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * player.cpp - Minimal CPlayer base class implementation for ESP32 port.
 */
#include "player.h"

const unsigned short CPlayer::note_table[12] = {
	340, 363, 385, 408, 432, 458, 485, 514, 544, 577, 611, 647
};
const unsigned char CPlayer::op_table[9] = {
	0, 1, 2, 8, 9, 10, 16, 17, 18
};

CPlayer::CPlayer(Copl *newopl)
	: opl(newopl), db(NULL)
{
}

CPlayer::~CPlayer()
{
}

void CPlayer::seek(unsigned long ms)
{
	(void)ms;
}

unsigned long CPlayer::songlength(int subsong)
{
	(void)subsong;
	return 0;
}
