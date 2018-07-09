/*******************************************************************************
 Welltris (c)1991 Video System
********************************************************************************
 hardware is similar to aerofgt.c but with slightly different sprites, sound,
 and an additional 'pixel' layer used for the backdrops

 Driver by David Haywood, with help from Steph from The Ultimate Patchers
 Thanks to the authors of aerofgt.c and fromance.c on which most of this is
 based
********************************************************************************
OW-13 CPU

CPU  : MC68000P10
Sound: Z80 YM2610 YM3016
OSC  : 14.31818MHz (X1), 12.000MHz (X2),
       8.000MHz (X3), 20.0000MHz (OSC1)

ROMs:
j1.7 - Main programs (271000 compatible onetime)
j2.8 /

lh532j11.9  - Data
lh532j10.10 /

3.144 - Sound program (27c1000)

lh534j09.123 - Samples
lh534j10.124 |
lh534j11.126 /

lh534j12.77 - BG chr.

046.93 - OBJ chr.
048.94 /

PALs (16L8):
ow13-1.1
ow13-2.2
ow13-3.97
ow13-4.115

Custom chips:
V-SYSTEM C7-01 GGA
VS8905 6620 9039 ABBA
V-SYSTEM VS8904 GGB
V-SYSTEM VS8803 6082 9040 EBBB

********************************************************************************

 its impossible to know what some of the video registers do due to lack of
 evidence (bg palette has a selector, but i'm not sure which ... test mode
 colours use different palette on rgb test

********************************************************************************

 Info from Steph (a lot of thanks to him for looking at this)
 ---------------


The main thing is that, as in 'ridleofp', there is some code that could be used
if there was no "brute hack" in the code to never use it ...

The "brute hack" is there :

00B91C: 0000 0030                ori.b   #$30, D0

Replace with 0x4e714e71 and you'll be able to test the "hidden features" .
* See #define in driver, maybe there are other versions of the game?..

Dip Switch 1 is read from $f00d and is stored at $8802, while Switch 2 is read from
$f00f and is stored at $8803 ...

"DIPSW 2-5" (bit 4 of DSW2) is tested at address 0x007a18 :

00A718: 0838 0004 8803           btst    #$4, $8803.w

I haven't been able to figure out what this Dip Switch does as I haven't found a way
to call the routine that seems to start at 0x00a710 8( If you find ANY infos, please
let me know ...

"DIPSW 2-6" (bit 5 of DSW2) is probably one of the most "important" Dip Switch, as
it sets the maximum player from 2 (when OFF) to 4 (when ON) ... Watch the "attract
mode" to see how the game looks like ...


Differences between 2 players and 4 players mode :

1) 2 players mode (DIPSW 2-5 is OFF)

There are 2 coin slots (each one has its own coinage), there are only 2 "Start" buttons
and 2 sets of controls :

  1  "P1 Start"
  2  "P2 Start"
  3  no effect
  4  no effect
  5  "Coin 1"  (reads "DIPSW 1-1" to "DIPSW 1-4")
  6  "Coin 2"  (reads "DIPSW 1-5" to "DIPSW 1-8")
  7  no effect (in fact, calls the routine (see below) but doesn't add credits)
  8  no effect
  0  "Service" (adds 1 credit)

  P1 and P2 controls are OK, while P3 and P4 controls have no effect (and you can't
  even test them !) ...

In a 2 players game, the players can move their pieces on the following walls :

  P1  West and South walls
  P2  East and North walls

"DIPSW 2-3" determines how many credits are needed for a 2 players game :

  ON   2 (this should be the default value)
  OFF  1

Note that in a 2 players game, "PLAYER 1" is displayed twice ...

2) 4 players mode  (DIPSW 2-5 is ON)

There are 4 coin slots (each one using the SAME coinage), there are 4 "Start" buttons
and 4 sets of controls :

  1  used only in "test mode"
  2  used only in "test mode"
  3  used only in "test mode"
  4  used only in "test mode"
  5  "Coin 1"  (reads "DIPSW 1-1" to "DIPSW 1-4")
  6  "Coin 2"  (reads "DIPSW 1-1" to "DIPSW 1-4")
  7  "Coin 3"  (reads "DIPSW 1-1" to "DIPSW 1-4")
  8  "Coin 4"  (reads "DIPSW 1-1" to "DIPSW 1-4")
  0  "Service" (adds 1 credit)

  P1 to P4 controls are OK ...

You can test ALL these inputs when you are in the "test mode" : even if you don't see
the ones for players 3 and 4 when you reboot, they will "appear" once you press the key !
However "DISPW 2-4" and ""DISPW 2-5" will still display "N.C." ("Not Connected" ?)
followed by OFF or ON ...

While in the "test mode", the controls will be shown with an arrow :

  MAME key    player 3 display    player 4 display

    Up              Right               Left
    Down            Left                Right
    Left            Up                  Down
    Right           Down                Up

Note that I haven't been able to find where the "Tilt" key was mapped ...

In a 4 players game, each player can move his pieces on ONE following wall :

  P1  North wall
  P2  East  wall
  P3  South wall
  P4  West  wall

When 1 credit is inserted, a timer appears to wait for players to enter the game, and
it's IMPOSSIBLE to start a "normal" 1 player game (with the 4 walls) ...

To select a player, press one of his 2 buttons ...

"DIPSW 2-3" is VERY important here :

  - When it's OFF, only player 1 can play, the number of credits is decremented when
    you press the player buttons ... If you wait until timer reaches 0 without selecting
    a player, 1 credit will be substracted, and you'll start a game with player 1 ...
  - When it's ON, 1 credit will be automatically substracted, then the 4 players can
    play by pressing one of their buttons ... If you wait until timer reaches 0 without
    selecting a player, you'll start a game with player 1 ...

"Unfortunately", it's an endless game, has you have NO penalty when a piece can't be
placed 8( Also note that there is NO possibility to "join in" once the game has started ...


Some useful addresses :

  0xff803a ($803a) : credits

Even if display is limited to 9, there doesn't seem to be any limit to the real number
of credits ...

  0xff803b ($803b) : credits that will be added to $803a when you press '6'
  0xff803c ($803c) : credits that will be added to $803a when you press '7'
  0xff803d ($803d) : credits that will be added to $803a when you press '8'
  0xff803e ($803e) : credits that will be added to $803a when you press '0'

These credits will be added by routine described below ...

  0xff8959 ($8959) : "handicap" for player 1 (in a 1 or 2 players game)
  0xff89d9 ($89d9) : "handicap" for player 2 (in a 2 players game only)

"Handicap" range is 0x00 (piece fall from the top of the well) to 0x0c (when the game
is over) ... To end a game quickly for a player, set his value to 0x0b and wait until
a piece can't be placed ...


Some useful routines :

  $b080 : initialisation

When this routine is called with A4 = $400 (at $2466), it determines which routine will
be called when a coin is inserted ... It stores 0x0490 at 0xff8026 ($8026), then it
calls routine at $414 which tests "DIPSW 2-5" ... If it's ON, it stores 0x04a2 ...

Then it stores addresses of routines to add credits according to "DIPSW 1-1" to
"DIPSW 1-4" (stored at 0xff802a ($802a)), and to "DIPSW 1-5" to "DIPSW 1-8" (stored at
0xff802e ($802e)) ...

  $490 : reads coins inputs ("2 players mode")

This routines reads "Coin 1" status, and adds credits according to routine stored at
0xff802a ($802a) ... Then it reads "Coin 2" status, and adds credits according to
routine stored at 0xff802e ($802e) ...

  $4a2 : reads coins inputs ("4 players mode")

This routines reads status of the 4 "Coin", and adds credits according to routine
stored at 0xff802a ($802a) ...

  $50c : read status of a "Coin" button

This routines checks if value is <> 0x00 ... If this is the case, the routine that adds
credits is called, and the value is reset to 0x00 ...

  $ba36 : determines "Coin" status

This routines splits the inputs into consecutive addresses : if the button is pressed,
0x01 will be added :

  - status of "Coin 1"  is stored at address 0xff8030 ($8030)
  - status of "Coin 2"  is stored at address 0xff8031 ($8031)
  - status of "Coin 3"  is stored at address 0xff8032 ($8032)
  - status of "Coin 4"  is stored at address 0xff8033 ($8033)
  - status of "Service" is stored at address 0xff8034 ($8034)

  $9002 : checks "1 Player Start" and "2 Players Start" buttons

  $9018 : "1 Player Start" button is pressed

If enough credits, the number of credits is decremented by 1 ...

  $9056 : "2 Player Start" button is pressed

If enough credits, the number of credits is decremented by 1 or 2, depending of
"DIPSW 2-3" ...

  $90ce : reads status of "DIPSW 2-3" Dip Switch

This routine determines how many credits (1 or 2) are needed for a 2 players game ...

  $b908 : stores Dip Switches in memory

  $b91c : "brute hack" to disable "4 players mode" features

  $b924 : reads status of "DIPSW 2-7" Dip Switch for "screen flipping" support ...

  $9962 : reads status of "DIPSW 2-4" Dip Switch for "demo sounds" support ...

  $cf70 : sound routine

There are read/writes on bit 7 of 0xfff009 ($f009) ...

*/

