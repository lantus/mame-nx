/****************************************************************************

Operation Wolf  (c) Taito 1987
==============

David Graves, Jarek Burczynski
C-Chip emulation by Bryan McPhail

Sources:		MAME Rastan driver
			MAME Taito F2 driver
			Raine source - many thanks to Richard Bush
			  and the Raine Team.

Main CPU: MC68000 uses irq 5.
Sound   : Z80 & YM2151 & MSM5205


Operation Wolf uses similar hardware to Rainbow Islands and Rastan.
The screen layout and registers and sprites appear to be identical.


Gun Travel
----------

Horizontal gun travel maybe could be widened to include more
of the status bar (you can shoot enemies underneath it).

To keep the input span 0-255 a multiplier (300/256 ?)
would be used.


TODO
====

There are a few unmapped writes for the sound Z80 in the log.

Unknown writes to the MSM5205 control addresses

Raine source has standard Asuka/Mofflot sprite/tile priority:
0x2000 in sprite_ctrl puts all sprites under top bg layer. But
Raine simply kludges in this value, failing to read it from a
register. So what is controlling priority.


***************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

static data8_t *cchip_ram;
static UINT8 adpcm_b[0x08];
static UINT8 adpcm_c[0x08];
static int opwolf_gun_xoffs, opwolf_gun_yoffs;

void opwolf_cchip_init(void);

READ16_HANDLER( opwolf_cchip_status_r );
READ16_HANDLER( opwolf_cchip_data_r );
WRITE16_HANDLER( opwolf_cchip_status_w );
WRITE16_HANDLER( opwolf_cchip_data_w );
WRITE16_HANDLER( opwolf_cchip_bank_w );
WRITE16_HANDLER( rainbow_spritectrl_w );
WRITE16_HANDLER( rastan_spriteflip_w );

VIDEO_START( opwolf );
VIDEO_UPDATE( opwolf );

static READ16_HANDLER( cchip_r )
{
	return cchip_ram[offset];
}

static WRITE16_HANDLER( cchip_w )
{
	cchip_ram[offset] = data &0xff;
}

/* C-Chip simulation */
/*************************************************************************

  Operation Wolf C-Chip Protection
  ================================

  The C-Chip (Taito TC0030CMD) is an unidentified mask programmed
  microcontroller of some sort with 64 pins.  It probably has
  about 2k of ROM and 8k of RAM.

  Interesting memory locations shared by cchip/68k:

    14 - dip switch A (written by 68k at start)
    15 - dip switch B (written by 68k at start)
    1b - Current level number (1-6)
    1c - Number of men remaining in level
    1e - Number of helicopters remaining in level
    1f - Number of tanks remaining in level
    20 - Number of boats remaining in level
    21 - Hostages in plane (last level)
    22 - Hostages remaining (last level)/Hostages saved (2nd last level)#
    27 - Set to 1 when final boss is destroyed
    32 - Set to 1 by cchip when level complete (no more enemies remaining)
    34 - Game state (0=attract mode, 1=intro, 2=in-game, 3=end-screen)
    51/52 - Used by cchip to signal change in credit level to 68k
    53 - Credit count
    75 - Set to 1 to trigger end of game boss
    7a - Used to trigger level data select command

  Notes on bootleg c-chip:
    Bootleg cchip forces english language mode
    Bootleg forces round 4 in attract mode
    Bootleg doesn't support service switch
    If you die after round 6 then the bootleg fails to reset the difficulty
    for the next game.

*************************************************************************/

static UINT8 current_bank=0;
static UINT8 current_cmd=0;
static UINT8* cchip_ram=0;
static UINT8 cchip_last_7a=0;
static UINT8 cchip_last_04=0;
static UINT8 cchip_last_05=0;
static UINT8 cchip_coins_for_credit_a=1;
static UINT8 cchip_credit_for_coin_b=2;
static UINT8 cchip_coins=0;
static UINT8 c588=0, c589=0, c58a=0; /* These variables derived from the bootleg */

static const UINT16 level_data_00[] = {
	0x0480, 0x1008, 0x0300,   0x5701, 0x0001, 0x0010,
	0x0480, 0x1008, 0x0300,   0x5701, 0x0001, 0x002b,
	0x0780, 0x0009, 0x0300,   0x4a01, 0x0004, 0x0020,
	0x0780, 0x1208, 0x0300,   0x5d01, 0x0004, 0x0030,
	0x0780, 0x0209, 0x0300,   0x4c01, 0x0004, 0x0038,
	0x0780, 0x0309, 0x0300,   0x4d01, 0x0004, 0x0048,
	0x0980, 0x1108, 0x0300,   0x5a01, 0xc005, 0x0018,
	0x0980, 0x0109, 0x0300,   0x4b01, 0xc005, 0x0028,
	0x0b80, 0x020a, 0x0000,   0x6401, 0x8006, 0x0004,
	0x0c80, 0x010b, 0x0000,   0xf201, 0x8006, 0x8002,
	0x0b80, 0x020a, 0x0000,   0x6401, 0x8006, 0x0017,
	0x0c80, 0x010b, 0x0000,   0xf201, 0x8006, 0x8015,
	0x0b80, 0x020a, 0x0000,   0x6401, 0x0007, 0x0034,
	0x0c80, 0x010b, 0x0000,   0xf201, 0x0007, 0x8032,
	0x0b80, 0x020a, 0x0000,   0x6401, 0x8006, 0x803e,
	0x0c80, 0x010b, 0x0000,   0xf201, 0x8006, 0x803d,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x0008,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x000b,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x001b,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x001e,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x8007, 0x0038,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x8007, 0x003b,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x8042,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x8045,
	0x0c80, 0x000b, 0x0000,   0xf101, 0x800b, 0x8007,
	0x0c80, 0x000b, 0x0000,   0xf101, 0x800b, 0x801a,
	0x0c80, 0x000b, 0x0000,   0xf101, 0x000c, 0x8037,
	0x0c80, 0x000b, 0x0000,   0xf101, 0x800b, 0x0042,
	0x0c80, 0xd04b, 0x0000,   0xf301, 0x8006, 0x8009,
	0x0c80, 0xd04b, 0x0000,   0xf301, 0x8006, 0x801c,
	0x0c80, 0xd04b, 0x0000,   0xf301, 0x8006, 0x0044,
	0x0c80, 0x030b, 0x0000,   0xf401, 0x0008, 0x0028,
	0x0c80, 0x030b, 0x0000,   0xf401, 0x0008, 0x804b,
	0x0c00, 0x040b, 0x0000,   0xf501, 0x0008, 0x8026,
	0xffff
};

