#include <iostream>
#include <string>
#include <vector>
#include <map> 

#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include "nx_romlist.h"

extern "C" {
#include "osd_cpu.h"
#include "driver.h"
#include "mame.h"
}

using namespace std;

extern const struct GameDriver *drivers[];

char szRoms[80]; 
char szRomPath[80];

int nLastRom = 0;
int nLastFilter = 0;
int HideChildren = 0;
int ThreeOrFourPlayerOnly = 0;

std::vector<std::string> m_vecAvailRomList;
std::vector<std::string> m_ListData;
std::map<std::string, int> mapRoms;


uint32_t totalMAMEGames = 0;

CRomList::CRomList()
{
  
}
int CRomList::InitRomList()
{

	RomCount = 0;
	RomListOK = false;
	 
	m_vecAvailRomList.clear();	 	

	return 0;
} 

int CRomList::FreeRomList()
{
	m_vecAvailRomList.clear();
	 
	return 0;
}

int CRomList::AvRoms()
{

	RomCount = m_vecAvailRomList.size();
	 
	return RomCount;
}

int CRomList::RefreshRomList()
{
	bool IsFiltered = false;
	std::vector<std::string> vecTempRomList;
	std::vector<std::string> vecAvailRomListFileName;
	std::vector<std::string> vecAvailRomList;
	std::vector<int>		 vecAvailRomIndex;

	InitRomList();
 
	m_vecAvailRomList.clear();

	if (m_ListData.empty())
	{
		// update this for multiple rom paths
		
		for (int d = 0; d < 1; d++) 
		{			
			DIR* dir;
			struct dirent* ent;

			dir = opendir("roms/");//Open current-working-directory.
			
			if(dir==NULL)
			{
				printf("Failed to open dir.\n");
			}
			else
			{
				printf("Dir-listing for '':\n");
				
				while ((ent = readdir(dir)))
				{
					//svcOutputDebugString( ent->d_name, 80);
												
					std::vector<std::string>::iterator iter = std::find(m_ListData.begin(), m_ListData.end(), ent->d_name);
					if (iter == m_ListData.end()) {
						m_ListData.push_back(ent->d_name);		
					}					
					
				}
				
				//std::transform(m_ListData.begin(), m_ListData.end(), m_ListData.begin(), ::tolower);	 						
				
				closedir(dir);
				printf("Done.\n");
			}			
			 
		}
		  
	}
 
	InitRomList();
	 
    for( totalMAMEGames = 0; drivers[totalMAMEGames]; ++totalMAMEGames)
	{
		char fullname[60];
		sprintf(fullname,"%s.zip",drivers[totalMAMEGames]->name);
		
		vecAvailRomListFileName.push_back(fullname);	 
		mapRoms[drivers[totalMAMEGames]->description] = totalMAMEGames;

	}
 
	for (unsigned int x = 0; x < vecAvailRomListFileName.size(); x++) {
		for (unsigned int y = 0; y < m_ListData.size() ; y++) {
			if (m_ListData[y] == vecAvailRomListFileName[x]) {
 				 
				m_vecAvailRomList.push_back(drivers[x]->description);
								
				//m_vecAvailRomReleasedBy.push_back(drivers[x]->manufacturer);						 
				//m_vecAvailRomManufacturer.push_back(drivers[x]->year);
							
			}

		}


	}

	std::sort(m_vecAvailRomList.begin(), m_vecAvailRomList.end());

	AvRoms();
	 	
	return 0;
}