#define WELLTRIS_4P_HACK 0

#include "driver.h"

data16_t *welltris_spriteram;
size_t welltris_spriteram_size;
data16_t *welltris_pixelram;
data16_t *welltris_charvideoram;

WRITE16_HANDLER( welltris_palette_bank_w );
WRITE16_HANDLER( welltris_gfxbank_w );
WRITE16_HANDLER( welltris_charvideoram_w );
VIDEO_START( welltris );
VIDEO_UPDATE( welltris );




static WRITE_HANDLER( welltris_sh_bankswitch_w )
{
	data8_t *rom = memory_region(REGION_CPU2) + 0x10000;

	cpu_setbank(1,rom + (data & 0x03) * 0x8000);
}


static int pending_command;

static WRITE16_HANDLER( sound_command_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(offset,data & 0xff);
		cpu_set_nmi_line(1, PULSE_LINE);
	}
}

static READ16_HANDLER( in0_r )
{
	return readinputport(0) | (pending_command ? 0x80 : 0);
}

static WRITE_HANDLER( pending_command_clear_w )
{
	pending_command = 0;
}


static MEMORY_READ16_START( welltris_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x17ffff, MRA16_ROM },
	{ 0x800000, 0x81ffff, MRA16_RAM }, /* Graph_1 & 2*/
	{ 0xff8000, 0xffcfff, MRA16_RAM }, /* Work */
	{ 0xffc000, 0xffc3ff, MRA16_RAM }, /* Sprite */
	{ 0xffd000, 0xffdfff, MRA16_RAM }, /* Char */
	{ 0xffe000, 0xffefff, MRA16_RAM }, /* Palette */

	{ 0xfff000, 0xfff001, input_port_1_word_r }, /* Bottom Controls */
	{ 0xfff002, 0xfff003, input_port_2_word_r }, /* Top Controls */
	{ 0xfff004, 0xfff005, input_port_3_word_r }, /* Left Side Ctrls */
	{ 0xfff006, 0xfff007, input_port_4_word_r }, /* Right Side Ctrls */

	{ 0xfff008, 0xfff009, in0_r }, /* Coinage, Start Buttons, pending sound command etc. */  /* Bit 5 Tested at start of irq 1 */
	{ 0xfff00a, 0xfff00b, input_port_5_word_r }, /* P3+P4 Coin + Start Buttons */
	{ 0xfff00c, 0xfff00d, input_port_6_word_r }, /* DSW0 Coinage */
	{ 0xfff00e, 0xfff00f, input_port_7_word_r }, /* DSW1 Game Options */
