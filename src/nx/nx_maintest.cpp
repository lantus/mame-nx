
// Include the only file
#include "CustomUI.h"
#include "nx_RomList.h"

CRomList romList;	
 
int main()
{	
	romList.InitRomList();
	romList.RefreshRomList();
	
	UI::Init();
	
    while(appletMainLoop())
    {
        UI::Loop();
    }
    UI::Exit();
    return 0;
}