static const UINT16 level_data_01[] = {
	0x0780, 0x0209, 0x0300,   0x4c01, 0x0004, 0x0010,
	0x0780, 0x0209, 0x0300,   0x4c01, 0x4004, 0x0020,
	0x0780, 0x0309, 0x0300,   0x4d01, 0xe003, 0x0030,
	0x0780, 0x0309, 0x0300,   0x4d01, 0x8003, 0x0040,
	0x0780, 0x0209, 0x0300,   0x4c01, 0x8004, 0x0018,
	0x0780, 0x0309, 0x0300,   0x4d01, 0xc003, 0x0028,
	0x0b80, 0x000b, 0x0000,   0x0b02, 0x8009, 0x0029,
	0x0b80, 0x0409, 0x0000,   0x0f02, 0x8008, 0x8028,
	0x0b80, 0x040a, 0x0000,   0x3502, 0x000a, 0x8028,
	0x0b80, 0x050a, 0x0000,   0x1002, 0x8006, 0x8028,
	0x0b80, 0x120a, 0x0000,   0x3602, 0x0008, 0x004d,
	0x0b80, 0x120a, 0x0000,   0x3602, 0x0008, 0x004f,
	0x0b80, 0x120a, 0x0000,   0x3602, 0x0008, 0x0001,
	0x0b80, 0x120a, 0x0000,   0x3602, 0x0008, 0x0003,
	0x0b80, 0x130a, 0x0000,   0x3a02, 0x0007, 0x0023,
	0x0b80, 0x130a, 0x0000,   0x3a02, 0x0007, 0x8025,
	0x0b80, 0x130a, 0x0000,   0x3a02, 0x8009, 0x0023,
	0x0b80, 0x130a, 0x0000,   0x3a02, 0x8009, 0x8025,
	0x0b80, 0x140a, 0x0000,   0x3e02, 0x0007, 0x000d,
	0x0b80, 0x140a, 0x0000,   0x3e02, 0x0007, 0x800f,
	0x0b80, 0x000b, 0x0000,   0x0102, 0x0007, 0x804e,
	0x0b80, 0xd24b, 0x0000,   0x0302, 0x0007, 0x000e,
	0x0b80, 0x000b, 0x0000,   0x0402, 0x8006, 0x0020,
	0x0b80, 0xd34b, 0x0000,   0x0502, 0x8006, 0x0024,
	0x0b80, 0x000b, 0x0000,   0x0602, 0x8009, 0x0001,
	0x0b80, 0xd44b, 0x0000,   0x0702, 0x800b, 0x800b,
	0x0b80, 0xd54b, 0x0000,   0x0802, 0x800b, 0x000e,
	0x0b80, 0x000b, 0x0000,   0x0902, 0x800b, 0x0010,
	0x0b80, 0x000b, 0x0000,   0x0a02, 0x0009, 0x0024,
	0x0b80, 0xd64b, 0x0000,   0x0c02, 0x000c, 0x8021,
	0x0b80, 0x000b, 0x0000,   0x0d02, 0x000c, 0x0025,
	0x0b80, 0x000b, 0x0000,   0x0e02, 0x8009, 0x004e,
	0x0b80, 0x000b, 0x0300,   0x4e01, 0x8006, 0x8012,
	0x0b80, 0x000b, 0x0300,   0x4e01, 0x0007, 0x8007,
	0xffff
};

static const UINT16 level_data_02[] = {
	0x0480, 0x000b, 0x0300,   0x4501, 0x0001, 0x0018,
	0x0480, 0x000b, 0x0300,   0x4501, 0x2001, 0x0030,
	0x0780, 0x1208, 0x0300,   0x5d01, 0x0004, 0x0010,
	0x0780, 0x1208, 0x0300,   0x5d01, 0x2004, 0x001c,
	0x0780, 0x1208, 0x0300,   0x5d01, 0xe003, 0x0026,
	0x0780, 0x1208, 0x0300,   0x5d01, 0x8003, 0x0034,
	0x0780, 0x1208, 0x0300,   0x5d01, 0x3004, 0x0040,
	0x0780, 0x010c, 0x0300,   0x4601, 0x4004, 0x0022,
	0x0780, 0x010c, 0x0300,   0x4601, 0x6004, 0x0042,
	0x0780, 0x000c, 0x0500,   0x7b01, 0x800b, 0x0008,
	0x0780, 0x010c, 0x0300,   0x4601, 0x2004, 0x0008,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0b80, 0x000b, 0x0000,   0x1902, 0x000b, 0x0004,
	0x0b80, 0x000b, 0x0000,   0x1a02, 0x0009, 0x8003,
	0x0b80, 0x000b, 0x0000,   0x1902, 0x000b, 0x000c,
	0x0b80, 0x000b, 0x0000,   0x1a02, 0x0009, 0x800b,
	0x0b80, 0x000b, 0x0000,   0x1902, 0x000b, 0x001c,
	0x0b80, 0x000b, 0x0000,   0x1a02, 0x0009, 0x801b,
	0x0b80, 0x000b, 0x0000,   0x1902, 0x000b, 0x002c,
	0x0b80, 0x000b, 0x0000,   0x1a02, 0x0009, 0x802b,
	0x0b80, 0x000b, 0x0000,   0x1902, 0x000b, 0x0044,
	0x0b80, 0x000b, 0x0000,   0x1a02, 0x0009, 0x8043,
	0x0b80, 0x000b, 0x0000,   0x1902, 0x000b, 0x004c,
	0x0b80, 0x000b, 0x0000,   0x1a02, 0x0009, 0x804b,
	0x0b80, 0x020c, 0x0300,   0x4801, 0xa009, 0x0010,
	0x0b80, 0x020c, 0x0300,   0x4801, 0xa009, 0x0028,
	0x0b80, 0x020c, 0x0300,   0x4801, 0xa009, 0x0036,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0xffff
};

static const UINT16 level_data_03[] = {
	0x0480, 0x000b, 0x0300,   0x4501, 0x0001, 0x0018,
	0x0480, 0x000b, 0x0300,   0x4501, 0x2001, 0x002b,
	0x0780, 0x010c, 0x0300,   0x4601, 0x0004, 0x000d,
	0x0780, 0x000c, 0x0500,   0x7b01, 0x800b, 0x0020,
	0x0780, 0x010c, 0x0300,   0x4601, 0x2004, 0x0020,
	0x0780, 0x010c, 0x0300,   0x4601, 0x8003, 0x0033,
	0x0780, 0x010c, 0x0300,   0x4601, 0x0004, 0x003c,
	0x0780, 0x010c, 0x0300,   0x4601, 0xd003, 0x0045,
	0x0780, 0x000c, 0x0500,   0x7b01, 0x900b, 0x0041,
	0x0780, 0x010c, 0x0300,   0x4601, 0x3004, 0x0041,
	0x0b80, 0x020c, 0x0300,   0x4801, 0x0007, 0x0000,
	0x0b80, 0x410a, 0x0000,   0x2b02, 0xe006, 0x4049,
	0x0b80, 0x020c, 0x0300,   0x4801, 0x8007, 0x000b,
	0x0b80, 0x000b, 0x0000,   0x2702, 0x800a, 0x8005,
	0x0b80, 0x000b, 0x0000,   0x1e02, 0x0008, 0x800e,
	0x0b80, 0x000b, 0x0000,   0x1f02, 0x8007, 0x0011,
	0x0b80, 0x000b, 0x0000,   0x2802, 0x000b, 0x0012,
	0x0b80, 0x000b, 0x0000,   0x2002, 0x0007, 0x8015,
	0x0b80, 0x000b, 0x0000,   0x2102, 0x0007, 0x801b,
	0x0b80, 0x000b, 0x0000,   0x2902, 0x800a, 0x001a,
	0x0b80, 0x000b, 0x0000,   0x2202, 0x8007, 0x001e,
	0x0b80, 0x000b, 0x0000,   0x1e02, 0x0008, 0x0025,
	0x0b80, 0x000b, 0x0000,   0x2302, 0x8007, 0x802c,
	0x0b80, 0x000b, 0x0000,   0x2802, 0x000b, 0x8028,
	0x0b80, 0x020c, 0x0300,   0x4801, 0x0007, 0x0030,
	0x0b80, 0x400a, 0x0000,   0x2e02, 0x4007, 0x002d,
	0x0b80, 0x000b, 0x0000,   0x2702, 0x800a, 0x8035,
	0x0b80, 0x020c, 0x0300,   0x4801, 0x8007, 0x0022,
	0x0b80, 0x000b, 0x0000,   0x2402, 0x8007, 0x0047,
	0x0b80, 0x000b, 0x0000,   0x2a02, 0x800a, 0x004b,
	0x0b80, 0x000b, 0x0000,   0x2502, 0x0007, 0x804b,
	0x0b80, 0x000b, 0x0000,   0x2602, 0x0007, 0x004e,
	0x0b80, 0x020c, 0x0300,   0x4801, 0x0007, 0x8043,
	0x0b80, 0x020c, 0x0300,   0x4801, 0x8007, 0x803d,
	0xffff
};