MEMORY_END

static MEMORY_WRITE16_START( welltris_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x17ffff, MWA16_ROM },
	{ 0x800000, 0x81ffff, MWA16_RAM, &welltris_pixelram },
	{ 0xff8000, 0xffcfff, MWA16_RAM },
	{ 0xffc000, 0xffc3ff, MWA16_RAM, &welltris_spriteram, &welltris_spriteram_size },
	{ 0xffd000, 0xffdfff, welltris_charvideoram_w, &welltris_charvideoram},
	{ 0xffe000, 0xffefff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },

	{ 0xfff000, 0xfff001, welltris_palette_bank_w },
	{ 0xfff002, 0xfff003, welltris_gfxbank_w },
//	{ 0xfff004, 0xfff005, ?? },
//	{ 0xfff006, 0xfff007, ?? },
	{ 0xfff008, 0xfff009, sound_command_w },
//	{ 0xfff00c, 0xfff00d, ?? },
//	{ 0xfff00e, 0xfff00f, ?? },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x77ff, MRA_ROM },
	{ 0x7800, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x77ff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x08, 0x08, YM2610_status_port_0_A_r },
	{ 0x0a, 0x0a, YM2610_status_port_0_B_r },
	{ 0x10, 0x10, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, welltris_sh_bankswitch_w },
	{ 0x08, 0x08, YM2610_control_port_0_A_w },
	{ 0x09, 0x09, YM2610_data_port_0_A_w },
	{ 0x0a, 0x0a, YM2610_control_port_0_B_w },
	{ 0x0b, 0x0b, YM2610_data_port_0_B_w },
	{ 0x18, 0x18, pending_command_clear_w },
