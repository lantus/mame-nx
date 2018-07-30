#include <switch.h>
#include <algorithm>
#include <new>
#include <iostream>
#include <vector>


#ifndef ROMLIST_H
#define ROMLIST_H


class CRomList
{
public:
	 
	CRomList();

	int FreeRomList();
	int AvRoms();
	int RefreshRomList();
	int InitRomList();

	int RomCount;
	bool RomListOK;
 
	uint32_t totalMAMEGames;
};


#endif