static const UINT16 level_data_04[] = {
	0x0780, 0x0209, 0x0300,   0x4c01, 0x0004, 0x0010,
	0x0780, 0x0209, 0x0300,   0x4c01, 0x4004, 0x0020,
	0x0780, 0x0309, 0x0300,   0x4d01, 0xe003, 0x0030,
	0x0780, 0x0309, 0x0300,   0x4d01, 0x8003, 0x0040,
	0x0780, 0x0209, 0x0300,   0x4c01, 0x8004, 0x0018,
	0x0780, 0x0309, 0x0300,   0x4d01, 0xc003, 0x0028,
	0x0780, 0x000b, 0x0300,   0x5601, 0x8004, 0x0008,
	0x0780, 0x000b, 0x0300,   0x5601, 0x8004, 0x0038,
	0x0780, 0x000b, 0x0300,   0x5501, 0x8004, 0x0048,
	0x0980, 0x0509, 0x0f00,   0x0f01, 0x4005, 0x4007,
	0x0980, 0x0509, 0x0f00,   0x0f01, 0x4005, 0x4037,
	0x0b80, 0x030a, 0x0000,   0x1302, 0x8006, 0x0040,
	0x0b80, 0x110a, 0x0000,   0x1502, 0x8008, 0x8048,
	0x0b80, 0x110a, 0x0000,   0x1502, 0x8008, 0x8049,
	0x0b80, 0x000b, 0x0000,   0xf601, 0x0007, 0x8003,
	0x0b80, 0x000b, 0x0000,   0xf701, 0x0007, 0x0005,
	0x0b80, 0x000b, 0x0000,   0xf901, 0x0007, 0x8008,
	0x0b80, 0x000b, 0x0000,   0xf901, 0x0007, 0x0010,
	0x0b80, 0x000b, 0x0000,   0xfa01, 0x0007, 0x8013,
	0x0b80, 0x000b, 0x0000,   0xf801, 0x800b, 0x800b,
	0x0b80, 0x000b, 0x0000,   0x0002, 0x800b, 0x801a,
	0x0b80, 0x000b, 0x0000,   0xf901, 0x0007, 0x8017,
	0x0b80, 0x000b, 0x0000,   0xfa01, 0x0007, 0x001b,
	0x0b80, 0x000b, 0x0000,   0xf801, 0x800b, 0x0013,
	0x0b80, 0x000b, 0x0000,   0x4202, 0x800b, 0x0016,
	0x0b80, 0x000b, 0x0000,   0xfb01, 0x8007, 0x8020,
	0x0b80, 0x000b, 0x0000,   0xf601, 0x0007, 0x8023,
	0x0b80, 0x000b, 0x0000,   0x4202, 0x800b, 0x800e,
	0x0b80, 0x000b, 0x0000,   0x4302, 0x800b, 0x801d,
	0x0b80, 0x000b, 0x0000,   0xf701, 0x0007, 0x0025,
	0x0b80, 0x000b, 0x0000,   0xfd01, 0x8006, 0x003f,
	0x0b80, 0x000b, 0x0000,   0xfe01, 0x0007, 0x0046,
	0x0b80, 0x000b, 0x0000,   0xff01, 0x8007, 0x8049,
	0x0b80, 0x000b, 0x0000,   0xfc01, 0x8009, 0x0042,
	0xffff
};

static const UINT16 level_data_05[] = {
	0x0480, 0x1008, 0x0300,   0x5701, 0x0001, 0x0010,
	0x0480, 0x1008, 0x0300,   0x5701, 0x0001, 0x002b,
	0x0780, 0x0009, 0x0300,   0x4a01, 0x0004, 0x0020,
	0x0780, 0x1208, 0x0300,   0x5d01, 0x0004, 0x0030,
	0x0780, 0x0209, 0x0300,   0x4c01, 0x0004, 0x0038,
	0x0780, 0x0309, 0x0300,   0x4d01, 0x0004, 0x0048,
	0x0980, 0x1108, 0x0300,   0x5a01, 0xc005, 0x0018,
	0x0980, 0x0109, 0x0300,   0x4b01, 0xc005, 0x0028,
	0x0b80, 0x020a, 0x0000,   0x6401, 0x8006, 0x0004,
	0x0c80, 0x010b, 0x0000,   0xf201, 0x8006, 0x8002,
	0x0b80, 0x020a, 0x0000,   0x6401, 0x8006, 0x0017,
	0x0c80, 0x010b, 0x0000,   0xf201, 0x8006, 0x8015,
	0x0b80, 0x020a, 0x0000,   0x6401, 0x0007, 0x0034,
	0x0c80, 0x010b, 0x0000,   0xf201, 0x0007, 0x8032,
	0x0b80, 0x020a, 0x0000,   0x6401, 0x8006, 0x803e,
	0x0c80, 0x010b, 0x0000,   0xf201, 0x8006, 0x803d,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x0008,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x000b,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x001b,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x001e,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x8007, 0x0038,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x8007, 0x003b,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x8042,
	0x0b80, 0x100a, 0x0000,   0x6001, 0x0007, 0x8045,
	0x0c80, 0x000b, 0x0000,   0xf101, 0x800b, 0x8007,
	0x0c80, 0x000b, 0x0000,   0xf101, 0x800b, 0x801a,
	0x0c80, 0x000b, 0x0000,   0xf101, 0x000c, 0x8037,
	0x0c80, 0x000b, 0x0000,   0xf101, 0x800b, 0x0042,
	0x0c80, 0xd04b, 0x0000,   0xf301, 0x8006, 0x8009,
	0x0c80, 0xd04b, 0x0000,   0xf301, 0x8006, 0x801c,
	0x0c80, 0xd04b, 0x0000,   0xf301, 0x8006, 0x0044,
	0x0c80, 0x030b, 0x0000,   0xf401, 0x0008, 0x0028,
	0x0c80, 0x030b, 0x0000,   0xf401, 0x0008, 0x804b,
	0x0c00, 0x040b, 0x0000,   0xf501, 0x0008, 0x8026,
	0xffff
};

static const UINT16 level_data_06[] = {
	0x0000, 0x1008, 0x0300,   0x5701, 0x0001, 0x0010,
	0x0000, 0x1008, 0x0300,   0x5701, 0x0001, 0x002b,
	0x0000, 0x0000, 0x0000,   0x0000, 0x0000, 0x0000,
	0x0700, 0x0009, 0x0300,   0x4a01, 0x0004, 0x0020,
	0x0700, 0x1208, 0x0300,   0x5d01, 0x0004, 0x0030,
	0x0700, 0x0209, 0x0300,   0x4c01, 0x0004, 0x0038,
	0x0700, 0x0309, 0x0300,   0x4d01, 0x0004, 0x0048,
	0x0900, 0x1108, 0x0300,   0x5a01, 0xc005, 0x0018,
	0x0900, 0x0109, 0x0300,   0x4b01, 0xc005, 0x0028,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0980, 0xdb4c, 0x0000,   0x3202, 0x0006, 0x0004,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0x0000, 0x000b, 0x0000,   0x0000, 0x0018, 0x0000,
	0xffff
};