PORT_END



INPUT_PORTS_START( welltris )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* Test (used to go through tests in service mode) */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )		/* Tested at start of irq 1 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* Service (adds a coin) */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* pending sound command */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#if WELLTRIS_4P_HACK
	/* These can actually be read in the test mode even if they're not used by the game without patching the code
	   might be useful if a real 4 player version ever turns up if it was ever produced */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
#else
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
#endif

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 )
#if WELLTRIS_4P_HACK
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
#else
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
#endif
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, "2-1, 4-2, 5-3, 6-4" )
	PORT_DIPSETTING(      0x0003, "2-1, 4-3" )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0001, "1-1, 2-2, 3-3, 4-5" )
	PORT_DIPSETTING(      0x0002, "1-1, 2-2, 3-3, 4-4, 5-6" )
	PORT_DIPSETTING(      0x0000, "1-1, 2-3" )
	PORT_DIPSETTING(      0x0005, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0040, "2-1, 4-2, 5-3, 6-4" )
	PORT_DIPSETTING(      0x0030, "2-1, 4-3" )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, "1-1, 2-2, 3-3, 4-5" )
	PORT_DIPSETTING(      0x0020, "1-1, 2-2, 3-3, 4-4, 5-6" )
	PORT_DIPSETTING(      0x0000, "1-1, 2-3" )
	PORT_DIPSETTING(      0x0050, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )			// "Super" in test mode
	PORT_DIPNAME( 0x0004, 0x0000, "Coin Mode" )
	PORT_DIPSETTING(      0x0004, "Mono Player" )
	PORT_DIPSETTING(      0x0000, "Many Player" )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
#if WELLTRIS_4P_HACK
	/* again might be handy if a real 4 player version shows up */
	PORT_DIPNAME( 0x0010, 0x0010, "DIPSW 2-5 (see notes)" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "4 Players Mode (see notes)" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
#else
	PORT_DIPNAME( 0x0010, 0x0010, "DIPSW 2-5 (unused)" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIPSW 2-6 (unused)" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
#endif
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) ) /* Flip Screen Not Currently Supported */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
  	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
