 
#include "nx_mame.h"

#include "osd_cpu.h"
#include "osdepend.h"
#include "driver.h"
#include "usrintrf.h"

int main(int argc, char **argv)
{
	
	int game_index;
	char *ext;
	int res = 0;
	
	// parse config and cmdline options
	game_index = 0;
	
	// have we decided on a game?
	if (game_index != -1)
	{
		// run the game
		res = run_game(game_index);

	}

}


int osd_init( void )
{

	return 0;
}


void osd_exit( void )
{

}