static const UINT16 level_data_07[] = {
	0x0480, 0x000b, 0x0300,   0x4501, 0x0001, 0x0001,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0780, 0x0109, 0x0300,   0x4a01, 0x0004, 0x0004,
	0x0780, 0x0009, 0x0300,   0x4a01, 0x0004, 0x000d,
	0x0780, 0x000c, 0x0500,   0x7b01, 0x000c, 0x0005,
	0x0780, 0x000c, 0x0540,   0x7b01, 0x000c, 0x0005,
	0x0780, 0x010c, 0x0300,   0x4601, 0x0005, 0x0005,
	0x0780, 0x000c, 0x0500,   0x7b01, 0x800b, 0xc00d,
	0x0780, 0x000c, 0x0540,   0x7b01, 0x800b, 0xc00d,
	0x0780, 0x010c, 0x0300,   0x4601, 0x8004, 0xc00d,
	0x0900, 0x0109, 0x0340,   0x4b01, 0x2006, 0x400c,
	0x0780, 0x020c, 0x0300,   0x4801, 0x8007, 0x0008,
	0x0780, 0x020c, 0x0300,   0x4801, 0x4007, 0xc00b,
	0x0980, 0x0109, 0x0300,   0x4b01, 0xc006, 0x8007,
	0x0980, 0x0109, 0x0300,   0x4b01, 0x8007, 0x8008,
	0x0980, 0x0109, 0x0300,   0x4b01, 0xc006, 0x800c,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0xffff
};

static const UINT16 level_data_08[] = {
	0xffff
};

static const UINT16 level_data_09[] = {
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0780, 0x0109, 0x0300,   0x4a01, 0x8003, 0x8003,
	0x0780, 0x0009, 0x0300,   0x4a01, 0x0004, 0x800e,
	0x0780, 0x000c, 0x0500,   0x7b01, 0x000c, 0x0005,
	0x0780, 0x000c, 0x0540,   0x7b01, 0x000c, 0x0005,
	0x0780, 0x010c, 0x0300,   0x4601, 0x0005, 0x0005,
	0x0780, 0x000c, 0x0500,   0x7b01, 0x800b, 0xc00d,
	0x0780, 0x000c, 0x0540,   0x7b01, 0x800b, 0xc00d,
	0x0780, 0x010c, 0x0300,   0x4601, 0x8004, 0xc00d,
	0x0900, 0x0109, 0x0340,   0x4b01, 0x2006, 0x400c,
	0x0780, 0x020c, 0x0300,   0x4801, 0x8007, 0x0008,
	0x0780, 0x020c, 0x0300,   0x4801, 0x4007, 0xc00b,
	0x0980, 0x0109, 0x0300,   0x4b01, 0xc006, 0x8007,
	0x0980, 0x0109, 0x0300,   0x4b01, 0x8007, 0x8008,
	0x0980, 0x0109, 0x0300,   0x4b01, 0xc006, 0x800c,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,   0xf001, 0x0000, 0x0000,
	0xffff
};

static const UINT16 *const level_data_lookup[] =
{
	level_data_00,
	level_data_01,
	level_data_02,
	level_data_03,
	level_data_04,
	level_data_05,
	level_data_06,
	level_data_07,
	level_data_08,
	level_data_09
};


static void timer_callback(int param)
{
/* Level data command */
	if (current_cmd == 0xf5)
	{
        int const level = cchip_ram[0x1b] % 10;
        UINT16 const *const level_data = level_data_lookup[level];
	 	unsigned i = 0;

        /* The c-chip data is stored as a series of 3 word sets, delimited by 0xffff */
        memset(cchip_ram + 0x200, 0, 0x200);
        for (i = 0; (i < 0x200) && (level_data[i] != 0xffff); i += 3)
		{
			cchip_ram[0x200 + i*2 + 0] = level_data[i]>>8;
			cchip_ram[0x200 + i*2 + 1] = level_data[i]&0xff;
			cchip_ram[0x200 + i*2 + 2] = level_data[i+1]>>8;
			cchip_ram[0x200 + i*2 + 3] = level_data[i+1]&0xff;
			cchip_ram[0x200 + i*2 + 4] = level_data[i+2]>>8;
			cchip_ram[0x200 + i*2 + 5] = level_data[i+2]&0xff;
		}

		/* The bootleg cchip writes 0 to these locations - hard to tell what the real one writes */
		cchip_ram[0x0]=0;
		cchip_ram[0x76]=0;
		cchip_ram[0x75]=0;
		cchip_ram[0x74]=0;
		cchip_ram[0x72]=0;
		cchip_ram[0x71]=0;
/*		cchip_ram[0x70]=0;  */ /* The bootleg writes this to disable mini-levels.  The real c-chip does not do this. */
		cchip_ram[0x66]=0;
		cchip_ram[0x2b]=0;
		cchip_ram[0x30]=0;
		cchip_ram[0x31]=0;
		cchip_ram[0x32]=0;
		cchip_ram[0x27]=0;
		c588=0;
		c589=0;
		c58a=0;

		cchip_ram[0x1a]=0;
		cchip_ram[0x7a]=1; /* Signal command complete */
	}

	current_cmd=0;
}

static void updateDifficulty(int mode)
{
	/* The game is made up of 6 rounds, when you complete the
	sixth you return to the start but with harder difficulty. 
	*/
	if (mode==0)
	{
		switch (cchip_ram[0x15]&3) /* Dipswitch B */
		{
		case 3:
			cchip_ram[0x2c]=0x31;
			cchip_ram[0x77]=0x05;
			cchip_ram[0x25]=0x0f;
			cchip_ram[0x26]=0x0b;
			break;
		case 0:
			cchip_ram[0x2c]=0x20;
			cchip_ram[0x77]=0x06;
			cchip_ram[0x25]=0x07;
			cchip_ram[0x26]=0x03;
			break;
		case 1:
			cchip_ram[0x2c]=0x31;
			cchip_ram[0x77]=0x05;
			cchip_ram[0x25]=0x0f;
			cchip_ram[0x26]=0x0b;
			break;
		case 2:
			cchip_ram[0x2c]=0x3c;
			cchip_ram[0x77]=0x04;
			cchip_ram[0x25]=0x13;
			cchip_ram[0x26]=0x0f;
			break;
		}
	}
	else
	{
		switch (cchip_ram[0x15]&3) /* Dipswitch B */
		{
		case 3:
			cchip_ram[0x2c]=0x46;
			cchip_ram[0x77]=0x5;
			cchip_ram[0x25]=0x11;
			cchip_ram[0x26]=0xe;
			break;
		case 0:
			cchip_ram[0x2c]=0x30;
			cchip_ram[0x77]=0x6;
			cchip_ram[0x25]=0xb;
			cchip_ram[0x26]=0x3;
			break;
		case 1:
			cchip_ram[0x2c]=0x3a;
			cchip_ram[0x77]=0x5;
			cchip_ram[0x25]=0xf;
			cchip_ram[0x26]=0x9;
			break;
		case 2:
			cchip_ram[0x2c]=0x4c;
			cchip_ram[0x77]=0x4;
			cchip_ram[0x25]=0x19;
			cchip_ram[0x26]=0x11;
			break;
		};
	}
}

/*************************************
 *
 * Writes to C-Chip - Important Bits
 *
 *************************************/