INPUT_PORTS_END



static struct GfxLayout welltris_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout welltris_spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, RGN_FRAC(1,2)+1*4, RGN_FRAC(1,2)+0*4, RGN_FRAC(1,2)+3*4, RGN_FRAC(1,2)+2*4,
			5*4, 4*4, 7*4, 6*4, RGN_FRAC(1,2)+5*4, RGN_FRAC(1,2)+4*4, RGN_FRAC(1,2)+7*4, RGN_FRAC(1,2)+6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8
};

static struct GfxDecodeInfo welltris_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &welltris_charlayout,   0x000, 64 },
	{ REGION_GFX2, 0, &welltris_spritelayout, 0x700, 64 },
	{ -1 } /* end of array */
};



static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,
	8000000,	/* 8 MHz??? */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};


void init_welltris(void)
{
#if WELLTRIS_4P_HACK
	/* A Hack which shows 4 player mode in code which is disabled */
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);
	RAM[0xB91C/2] = 0x4e71;
	RAM[0xB91E/2] = 0x4e71;
#endif
}

static MACHINE_DRIVER_START( welltris )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)	/* 10 MHz */
	MDRV_CPU_MEMORY(welltris_readmem,welltris_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
								/* IRQs are triggered by the YM2610 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 351, 0, 239)
	MDRV_GFXDECODE(welltris_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(welltris)
	MDRV_VIDEO_UPDATE(welltris)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END



ROM_START( welltris )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "j2.8",			0x000000, 0x20000, CRC(68ec5691) SHA1(8615415c5c98aa9caa0878a8251da7985f050f94) )
	ROM_LOAD16_BYTE( "j1.7",			0x000001, 0x20000, CRC(1598ea2c) SHA1(e9150c3ab9b5c0eb9a5fee3e071358f92a005078) )
	/* Space */
	ROM_LOAD16_BYTE( "lh532j10.10",		0x100000, 0x40000, CRC(1187c665) SHA1(c6c55016e46805694348b386e521a3ef1a443621) )
	ROM_LOAD16_BYTE( "lh532j11.9",		0x100001, 0x40000, CRC(18eda9e5) SHA1(c01d1dc6bfde29797918490947c89440b58d5372) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "3.144",        0x00000, 0x20000, CRC(ae8f763e) SHA1(255419e02189c2e156c1fbcb0cd4aedd14ed8ffa) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x0a0000, REGION_GFX1, ROMREGION_DISPOSE ) /* CHAR Tiles */
	ROM_LOAD( "lh534j12.77",          0x000000, 0x80000, CRC(b61a8b74) SHA1(e17f7355375bdc166ef8131f7de9dbda5453f570) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* SPRITE Tiles */
	ROM_LOAD( "046.93",          0x000000, 0x40000, CRC(31d96d77) SHA1(5613ef9e9e38406b4e64fc8983ea50b57613923e) )
	ROM_LOAD( "048.94",          0x040000, 0x40000, CRC(bb4643da) SHA1(38d54f8c3dba09b528df05d748ab5bdf5d028453) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "lh534j11.126",          0x00000, 0x80000, CRC(bf85fb0d) SHA1(358f91bbff2d3260f83b5a0422c0d985d1735cef) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "lh534j09.123",          0x00000, 0x80000, CRC(6c2ce9a5) SHA1(a4011ecfb505191c9934ba374933cd11b331d55a) )
	ROM_LOAD( "lh534j10.124",          0x80000, 0x80000, CRC(e3682221) SHA1(3e1cda07cf451955dc473eabe007854e5148ae27) )
ROM_END



GAMEX( 1991, welltris, 0,        welltris, welltris, welltris, ROT0,   "Video System Co.", "Welltris (Japan, 2 players)", GAME_NO_COCKTAIL )