WRITE16_HANDLER( opwolf_cchip_status_w )
{
	/* This is written once after the C-Chip init is complete (and passes)
	We use it to setup some initial state (it's not clear if the real
	c-chip sets this here, or if it's as a side-effect of the other
	init sequence data). 
	*/

	cchip_ram[0x3d]=1;
	cchip_ram[0x7a]=1;
	updateDifficulty(0);
}

WRITE16_HANDLER( opwolf_cchip_bank_w )
{
	current_bank=data&7;
}

WRITE16_HANDLER( opwolf_cchip_data_w )
{
	cchip_ram[(current_bank * 0x400) + offset]=data&0xff;

	if (current_bank == 0)
	{
		/* Dip switch A is written here by the 68k - precalculate the coinage values */
		if (offset == 0x14)
		{
			switch (data&0x30)
			{
			case 0x00: cchip_coins_for_credit_a=4; break;
			case 0x10: cchip_coins_for_credit_a=3; break;
			case 0x20: cchip_coins_for_credit_a=2; break;
			case 0x30: cchip_coins_for_credit_a=1; break;
			}

			switch (data&0xc0)
			{
			case 0x00: cchip_credit_for_coin_b=6; break;
			case 0x40: cchip_credit_for_coin_b=4; break;
			case 0x80: cchip_credit_for_coin_b=3; break;
			case 0xc0: cchip_credit_for_coin_b=2; break;
			}
		}

		/* Dip switch B */
		if (offset == 0x15)
		{
			updateDifficulty(0);
		}
	}
}


/*************************************
 *
 * Reads from C-Chip
 *
 *************************************/

READ16_HANDLER( opwolf_cchip_status_r )
{
	/*
        Bit 0x4 = Error signal
        Bit 0x1 = Ready signal
    */
	return 0x1; /* Return 0x5 for C-Chip error */
}

READ16_HANDLER( opwolf_cchip_data_r )
{
	return cchip_ram[(current_bank * 0x400) + offset];
}

/*************************************
 *
 * C-Chip Tick
 *
 *************************************/

static void cchip_timer(int dummy)
{
	/* Update input ports, these are used by both the 68k directly and by the c-chip */
	cchip_ram[0x4]=input_port_0_word_r(0,0);
	cchip_ram[0x5]=input_port_1_word_r(0,0);

	/* Update coins - cchip handles coinage according to dipswitch A */
	if (cchip_ram[0x4]!=cchip_last_04)
	{
		/* Coin slot A */
		if (cchip_ram[0x4]&1)
		{
			cchip_coins++;
			if (cchip_coins >= cchip_coins_for_credit_a)
			{
				cchip_ram[0x53]++;
				cchip_ram[0x51]=0x55;
				cchip_ram[0x52]=0x55;
				cchip_coins=0;
			}
			coin_counter_w(0, 1);
		}

		/* Coin slot B */
		if (cchip_ram[0x4]&2)
		{
			cchip_ram[0x53]+=cchip_credit_for_coin_b;
			cchip_ram[0x51]=0x55;
			cchip_ram[0x52]=0x55;
			coin_counter_w(1, 1);
		}

		if (cchip_ram[0x53]>9)
			cchip_ram[0x53]=9;
	}
	cchip_last_04=cchip_ram[0x4];

	/* Service switch */
	if (cchip_ram[0x5]!=cchip_last_05)
	{
		if ((cchip_ram[0x5]&4)==0)
		{
			cchip_ram[0x53]++;
			cchip_ram[0x51]=0x55;
			cchip_ram[0x52]=0x55;
		}
	}
	cchip_last_05=cchip_ram[0x5];

	/* Cchip handles coin lockout (68k flags error if more than 9 coins) */
	coin_lockout_w(1, cchip_ram[0x53]==9);
	coin_lockout_w(0, cchip_ram[0x53]==9);
	coin_counter_w(0, 0);
	coin_counter_w(1, 0);

	/* Special handling for last level */
	if (cchip_ram[0x1b]==0x6)
	{
		/* Check for triggering final helicopter (end boss) */
		if (c58a==0)
		{
			if ((cchip_ram[0x72]&0x7f) >= 8 && cchip_ram[0x74]==0 && cchip_ram[0x1c]==0 && cchip_ram[0x1d]==0 && cchip_ram[0x1f]==0)
			{
				cchip_ram[0x30]=1;
				cchip_ram[0x74]=1;
				c58a=1;
			}
		}

		if (cchip_ram[0x1a]==0x90)
			cchip_ram[0x74]=0;

		if (c58a!=0)
		{
			if (c589==0 && cchip_ram[0x27]==0 && cchip_ram[0x75]==0 && cchip_ram[0x1c]==0 && cchip_ram[0x1d]==0 && cchip_ram[0x1e]==0 && cchip_ram[0x1f]==0)
			{
				cchip_ram[0x31]=1;
				cchip_ram[0x75]=1;
				c589=1;
			}
		}

		if (cchip_ram[0x2b]==0x1)
		{
			cchip_ram[0x2b]=0;

			if (cchip_ram[0x30]==0x1)
			{
				if (cchip_ram[0x1a]!=0x90)
					cchip_ram[0x1a]--;
			}

			if (cchip_ram[0x72]==0x9)
			{
				if (cchip_ram[0x76]!=0x4)
				{
					cchip_ram[0x76]=3;
				}
			}
			else
			{
				/* This timer is derived from the bootleg rather than the real board, I'm not 100% sure about it */
				c588|=0x80;

				cchip_ram[0x72]=c588;
				c588++;

				cchip_ram[0x1a]--;
				cchip_ram[0x1a]--;
				cchip_ram[0x1a]--;
			}
		}

		/* Update difficulty settings */
		if (cchip_ram[0x76]==0)
		{
			cchip_ram[0x76]=1;
			updateDifficulty(1);
		}
	}

	/* These variables are cleared every frame during attract mode and the intro. */
	if (cchip_ram[0x34] < 2)
	{
		updateDifficulty(0);
		cchip_ram[0x76]=0;
		cchip_ram[0x75]=0;
		cchip_ram[0x74]=0;
		cchip_ram[0x72]=0;
		cchip_ram[0x71]=0;
		cchip_ram[0x70]=0;
		cchip_ram[0x66]=0;
		cchip_ram[0x2b]=0;
		cchip_ram[0x30]=0;
		cchip_ram[0x31]=0;
		cchip_ram[0x32]=0;
		cchip_ram[0x27]=0;
		c588=0;
		c589=0;
		c58a=0;
	}

	/* Check for level completion (all enemies destroyed) */
	if (cchip_ram[0x1c] == 0 && cchip_ram[0x1d] == 0 && cchip_ram[0x1e] == 0 && cchip_ram[0x1f] == 0 && cchip_ram[0x20] == 0)
	{
		/* Special handling for end of level 6 */
		if (cchip_ram[0x1b]==0x6)
		{
			/* Don't signal end of level until final boss is destroyed */
			if (cchip_ram[0x27]==0x1)
				cchip_ram[0x32]=1;
		}
		else
		{
			/* Signal end of level */
			cchip_ram[0x32]=1;
		}
	}

	if (cchip_ram[0xe] == 1)
	{
		cchip_ram[0xe]=0xfd;
		cchip_ram[0x61]=0x04;
	}

	/* Access level data command (address 0xf5 goes from 1 -> 0) */
	if (cchip_ram[0x7a]==0 && cchip_last_7a!=0 && current_cmd!=0xf5)
	{
		/* Simulate time for command to execute (exact timing unknown, this is close) */
		current_cmd=0xf5;
		timer_set(TIME_IN_CYCLES(80000,0), 0, timer_callback);
	}
	cchip_last_7a=cchip_ram[0x7a];
/*
	This seems to some kind of periodic counter - results are expected
	by the 68k when the counter reaches 0xa
*/	
	if (cchip_ram[0x7f]==0xa)
	{
		cchip_ram[0xfe]=0xf7;
		cchip_ram[0xff]=0x6e;
	}

	/* These are set every frame */
	cchip_ram[0x64]=0;
	cchip_ram[0x66]=0;
}

/*************************************
 *
 * C-Chip State Saving
 *
 *************************************/

void opwolf_cchip_init(void)
{
	cchip_ram=auto_malloc(0x400 * 8);

	state_save_register_UINT8("opwolf", 0, "cc_bank", &current_bank, 1);
	state_save_register_UINT8("opwolf", 0, "cc_data", &current_cmd, 1);
	state_save_register_UINT8("opwolf", 0, "7a", &cchip_last_7a, 1);
	state_save_register_UINT8("opwolf", 0, "04", &cchip_last_04, 1);
	state_save_register_UINT8("opwolf", 0, "05", &cchip_last_05, 1);
	state_save_register_UINT8("opwolf", 0, "c588", &c588, 1);
	state_save_register_UINT8("opwolf", 0, "c589", &c589, 1);
	state_save_register_UINT8("opwolf", 0, "c58a", &c58a, 1);
	state_save_register_UINT8("opwolf", 0, "cc", &cchip_coins, 1);
	state_save_register_UINT8("opwolf", 0, "ca", &cchip_coins_for_credit_a, 1);
	state_save_register_UINT8("opwolf", 0, "cb", &cchip_credit_for_coin_b, 1);
	state_save_register_UINT8("opwolf", 0, "cc_ram", cchip_ram, 0x400 * 8);

	cchip_last_7a=0;
	cchip_last_04=0xfc;
	cchip_last_05=0xff;
	cchip_coins=0;
	cchip_coins_for_credit_a=1;
	cchip_credit_for_coin_b=2;

	timer_pulse(TIME_IN_HZ(60), 0, cchip_timer);
}


/**********************************************************
				GAME INPUTS
**********************************************************/

static READ16_HANDLER( opwolf_lightgun_r )
{
	int scaled;

	switch (offset)
	{
		case 0x00:	/* P1X - Have to remap 8 bit input value, into 0-319 visible range */
			scaled=(input_port_4_word_r(0,mem_mask) * 320 ) / 256;
			return (scaled + 0x15 + opwolf_gun_xoffs);
		case 0x01:	/* P1Y */
			return (input_port_5_word_r(0,mem_mask) - 0x24 + opwolf_gun_yoffs);
	}

	return 0xff;
}

static READ_HANDLER( z80_input1_r )
{
	return input_port_0_word_r(0,0);	/* irrelevant mirror ? */
}

static READ_HANDLER( z80_input2_r )
{
	return input_port_0_word_r(0,0);	/* needed for coins */
}


/******************************************************
				SOUND
******************************************************/

static WRITE_HANDLER( sound_bankswitch_w )
{
	cpu_setbank( 10, memory_region(REGION_CPU2) + ((data-1) & 0x03) * 0x4000 + 0x10000 );
}

/***********************************************************
             MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ16_START( opwolf_readmem )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x0f0000, 0x0f07ff, opwolf_cchip_data_r },
	{ 0x0ff000, 0x0ff7ff, opwolf_cchip_data_r }, /* mirror of cchip read*/
	{ 0x0f0802, 0x0f0803, opwolf_cchip_status_r },
	{ 0x0ff802, 0x0ff803, opwolf_cchip_status_r }, /* mirror of cchip read*/
	{ 0x100000, 0x107fff, MRA16_RAM },	/* RAM */
	{ 0x200000, 0x200fff, paletteram16_word_r },
	{ 0x380000, 0x380001, input_port_2_word_r },	/* DSW A */
	{ 0x380002, 0x380003, input_port_3_word_r },	/* DSW B */
	{ 0x3a0000, 0x3a0003, opwolf_lightgun_r },	/* lightgun, read at $11e0/6 */
	{ 0x3e0000, 0x3e0001, MRA16_NOP },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_msb_r },
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_r },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_r },	/* sprite ram */
MEMORY_END

static MEMORY_WRITE16_START( opwolf_writemem )
    { 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x0ff000, 0x0ff7ff, opwolf_cchip_data_w },
	{ 0x0ff802, 0x0ff803, opwolf_cchip_status_w },
	{ 0x0ffc00, 0x0ffc01, opwolf_cchip_bank_w },
	{ 0x100000, 0x107fff, MWA16_RAM },
	{ 0x200000, 0x200fff, paletteram16_xxxxRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x380000, 0x380003, rainbow_spritectrl_w },	/* usually 0x4, changes when you fire */
	{ 0x3c0000, 0x3c0001, MWA16_NOP },	/* watchdog ?? */
	{ 0x3e0000, 0x3e0001, taitosound_port16_msb_w },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_msb_w },
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_w },
	{ 0xc10000, 0xc1ffff, MWA16_RAM },	/* error in init code (?) */
	{ 0xc20000, 0xc20003, PC080SN_yscroll_word_0_w },
	{ 0xc40000, 0xc40003, PC080SN_xscroll_word_0_w },
	{ 0xc50000, 0xc50003, PC080SN_ctrl_word_0_w },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_w },	/* sprite ram */
MEMORY_END


static MEMORY_READ16_START( opwolfb_readmem )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x0f0008, 0x0f0009, input_port_0_word_r },	/* IN0 */
	{ 0x0f000a, 0x0f000b, input_port_1_word_r },	/* IN1 */
	{ 0x0ff000, 0x0fffff, cchip_r },
	{ 0x100000, 0x107fff, MRA16_RAM },	/* RAM */
	{ 0x200000, 0x200fff, paletteram16_word_r },
	{ 0x380000, 0x380001, input_port_2_word_r },	/* DSW A */
    { 0x380002, 0x380003, input_port_3_word_r },	/* DSW B */
	{ 0x3a0000, 0x3a0003, opwolf_lightgun_r },	/* lightgun, read at $11e0/6 */
	{ 0x3e0000, 0x3e0001, MRA16_NOP },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_msb_r },
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_r },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_r },	/* sprite ram */
MEMORY_END

static MEMORY_WRITE16_START( opwolfb_writemem )
    { 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x0ff000, 0x0fffff, cchip_w },
	{ 0x100000, 0x107fff, MWA16_RAM },
	{ 0x200000, 0x200fff, paletteram16_xxxxRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x380000, 0x380003, rainbow_spritectrl_w },	/* usually 0x4, changes when you fire */
	{ 0x3c0000, 0x3c0001, MWA16_NOP },	/* watchdog ?? */
	{ 0x3e0000, 0x3e0001, taitosound_port16_msb_w },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_msb_w },
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_w },
	{ 0xc10000, 0xc1ffff, MWA16_RAM },	/* error in init code (?) */
	{ 0xc20000, 0xc20003, PC080SN_yscroll_word_0_w },
	{ 0xc40000, 0xc40003, PC080SN_xscroll_word_0_w },
	{ 0xc50000, 0xc50003, PC080SN_ctrl_word_0_w },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_w },	/* sprite ram */
MEMORY_END

/***************************************************************************
    This extra Z80 substitutes for the c-chip in the bootleg
 */

static MEMORY_READ_START( sub_z80_readmem )
    { 0x0000, 0x7fff, MRA_ROM },
	{ 0x8800, 0x8800, z80_input1_r },	/* read at PC=$637: poked to $c004 */
	{ 0x9800, 0x9800, z80_input2_r },	/* read at PC=$631: poked to $c005 */
	{ 0xc000, 0xc7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sub_z80_writemem )
    { 0x0000, 0x7fff, MWA_ROM },
	{ 0x9000, 0x9000, MWA_NOP },	/* unknown write, 0 then 1 each interrupt */
	{ 0xa000, 0xa000, MWA_NOP },	/* IRQ acknowledge (unimplemented) */
	{ 0xc000, 0xc7ff, MWA_RAM, &cchip_ram },
MEMORY_END

/***************************************************************************/

static MEMORY_READ_START( z80_readmem )
    { 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK10 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
	{ 0x9002, 0x9100, MRA_RAM },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END


static int adpcm_pos[2],adpcm_end[2];

/*static unsigned char adpcm_d[0x08];
0 - start ROM offset LSB
1 - start ROM offset MSB
2 - end ROM offset LSB
3 - end ROM offset MSB
start & end need to be multiplied by 16 to get a proper _byte_ address in adpcm ROM
4 - always zero write (start trigger ?)
5 - different values
6 - different values 
*/

static MACHINE_INIT( opwolf )
{
	MSM5205_reset_w(0, 1);
	MSM5205_reset_w(1, 1);
}

static void opwolf_msm5205_vck(int chip)
{
	static int adpcm_data[2] = { -1, -1 };

	if (adpcm_data[chip] != -1)
	{
		MSM5205_data_w(chip, adpcm_data[chip] & 0x0f);
		adpcm_data[chip] = -1;
		if (adpcm_pos[chip] == adpcm_end[chip])
		{	
			MSM5205_reset_w(chip, 1);
		}	
	}
	else
	{
		adpcm_data[chip] = memory_region(REGION_SOUND1)[adpcm_pos[chip]];
		adpcm_pos[chip] = (adpcm_pos[chip] + 1) & 0x7ffff;
		MSM5205_data_w(chip, adpcm_data[chip] >> 4);
	}
}

static WRITE_HANDLER( opwolf_adpcm_b_w )
{
	int start;
	int end;

	adpcm_b[offset] = data;

	if (offset==0x04) /* trigger ? */
	{
		start = adpcm_b[0] + adpcm_b[1]*256;
		end   = adpcm_b[2] + adpcm_b[3]*256;
		start *=16;
		end   *=16;
		adpcm_pos[0] = start;
		adpcm_end[0] = end;
		MSM5205_reset_w(0, 0);
	}

/*  logerror("CPU #1     b00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() ); */
}


static WRITE_HANDLER( opwolf_adpcm_c_w )
{
	int start;
	int end;

	adpcm_c[offset] = data;

	if (offset==0x04) /* trigger ? */
	{
		start = adpcm_c[0] + adpcm_c[1]*256;
		end   = adpcm_c[2] + adpcm_c[3]*256;
		start *=16;
		end   *=16;
		adpcm_pos[1] = start;
		adpcm_end[1] = end;
		MSM5205_reset_w(1, 0);
	}

	/*logerror("CPU #1     c00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() );*/
}


static WRITE_HANDLER( opwolf_adpcm_d_w )
{
	/*logerror("CPU #1         d00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() );*/
}

static WRITE_HANDLER( opwolf_adpcm_e_w )
{
	/*logerror("CPU #1         e00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() );*/
}


static MEMORY_WRITE_START( z80_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_register_port_0_w },
	{ 0x9001, 0x9001, YM2151_data_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
	{ 0xb000, 0xb006, opwolf_adpcm_b_w },
	{ 0xc000, 0xc006, opwolf_adpcm_c_w },
	{ 0xd000, 0xd000, opwolf_adpcm_d_w },
	{ 0xe000, 0xe000, opwolf_adpcm_e_w },
MEMORY_END


/***********************************************************
			 INPUT PORTS, DIPs
***********************************************************/

#define TAITO_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_START( opwolf )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, "NY Conversion of Upright" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ))
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ))
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Ammo Magazines at Start" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )	/* Manual says all 3 unused */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )

	PORT_START	/* P1X (span allows you to shoot enemies behind status bar) */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 25, 15, 0x00, 0xff)

	PORT_START	/* P1Y (span allows you to be slightly offscreen) */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 25, 15, 0x00, 0xff)
INPUT_PORTS_END


/**************************************************************
				GFX DECODING
**************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4, 10*4, 11*4, 8*4, 9*4, 14*4, 15*4, 12*4, 13*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout_b =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout_b =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo opwolf_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,  0, 256 },	/* sprites */
	{ REGION_GFX1, 0, &charlayout,  0, 256 },	/* scr tiles */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo opwolfb_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout_b,  0, 256 },	/* sprites */
	{ REGION_GFX1, 0, &charlayout_b,  0, 256 },	/* scr tiles */
	{ -1 } /* end of array */
};


/**************************************************************
			     YM2151 (SOUND)
**************************************************************/

/* handler called by the YM2151 emulator when the internal timers cause an IRQ */

static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}


static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ irq_handler },
	{ sound_bankswitch_w }
};


static struct MSM5205interface msm5205_interface =
{
	2,					/* 2 chip             */
	384000,				/* 384KHz             */
	{ opwolf_msm5205_vck, opwolf_msm5205_vck },/* interrupt function */
	{ MSM5205_S48_4B, MSM5205_S48_4B  },	/* 8KHz               */
	{ 60, 60 }				/* volume */
};


/***********************************************************
			     MACHINE DRIVERS
***********************************************************/

static MACHINE_DRIVER_START( opwolf )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000 )	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(opwolf_readmem,opwolf_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000 )	/* 4 MHz */
	MDRV_CPU_MEMORY(z80_readmem,z80_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */

	MDRV_MACHINE_INIT(opwolf)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(opwolf_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(opwolf)
	MDRV_VIDEO_UPDATE(opwolf)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( opwolfb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(opwolf_readmem,opwolf_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_readmem,z80_writemem)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(sub_z80_readmem,sub_z80_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(opwolfb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(opwolf)
	MDRV_VIDEO_UPDATE(opwolf)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END


/***************************************************************************
					DRIVERS
***************************************************************************/

ROM_START( opwolf )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "opwlf.40",  0x00000, 0x10000, CRC(3ffbfe3a) SHA1(e41257e6af18bab4e36267a0c25a6aaa742972d2) )
	ROM_LOAD16_BYTE( "opwlf.30",  0x00001, 0x10000, CRC(fdabd8a5) SHA1(866ec6168489024b8d157f2d5b1553d7f6e3d9b7) )
	ROM_LOAD16_BYTE( "opwlf.39",  0x20000, 0x10000, CRC(216b4838) SHA1(2851cae00bb3e32e20f35fdab8ed6f149e658363) )
	ROM_LOAD16_BYTE( "opwlf.29",  0x20001, 0x10000, CRC(b71bc44c) SHA1(5b404bd7630f01517ab98bda40ca43c11268035a) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "opwlf_s.10",  0x00000, 0x04000, CRC(45c7ace3) SHA1(06f7393f6b973b7735c27e8380cb4148650cfc16) )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "opwlf.13",  0x00000, 0x80000, CRC(f6acdab1) SHA1(716b94ab3fa330ecf22df576f6a9f47a49c7554a) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "opwlf.72",  0x00000, 0x80000, CRC(89f889e5) SHA1(1592f6ce4fbb75e33d6ab957e5b90242a7a7a8c4) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "opwlf_s.21",   0x00000, 0x80000, CRC(f3e19c64) SHA1(39d48645f776c9c2ade537d959ecc6f9dc6dfa1b) )
ROM_END

ROM_START( opwolfb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "opwlfb.12",  0x00000, 0x10000, CRC(d87e4405) SHA1(de8a7763acd57293fbbff609e949ecd66c0f9234) )
	ROM_LOAD16_BYTE( "opwlfb.10",  0x00001, 0x10000, CRC(9ab6f75c) SHA1(85310258ca005ffb031e8d6b3f43c3d1fc29ef14) )
	ROM_LOAD16_BYTE( "opwlfb.13",  0x20000, 0x10000, CRC(61230c6e) SHA1(942764aec0c55ba00df8dbb54e127b73e24192ae) )
	ROM_LOAD16_BYTE( "opwlfb.11",  0x20001, 0x10000, CRC(342e318d) SHA1(a52918d16884ca42b2a3b910bc71bfd81b45f1ab) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "opwlfb.30",  0x00000, 0x04000, CRC(0669b94c) SHA1(f10894a6fad8ed144a528db696436b58f62ddee4) )
	ROM_CONTINUE(           0x10000, 0x04000 ) /* banked stuff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )      /* c-chip substitute Z80 */
	ROM_LOAD( "opwlfb.09",   0x00000, 0x08000, CRC(ab27a3dd) SHA1(cf589e7a9ccf3e86020b86f917fb91f3d8ba7512) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "opwlfb.08",   0x00000, 0x10000, CRC(134d294e) SHA1(bd05169dbd761c2944f0ac51c1ec114577777452) )	/* SCR tiles (8 x 8) */
	ROM_LOAD16_BYTE( "opwlfb.06",   0x20000, 0x10000, CRC(317d0e66) SHA1(70298c0ef5243f481b18f904be9404527d1d99d5) )
	ROM_LOAD16_BYTE( "opwlfb.07",   0x40000, 0x10000, CRC(e1c4095e) SHA1(d5f1d26d6612e78001002f92de670e68e00c6f9e) )
	ROM_LOAD16_BYTE( "opwlfb.05",   0x60000, 0x10000, CRC(fd9e72c8) SHA1(7a76f57641c3f0198565cd163188b581253173b2) )
	ROM_LOAD16_BYTE( "opwlfb.04",   0x00001, 0x10000, CRC(de0ca98d) SHA1(066e89ec0c64da14bdcd2b337f95c0de5de33c11) )
	ROM_LOAD16_BYTE( "opwlfb.02",   0x20001, 0x10000, CRC(6231fdd0) SHA1(1c830c106cf3c94a8d06ed2fff030a5d516ab6d6) )
	ROM_LOAD16_BYTE( "opwlfb.03",   0x40001, 0x10000, CRC(ccf8ba80) SHA1(8366f5ef0de885e5241567d1a083d98a8a2875d9) )
	ROM_LOAD16_BYTE( "opwlfb.01",   0x60001, 0x10000, CRC(0a65f256) SHA1(4dfcd3cb138a87d002eb65a02f94e33f4d07676d) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "opwlfb.14",   0x00000, 0x10000, CRC(663786eb) SHA1(a25710f6c16158e51d0934f184390a01ff0a614a) )	/* Sprites (16 x 16) */
	ROM_LOAD16_BYTE( "opwlfb.15",   0x20000, 0x10000, CRC(315b8aa9) SHA1(4a904e5532421d933e4c401c03c958eb32b15e03) )
	ROM_LOAD16_BYTE( "opwlfb.16",   0x40000, 0x10000, CRC(e01099e3) SHA1(4c5391d71978f72c57c140e58a767e138acdce12) )
	ROM_LOAD16_BYTE( "opwlfb.17",   0x60000, 0x10000, CRC(56fbe61d) SHA1(0e4dce8ee981bdd851e500fa9dca5d40908e142f) )
	ROM_LOAD16_BYTE( "opwlfb.18",   0x00001, 0x10000, CRC(de9ab08e) SHA1(ef674c965f35efaf747f1ddbf9e9164fcceb0c1c) )
	ROM_LOAD16_BYTE( "opwlfb.19",   0x20001, 0x10000, CRC(645cf85e) SHA1(91c244c2e238b61c8b2f39e5fa01cc23ebbfe2ce) )
	ROM_LOAD16_BYTE( "opwlfb.20",   0x40001, 0x10000, CRC(d80b9cc6) SHA1(b189f35eb206da1ab313620e251e6bb10edeee04) )
	ROM_LOAD16_BYTE( "opwlfb.21",   0x60001, 0x10000, CRC(97d25157) SHA1(cfb3f76ed860d90235dc0e32919a5ec3d3e683dd) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples (interleaved) */
	ROM_LOAD16_BYTE( "opwlfb.29",   0x00000, 0x10000, CRC(05a9eac0) SHA1(26eb1acc65aeb759920b35bcbcac6d6c2789584c) )
	ROM_LOAD16_BYTE( "opwlfb.28",   0x20000, 0x10000, CRC(281b2175) SHA1(3789e58da682041226f70eba87b31876cb206906) )
	ROM_LOAD16_BYTE( "opwlfb.27",   0x40000, 0x10000, CRC(441211a6) SHA1(82e84ae90765df5f7f6b6f32a2bb52ac40132f8d) )
	ROM_LOAD16_BYTE( "opwlfb.26",   0x60000, 0x10000, CRC(86d1d42d) SHA1(9d63e9e35fa51d8e6eac30556ba5a4dca7c14418) )
	ROM_LOAD16_BYTE( "opwlfb.25",   0x00001, 0x10000, CRC(85b87f58) SHA1(f26cf4ab8f9d30d1b1ac84be328ca821524b234e) )
	ROM_LOAD16_BYTE( "opwlfb.24",   0x20001, 0x10000, CRC(8efc5d4d) SHA1(21068d7fcfe293d99ad9f999d84483bf1a49ec6d) )
	ROM_LOAD16_BYTE( "opwlfb.23",   0x40001, 0x10000, CRC(a874c703) SHA1(c9d6074265f5d5028c69c81eaba29fa178943341) )
	ROM_LOAD16_BYTE( "opwlfb.22",   0x60001, 0x10000, CRC(9228481f) SHA1(8160f919f5e6a347c915a2bd7488b488fe2401bc) )
ROM_END


static DRIVER_INIT( opwolf )
{
	opwolf_cchip_init();

	opwolf_gun_xoffs = 0;
	opwolf_gun_yoffs = 0;

	/* (there are other sound vars that may need saving too) */
	state_save_register_UINT8("sound2", 0, "registers", adpcm_b, 8);
	state_save_register_UINT8("sound3", 0, "registers", adpcm_c, 8);
}

static DRIVER_INIT( opwolfb )
{
	/* bootleg needs different range of raw gun coords */
	opwolf_gun_xoffs = -2;
	opwolf_gun_yoffs = 17;

	/* (there are other sound vars that may need saving too) */
	state_save_register_UINT8("sound2", 0, "registers", adpcm_b, 8);
	state_save_register_UINT8("sound3", 0, "registers", adpcm_c, 8);
}



/*    year  rom       parent    machine   inp       init */
GAME( 1987, opwolf,   0,        opwolf,   opwolf,   opwolf,   ROT0, "Taito America Corporation", "Operation Wolf (US)" )
GAME( 1987, opwolfb,  opwolf,   opwolfb,  opwolf,   opwolfb,  ROT0, "bootleg", "Operation Bear" )
