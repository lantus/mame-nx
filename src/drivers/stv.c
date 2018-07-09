/* Sega ST-V (Sega Titan Video)

built to run the rom test mode only, don't consider anything here too accurate ;-)
we only run 1 sh2, not both, vidhrdw is just made to display bios text, interrupts
are mostly not done, most smpc commands not done, no scu / dsp stuff, no sound, you
get the idea ;-) 40ghz pc recommended once driver is finished

any rom which has a non-plain loaded rom at 0x2200000 (or 0x2000000, i think it
recognises a cart at either) appears to fail its self check, reason unknown, the roms
are almost certainly not bad its a mystery.

this hardware comes above hell on the great list of hellish things as far as emulation goes anyway ;-)

Preliminary Memory map:
0x00000000, 0x0007ffff  BIOS ROM
0x00080000, 0x000fffff  Unused
0x00100000, 0x00100080  SMPC
0x00100080, 0x0017ffff  Unused
0x00180000, 0x0018ffff  Back Up Ram
0x00190000, 0x001fffff  Unused
0x00200000, 0x002fffff  Work Ram-L
0x00300000, 0x00ffffff  Unused
0x01000000, 0x01000003  MINIT
0x01000004, 0x017fffff  Unused
0x01800000, 0x01800003  SINIT
0x01800004, 0x01ffffff  Unused
0x02000000, 0x03ffffff  A-BUS CS0
0x04000000, 0x04ffffff  A-BUS CS1
0x05000000, 0x057fffff  A-BUS DUMMY
0x05800000, 0x058fffff  A-BUS CS2
0x05900000, 0x059fffff  Unused
0x05a00000, 0x05b00ee3  Sound Region
0x05b00ee4, 0x05bfffff  Unused
0x05c00000, 0x05cbffff  VDP 1
0x05cc0000, 0x05cfffff  Unused
0x05d00000, 0x05d00017  VDP 1 regs
0x05d00018, 0x05dfffff  Unused
0x05e00000, 0x05e7ffff  VDP2
0x05e80000, 0x05efffff  VDP2 Extra RAM,accessible thru the VRAMSZ register
0x05f00000, 0x05f00fff  VDP2 color RAM
0x05f01000, 0x05f7ffff  Unused
0x05f80000  0x05f8011f  VDP2 regs
0x05f80120, 0x05fdffff  Unused
0x05fe0000, 0x05fe00cf  SCU regs
0x05fe00d0, 0x05ffffff  Unused
0x06000000, 0x060fffff  Work Ram H
0x06100000, 0x07ffffff  Unused

*the unused locations aren't known if they are really unused or not,needs verification...

Version Log(for reference,to be removed):
25/06/2003(Angelo Salese)
\-Added proper Color RAM format & CRMD emulation.
\-Fixed current cell display used,adding colors and flipx/y support to it.
\-Added some extra SMPC commands,and commented the rest.
\-Added dates & manufacturers to the various games.

23/07/2003(Angelo Salese)
\-Fixed the C.R.T test hang by writing a preliminary I/O handling.
\-Done some modifications to the VDP2 handling:this includes a better gfxdecode
  handling and the addition of the N1,N2 and N3 planes.
*/

#include "driver.h"

static data8_t *smpc_ram;
static void stv_dump_ram(void);

static data32_t* stv_workram_l;
static data32_t* stv_workram_h;
static data32_t* stv_scu;
static data32_t* stv_a0_vram,*stv_a1_vram,*stv_b0_vram,*stv_b1_vram;
static data32_t* stv_cram;
static data32_t* stv_vdp2_regs;
static data32_t* ioga;

static char stv_b1_dirty[0x2000],stv_b0_dirty[0x2000],stv_a1_dirty[0x2000],stv_a0_dirty[0x2000];

/*SMPC stuff*/
static unsigned char nmi_enabled;/*for NMI enable,dunno where it could be used...*/
/*SCU stuff*/

/*VDP1 stuff*/

/*VDP2 stuff*/
static unsigned char 	CRMD,/*Color Mode*/
						xxON,/*Screen display enable bit*/
					    CHCTLN0,/*Character Control Register for N0*/
					    CLOFEN,/*Color Offset Enable bits*/
					    CLOFSL;/*Color Offset Select bits*/
static int			    PNCN0;/*Pattern Name Control Register for N0*/

VIDEO_START(stv)
{
	return 0;
}


/* a complete hack just to display the text graphics used in the test mode */
VIDEO_UPDATE(stv)
{
	int i;
	data8_t *b1tile_gfxregion = memory_region(REGION_GFX1);
	data8_t *b0tile_gfxregion = memory_region(REGION_GFX2);
	data8_t *a1tile_gfxregion = memory_region(REGION_GFX3);
	data8_t *a0tile_gfxregion = memory_region(REGION_GFX4);
	int address,data1,data2,flipx1,flipx2,flipy1,flipy2,color1,color2;

	for(i = 0;i < 0x2000;i++)
		if(stv_b1_dirty[i] == 1)
		{
			stv_b1_dirty[i] = 0;
			decodechar(Machine->gfx[0], i, (data8_t*)b1tile_gfxregion, Machine->drv->gfxdecodeinfo[0].gfxlayout);
		}

	for(i = 0;i < 0x2000;i++)
		if(stv_b0_dirty[i] == 1)
		{
			stv_b0_dirty[i] = 0;
			decodechar(Machine->gfx[1], i, (data8_t*)b0tile_gfxregion, Machine->drv->gfxdecodeinfo[1].gfxlayout);
		}

	for(i = 0;i < 0x2000;i++)
		if(stv_a1_dirty[i] == 1)
		{
			stv_a1_dirty[i] = 0;
			decodechar(Machine->gfx[2], i, (data8_t*)a1tile_gfxregion, Machine->drv->gfxdecodeinfo[2].gfxlayout);
		}

	for(i = 0;i < 0x2000;i++)
		if(stv_a0_dirty[i] == 1)
		{
			stv_a0_dirty[i] = 0;
			decodechar(Machine->gfx[3], i, (data8_t*)a0tile_gfxregion, Machine->drv->gfxdecodeinfo[3].gfxlayout);
		}

	if ( keyboard_pressed_memory(KEYCODE_W) ) stv_dump_ram();

/*N0*/
/*Character Size                   : 1 h x 1 v*/
/*Character Color count            : 16 colors*/
/*Character number supplement mode : mode 0   */
	if(xxON & 8)
	{
		int x,y;

		for (y = 0; y < 128; y++){
			for (x = 0; x < 32; x++){
				/*First $2000 is for gfx-decoding*/
				address = 0x2000/4;

				address += (x + y*32);

#if 0
				if(CHCTLN0 & 1)
					usrintf_showmessage("Warning: 2 H x 2 V currently used");
#endif
				data1=(stv_b1_vram[address] & 0x03ff0000) >> 16;
				data2=(stv_b1_vram[address] & 0x000003ff) >> 0;
				flipy1=(stv_b1_vram[address] & 0x08000000) >> 27;
				flipy2=(stv_b1_vram[address] & 0x00000800) >> 11;
				flipx1=(stv_b1_vram[address] & 0x04000000) >> 26;
				flipx2=(stv_b1_vram[address] & 0x00000400) >> 10;
				color1=(stv_b1_vram[address] & 0xf0000000) >> 28;
				color2=(stv_b1_vram[address] & 0x0000f000) >> 12;
				/*Needs better VDP2/VDP2 regs support to avoid this kludge...*/
				color1+=0x40;
				color2+=0x40;

/*Doesn't work???*/
//				data1+= ((PNCN0 & 0x001f) >> 0)*0x400;
//				data2+= ((PNCN0 & 0x001f) >> 0)*0x400;
/*				color1+=((PNCN0 & 0x00e0) >> 5)*0x10;
				color2+=((PNCN0 & 0x00e0) >> 5)*0x10;
*/
				drawgfx(bitmap,Machine->gfx[0],data1,color1,flipx1,flipy1,x*16,y*8,cliprect,TRANSPARENCY_NONE,0);
				drawgfx(bitmap,Machine->gfx[0],data2,color2,flipx2,flipy2,x*16+8,y*8,cliprect,TRANSPARENCY_NONE,0);

			}

		}

	}
/* // stv logo
	{
		int x,y;

		for (y = 0; y < 128; y++){
			for (x = 0; x < 32; x++){

				int address;
				int data1;
				int data2;

				address = 0x2000/4;

				address += (x + y*32);

				data1=(stv_vram[address] & 0x0fff0000) >> 16;
				data2=(stv_vram[address] & 0x00000fff) >> 0;

				drawgfx(bitmap,Machine->gfx[1],data1/2,0,0,0,x*16,y*8,cliprect,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,Machine->gfx[1],data2/2,0,0,0,x*16+8,y*8,cliprect,TRANSPARENCY_PEN,0);




			}

		}


	}*/
/*N1*/
/*Character Size                   : 1 h x 1 v*/
/*Character Color count            : 16 colors*/
/*Character number supplement mode : mode 0   */
	if(xxON & 4)
	{
		int x,y;

		for (y = 0; y < 128; y++){
			for (x = 0; x < 32; x++){
				/*First $2000 is for gfx-decoding*/
				address = 0x2000/4;

				address += (x + y*32);


				data1=(stv_b0_vram[address] & 0x03ff0000) >> 16;
				data2=(stv_b0_vram[address] & 0x000003ff) >> 0;
				flipy1=(stv_b0_vram[address] & 0x08000000) >> 27;
				flipy2=(stv_b0_vram[address] & 0x00000800) >> 11;
				flipx1=(stv_b0_vram[address] & 0x04000000) >> 26;
				flipx2=(stv_b0_vram[address] & 0x00000400) >> 10;
				color1=(stv_b0_vram[address] & 0xf0000000) >> 28;
				color2=(stv_b0_vram[address] & 0x0000f000) >> 12;
				/*Needs better VDP2/VDP2 regs support to avoid this kludge...*/
				color1+=0x40;
				color2+=0x40;

/*Doesn't work???*/
//				data1+= ((PNCN0 & 0x001f) >> 0)*0x400;
//				data2+= ((PNCN0 & 0x001f) >> 0)*0x400;
/*				color1+=((PNCN0 & 0x00e0) >> 5)*0x10;
				color2+=((PNCN0 & 0x00e0) >> 5)*0x10;
*/
				drawgfx(bitmap,Machine->gfx[1],data1,color1,flipx1,flipy1,x*16,y*8,cliprect,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,Machine->gfx[1],data2,color2,flipx2,flipy2,x*16+8,y*8,cliprect,TRANSPARENCY_PEN,0);

			}

		}

	}
/*N2*/
/*Character Size                   : 1 h x 1 v*/
/*Character Color count            : 16 colors*/
/*Character number supplement mode : mode 0   */
	if(xxON & 2)
	{
		int x,y;

		for (y = 0; y < 128; y++){
			for (x = 0; x < 32; x++){
				/*First $2000 is for gfx-decoding*/
				address = 0x2000/4;

				address += (x + y*32);

				data1=(stv_a1_vram[address] & 0x03ff0000) >> 16;
				data2=(stv_a1_vram[address] & 0x000003ff) >> 0;
				flipy1=(stv_a1_vram[address] & 0x08000000) >> 27;
				flipy2=(stv_a1_vram[address] & 0x00000800) >> 11;
				flipx1=(stv_a1_vram[address] & 0x04000000) >> 26;
				flipx2=(stv_a1_vram[address] & 0x00000400) >> 10;
				color1=(stv_a1_vram[address] & 0xf0000000) >> 28;
				color2=(stv_a1_vram[address] & 0x0000f000) >> 12;
				/*Needs better VDP2/VDP2 regs support to avoid this kludge...*/
				color1+=0x40;
				color2+=0x40;

	/*Doesn't work???*/
	//				data1+= ((PNCN0 & 0x001f) >> 0)*0x400;
	//				data2+= ((PNCN0 & 0x001f) >> 0)*0x400;
	/*				color1+=((PNCN0 & 0x00e0) >> 5)*0x10;
					color2+=((PNCN0 & 0x00e0) >> 5)*0x10;
	*/
				drawgfx(bitmap,Machine->gfx[2],data1,color1,flipx1,flipy1,x*16,y*8,cliprect,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,Machine->gfx[2],data2,color2,flipx2,flipy2,x*16+8,y*8,cliprect,TRANSPARENCY_PEN,0);

			}

		}

	}

/*N3*/
/*Character Size                   : 1 h x 1 v*/
/*Character Color count            : 16 colors*/
/*Character number supplement mode : mode 0   */
	if(xxON & 1)
	{
		int x,y;

		for (y = 0; y < 128; y++){
			for (x = 0; x < 32; x++){

				/*First $2000 is for gfx-decoding*/
				address = 0x2000/4;

				address += (x + y*32);

				data1=(stv_a0_vram[address] & 0x03ff0000) >> 16;
				data2=(stv_a0_vram[address] & 0x000003ff) >> 0;
				flipy1=(stv_a0_vram[address] & 0x08000000) >> 27;
				flipy2=(stv_a0_vram[address] & 0x00000800) >> 11;
				flipx1=(stv_a0_vram[address] & 0x04000000) >> 26;
				flipx2=(stv_a0_vram[address] & 0x00000400) >> 10;
				color1=(stv_a0_vram[address] & 0xf0000000) >> 28;
				color2=(stv_a0_vram[address] & 0x0000f000) >> 12;
				/*Needs better VDP2/VDP2 regs support to avoid this kludge...*/
				color1+=0x40;
				color2+=0x40;

/*Doesn't work???*/
//				data1+= ((PNCN0 & 0x001f) >> 0)*0x400;
//				data2+= ((PNCN0 & 0x001f) >> 0)*0x400;
/*				color1+=((PNCN0 & 0x00e0) >> 5)*0x10;
				color2+=((PNCN0 & 0x00e0) >> 5)*0x10;
*/
				drawgfx(bitmap,Machine->gfx[3],data1,color1,flipx1,flipy1,x*16,y*8,cliprect,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,Machine->gfx[3],data2,color2,flipx2,flipy2,x*16+8,y*8,cliprect,TRANSPARENCY_PEN,0);

			}

		}

	}
}

/* SMPC
 System Manager and Peripheral Control

*/
/* SMPC Addresses

00
01 -w  Input Register 0 (IREG)
02
03 -w  Input Register 1
04
05 -w  Input Register 2
06
07 -w  Input Register 3
08
09 -w  Input Register 4
0a
0b -w  Input Register 5
0c
0d -w  Input Register 6
0e
0f
10
11
12
13
14
15
16
17
18
19
1a
1b
1c
1d
1e
1f -w  Command Register (COMREG)
20
21 r-  Output Register 0 (OREG)
22
23 r-  Output Register 1
24
25 r-  Output Register 2
26
27 r-  Output Register 3
28
29 r-  Output Register 4
2a
2b r-  Output Register 5
2c
2d r-  Output Register 6
2e
2f r-  Output Register 7
30
31 r-  Output Register 8
32
33 r-  Output Register 9
34
35 r-  Output Register 10
36
37 r-  Output Register 11
38
39 r-  Output Register 12
3a
3b r-  Output Register 13
3c
3d r-  Output Register 14
3e
3f r-  Output Register 15
40
41 r-  Output Register 16
42
43 r-  Output Register 17
44
45 r-  Output Register 18
46
47 r-  Output Register 19
48
49 r-  Output Register 20
4a
4b r-  Output Register 21
4c
4d r-  Output Register 22
4e
4f r-  Output Register 23
50
51 r-  Output Register 24
52
53 r-  Output Register 25
54
55 r-  Output Register 26
56
57 r-  Output Register 27
58
59 r-  Output Register 28
5a
5b r-  Output Register 29
5c
5d r-  Output Register 30
5e
5f r-  Output Register 31
60
61 r-  SR
62
63 rw  SF
64
65
66
67
68
69
6a
6b
6c
6d
6e
6f
70
71
72
73
74
75 rw PDR1
76
77 rw PDR2
78
79 -w DDR1
7a
7b -w DDR2
7c
7d -w IOSEL2/1
7e
7f -w EXLE2/1
*/

static UINT8 stv_SMPC_r8 (int offset)
{
//	logerror ("8-bit SMPC Read from Offset %02x Returns %02x\n", offset, smpc_ram[offset]);
	if (offset == 0x75)
		return readinputport(3);//temporary kludged

	if (offset == 0x77)
		return readinputport(2);

	return smpc_ram[offset];
}

static void stv_SMPC_w8 (int offset, UINT8 data)
{
//	logerror ("8-bit SMPC Write to Offset %02x with Data %02x\n", offset, data);
	smpc_ram[offset] = data;

	if (offset == 0x1f)
	{
		switch (data)
		{
			case 0x00:
				logerror ("SMPC: Master ON\n");
				smpc_ram[0x5f]=0x00;
				break;
			//in theory 0x01 is for Master OFF,but obviously is not used.
			case 0x02:
				logerror ("SMPC: Slave ON\n");
				smpc_ram[0x5f]=0x02;
				break;
			case 0x03:
				logerror ("SMPC: Slave OFF\n");
				smpc_ram[0x5f]=0x03;
				break;
			case 0x06:
				logerror ("SMPC: Sound ON\n");
				smpc_ram[0x5f]=0x06;
				break;
			case 0x07:
				logerror ("SMPC: Sound OFF\n");
				smpc_ram[0x5f]=0x07;
				break;
			/*CD (SH-1) ON/OFF,guess that's needed for Sports Fishing games...*/
			//case 0x08:
			//case 0x09:
			case 0x0d:
				logerror ("SMPC: System Reset\n");
				smpc_ram[0x5f]=0x0d;
				cpu_set_reset_line(0, PULSE_LINE);
				break;
			case 0x0e:
				logerror ("SMPC: Change Clock to 352\n");
				smpc_ram[0x5f]=0x0e;
				cpu_set_nmi_line(0,PULSE_LINE); // ff said this causes nmi, should we set a timer then nmi?
				break;
			case 0x0f:
				logerror ("SMPC: Change Clock to 320\n");
				smpc_ram[0x5f]=0x0f;
				cpu_set_nmi_line(0,PULSE_LINE); // ff said this causes nmi, should we set a timer then nmi?
				break;
			/*"Interrupt Back"*/
			case 0x10:
				logerror ("SMPC: Status Acquire\n");
				smpc_ram[0x5f]=0x10;
				/*This is for RTC,cartridge code and similar stuff...*/
			break;
			/* RTC write*/
			case 0x16:
				logerror("SMPC: RTC write\n");
				smpc_ram[0x2f] = smpc_ram[0x0d];
				smpc_ram[0x2d] = smpc_ram[0x0b];
				smpc_ram[0x2b] = smpc_ram[0x09];
				smpc_ram[0x29] = smpc_ram[0x07];
				smpc_ram[0x27] = smpc_ram[0x05];
				smpc_ram[0x25] = smpc_ram[0x03];
				smpc_ram[0x23] = smpc_ram[0x01];
				smpc_ram[0x5f]=0x16;
			break;
			/* SMPC memory setting*/
			case 0x17:
				logerror ("SMPC: memory setting\n");
				smpc_ram[0x5f]=0x17;
			break;
			case 0x18:
				logerror ("SMPC: NMI request\n");
				smpc_ram[0x5f]=0x18;
				/*NMI is unconditionally requested*/
				cpu_set_nmi_line(0,PULSE_LINE);
				break;
			case 0x19:
				logerror ("SMPC: NMI Enable\n");
				smpc_ram[0x5f]=0x19;
				nmi_enabled = 1;
				break;
			case 0x1a:
				logerror ("SMPC: NMI Disable\n");
				smpc_ram[0x5f]=0x1a;
				nmi_enabled = 0;
				break;
			//default:
			//	logerror ("SMPC: Unhandled Command %02x\n",data);
		}

		// we've processed the command, clear status flag
		smpc_ram[0x63] = 0x00;
		/*We have to simulate the timing of each command somehow...*/
	}
}


static READ32_HANDLER ( stv_SMPC_r32 )
{
	int byte = 0;
	int readdata = 0;
	/* registers are all byte accesses, convert here */
	offset = offset << 2; // multiply offset by 4

	if (!(mem_mask & 0xff000000))	{ byte = 0; readdata = stv_SMPC_r8(offset+byte) << 24; }
	if (!(mem_mask & 0x00ff0000))	{ byte = 1; readdata = stv_SMPC_r8(offset+byte) << 16; }
	if (!(mem_mask & 0x0000ff00))	{ byte = 2; readdata = stv_SMPC_r8(offset+byte) << 8;  }
	if (!(mem_mask & 0x000000ff))	{ byte = 3; readdata = stv_SMPC_r8(offset+byte) << 0;  }

	return readdata;
}


static WRITE32_HANDLER ( stv_SMPC_w32 )
{
	int byte = 0;
	int writedata = 0;
	/* registers are all byte accesses, convert here so we can use the data more easily later */
	offset = offset << 2; // multiply offset by 4

	if (!(mem_mask & 0xff000000))	{ byte = 0; writedata = data >> 24; }
	if (!(mem_mask & 0x00ff0000))	{ byte = 1; writedata = data >> 16; }
	if (!(mem_mask & 0x0000ff00))	{ byte = 2; writedata = data >> 8;  }
	if (!(mem_mask & 0x000000ff))	{ byte = 3; writedata = data >> 0;  }

	writedata &= 0xff;

	offset += byte;

	stv_SMPC_w8(offset,writedata);
}

static READ32_HANDLER ( stv_vdp2_regs_r32 )
{
//	if (offset!=1) logerror ("VDP2: Read from Registers, Offset %04x\n",offset);
	switch(offset)
	{
		case 1:
		/*Screen Status Register*/
		/*VBLANK & HBLANK(bit 3 & 2 of high word),fake for now*/
			stv_vdp2_regs[offset] ^= 0x000c0000;
		break;
		case 3:
		/*(V)RAM Control Register*/
		/*Color RAM Mode (bit 13 & 12) (CRMD1 & CRMD0) */
			CRMD = ((stv_vdp2_regs[offset] & 0x00003000) >> 12);
		break;
	}
	return stv_vdp2_regs[offset];
}

static WRITE32_HANDLER ( stv_vdp2_regs_w32 )
{
	COMBINE_DATA(&stv_vdp2_regs[offset]);

	/*This is just for debugging ATM.*/
	switch(offset)
	{
		case 8:
		/*Screen Display enable bit*/
		 	xxON = ((stv_vdp2_regs[offset] & 0x003f0000) >> 16);
		break;
		case 10:
		/*Character Control Register*/
			CHCTLN0 = ((stv_vdp2_regs[offset] & 0x007f0000) >> 16);
		break;
		case 12:
		/*Pattern Name Control Register*/
			PNCN0 = ((stv_vdp2_regs[offset] & 0xffff0000) >> 16);
		break;
		/* Color Offset Enable/Select Register */
		case 36:
			CLOFEN = ((stv_vdp2_regs[offset] & 0x007f0000) >> 16);
			CLOFSL = ((stv_vdp2_regs[offset] & 0x0000007f) >> 0);
		break;
		/* Is this supposed to change when the warning msg appears?*/
		case 37:
		/* Color Offset Register*/
			if(stv_vdp2_regs[offset] != 0)
			{
			logerror("Color offset A register set RRRRGGGG \n"
			 		 "                            %08x\n",stv_vdp2_regs[offset]);
			}
		break;
		case 38:
			if(stv_vdp2_regs[offset] != 0)
			{
			logerror("Color offset A/B register set BBBBRRRR\n"
			 		 "                              %08x\n",stv_vdp2_regs[offset]);
			}
		break;
		case 39:
			if(stv_vdp2_regs[offset] != 0)
			{
			logerror("Color offset B register set BBBBRRRR\n"
			 	     "                            %08x\n",stv_vdp2_regs[offset]);
			}
		break;
		}
}

static void stv_dump_ram()
{
	FILE *fp;

	fp=fopen("workraml.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_workram_l, 0x00100000, 1, fp);
		fclose(fp);
	}
	fp=fopen("workramh.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_workram_h, 0x00100000, 1, fp);
		fclose(fp);
	}
	fp=fopen("scu.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_scu, 0xd0, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_a0_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_a0_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_a1_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_a1_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_b0_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_b0_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_b1_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_b1_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("cram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_cram, 0x00080000, 1, fp);
		fclose(fp);
	}

}
static READ32_HANDLER( a0_vdp2_r )	{ return stv_a0_vram[offset]; }
static READ32_HANDLER( a1_vdp2_r )  { return stv_a1_vram[offset]; }
static READ32_HANDLER( b0_vdp2_r )	{ return stv_b0_vram[offset]; }
static READ32_HANDLER( b1_vdp2_r )	{ return stv_b1_vram[offset]; }

static WRITE32_HANDLER ( b1_vdp2_w )
{
	data8_t *btiles = memory_region(REGION_GFX1);

	COMBINE_DATA (&stv_b1_vram[offset]);

	data = stv_b1_vram[offset];
	/* put in gfx region for easy decoding */
	btiles[offset*4+0] = (data & 0xff000000) >> 24;
	btiles[offset*4+1] = (data & 0x00ff0000) >> 16;
	btiles[offset*4+2] = (data & 0x0000ff00) >> 8;
	btiles[offset*4+3] = (data & 0x000000ff) >> 0;

	stv_b1_dirty[offset/2] = 1;
}

static WRITE32_HANDLER ( b0_vdp2_w )
{
	data8_t *btiles = memory_region(REGION_GFX2);

	COMBINE_DATA (&stv_b0_vram[offset]);

	data = stv_b0_vram[offset];
	/* put in gfx region for easy decoding */
	btiles[offset*4+0] = (data & 0xff000000) >> 24;
	btiles[offset*4+1] = (data & 0x00ff0000) >> 16;
	btiles[offset*4+2] = (data & 0x0000ff00) >> 8;
	btiles[offset*4+3] = (data & 0x000000ff) >> 0;

	stv_b0_dirty[offset/2] = 1;
}


static WRITE32_HANDLER ( a1_vdp2_w )
{
	data8_t *btiles = memory_region(REGION_GFX3);

	COMBINE_DATA (&stv_a1_vram[offset]);

	data = stv_a1_vram[offset];
	/* put in gfx region for easy decoding */
	btiles[offset*4+0] = (data & 0xff000000) >> 24;
	btiles[offset*4+1] = (data & 0x00ff0000) >> 16;
	btiles[offset*4+2] = (data & 0x0000ff00) >> 8;
	btiles[offset*4+3] = (data & 0x000000ff) >> 0;

	stv_a1_dirty[offset/2] = 1;
}

static WRITE32_HANDLER ( a0_vdp2_w )
{
	data8_t *btiles = memory_region(REGION_GFX4);

	COMBINE_DATA (&stv_a0_vram[offset]);

	data = stv_a0_vram[offset];
	/* put in gfx region for easy decoding */
	btiles[offset*4+0] = (data & 0xff000000) >> 24;
	btiles[offset*4+1] = (data & 0x00ff0000) >> 16;
	btiles[offset*4+2] = (data & 0x0000ff00) >> 8;
	btiles[offset*4+3] = (data & 0x000000ff) >> 0;

	stv_a0_dirty[offset/2] = 1;

}

static INTERRUPT_GEN(stv_interrupt)
{
	/* i guess this is wrong ;-) */
	switch(cpu_getiloops())
	{
		case 0: cpu_set_irq_line_and_vector(0, 15, HOLD_LINE , 0x40); break;
		case 1: cpu_set_irq_line_and_vector(0, 14, HOLD_LINE , 0x41); break;
	}
}

/*
I/O overview:
	PORT-A  1player input
	PORT-B  2player input
	PORT-C  system input
	PORT-D  system output
	PORT-E  I/O 1
	PORT-F  I/O 2
	PORT-G  I/O 3
	PORT-AD AD-Stick inputs?(Fake for now...)
	SERIAL COM
*/
static unsigned char port_ad[] =
{
	0xcc,0xb2,0x99,0x7f,0x66,0x4c,0x33,0x19
};

READ32_HANDLER ( stv_io_r32 )
{
	static int i= -1;
	//logerror("(PC=%08X): I/O r %08X & %08X\n", activecpu_get_pc(), offset*4, mem_mask);

	switch(offset)
	{
		case 0:
		return (readinputport(0) << 16) | (readinputport(1));
		case 1:
		return (readinputport(2) << 16) | (ioga[1]);
		case 7:
		i++;
		if(i > 7)
			i = 0;
		return port_ad[i];
		default:
		return ioga[offset];
	}
}

WRITE32_HANDLER ( stv_io_w32 )
{
	//logerror("(PC=%08X): I/O w %08X = %08X & %08X\n", activecpu_get_pc(), offset*4, data, mem_mask);

	switch(offset)
	{
		case 0:
		break;
		case 1:
			if( (mem_mask & 0x000000ff) == 0)
				ioga[1] = (data*0x00010000)+(data);
		break;
		case 2:
			if( (mem_mask & 0x00ff0000) == 0)
			{
				ioga[2] = data/0x00010000;
				ioga[3] = data;
			}
			if( (mem_mask & 0x000000ff) == 0)
				ioga[2] = data*0x00010000;
		break;
		case 5:
			if( (mem_mask & 0x00ff0000) == 0)
				ioga[6] = data/0x00010000;
		break;
		case 7:
			ioga[7] = data;
		break;
	}
}

/*
READ32_HANDLER (read_cart)
{
	usrintf_showmessage("read cart address %08x",0x02200000+offset*4);
	return 0xff;
}
*/

READ32_HANDLER( stv_palette_r )
{
	return stv_cram[offset];
}

/*
One of the features of Sega ST-V is that the Color RAM could use two+one
different formats of Paletteram:
(1)Mode 0:RGB up to 5 bits per color,for 1024 possible combinations.16-bit format.
(2)Mode 1:Same as mode 0 but with 2048 possible combinations.
(3)Mode 2:RGB up to 8 bits per color,for 1024 possible combinations.32-bit format.
Notice that if it's currently using the mode 0/1,the first three bits (aka bits 0,1 and 2)
aren't used in output data(they are filled with 0).
The MSB in any mode is known to be used as "Color Calculation"(transparency).
*/

WRITE32_HANDLER( stv_palette_w )
{
	int r,g,b;
	COMBINE_DATA(&stv_cram[offset]);

	switch( CRMD )
	{
		/*Mode 2/3*/
		case 2:
		case 3:
			b = ((stv_cram[offset] & 0x00ff0000) >> 16);
			g = ((stv_cram[offset] & 0x0000ff00) >> 8);
			r = ((stv_cram[offset] & 0x000000ff) >> 0);
			palette_set_color(offset,r,g,b);
		break;
		/*Mode 0/1*/
		default:
			b = ((stv_cram[offset] & 0x00007c00) >> 10);
			g = ((stv_cram[offset] & 0x000003e0) >> 5);
			r = ((stv_cram[offset] & 0x0000001f) >> 0);
			b*=0x8;
			g*=0x8;
			r*=0x8;
			palette_set_color((offset*2)+1,r,g,b);
			b = ((stv_cram[offset] & 0x7c000000) >> 26);
			g = ((stv_cram[offset] & 0x03e00000) >> 21);
			r = ((stv_cram[offset] & 0x001f0000) >> 16);
			b*=0x8;
			g*=0x8;
			r*=0x8;
			palette_set_color(offset*2,r,g,b);
		break;
	}
}

READ32_HANDLER( stv_scu_r32 )
{
	return stv_scu[offset];
}

WRITE32_HANDLER( stv_scu_w32 )
{
	COMBINE_DATA(&stv_scu[offset]);
}

static MEMORY_READ32_START( stv_readmem )
	{ 0x00000000, 0x0007ffff, MRA32_ROM },   // bios
	{ 0x00100000, 0x0010007f, stv_SMPC_r32 },/*SMPC*/
	{ 0x00180000, 0x0018ffff, MRA32_RAM },	 /*Back up RAM*/

	{ 0x00200000, 0x002fffff, MRA32_RAM },
	{ 0x00400000, 0x0040001f, stv_io_r32 },

	{ 0x02000000, 0x04ffffff, MRA32_BANK1 }, // cartridge
//	{ 0x02200000, 0x04ffffff, read_cart }, // cartridge
	{ 0x05000000, 0x058fffff, MRA32_RAM },

	/* Sound */
	{ 0x05a00000, 0x05b00ee3, MRA32_RAM },

	/* VDP1 */
	/*0x05c00000-0x05c7ffff VRAM*/
	/*0x05c80000-0x05c9ffff Frame Buffer 0*/
	/*0x05ca0000-0x05cbffff Frame Buffer 1*/
	/*0x05d00000-0x05d7ffff VDP1 Regs */
	{ 0x05c00000, 0x05cbffff, MRA32_RAM },
	{ 0x05d00000, 0x05d0001f, MRA32_RAM },

	/* VDP2 when VRAMSZ is 0*/
	{ 0x5e00000 , 0x5e1ffff, a0_vdp2_r },
	{ 0x5e20000 , 0x5e3ffff, a1_vdp2_r },
	{ 0x5e40000 , 0x5e5ffff, b0_vdp2_r },
	{ 0x5e60000 , 0x5e7ffff, b1_vdp2_r },
	/* VDP2 when VRAMSZ is 1*/
	/*0x5e00000-0x5e3ffff A0*/
	/*0x5e40000-0x5e7ffff A1*/
	/*0x5e80000-0x5ecffff B0*/
	/*0x5ed0000-0x5efffff B1*/
//	{ 0x05e00000, 0x05e7ffff, MRA32_RAM },
	{ 0x05f00000, 0x05f0ffff, stv_palette_r }, /* CRAM */
	{ 0x05f80000, 0x05fbffff, stv_vdp2_regs_r32 }, /* REGS */
	{ 0x05fe0000, 0x05fe00cf, stv_scu_r32 },

	{ 0x06000000, 0x060fffff, MRA32_RAM },
//	{ 0x06100000, 0x07ffffff, MRA32_NOP },
MEMORY_END

static MEMORY_WRITE32_START( stv_writemem )
	{ 0x00000000, 0x0007ffff, MWA32_ROM },
	{ 0x00100000, 0x0010007f, stv_SMPC_w32 },

	{ 0x00180000, 0x0018ffff, MWA32_RAM },


	{ 0x00200000, 0x002fffff, MWA32_RAM, &stv_workram_l },
	{ 0x00400000, 0x0040001f, stv_io_w32 ,&ioga },

	{ 0x02000000, 0x04ffffff, MWA32_ROM },
	{ 0x05000000, 0x058fffff, MWA32_RAM },

	/* Sound */
	{ 0x05a00000, 0x05b00ee3, MWA32_RAM },

	/* VDP1 */
	{ 0x05c00000, 0x05cbffff, MWA32_RAM },
	{ 0x05d00000, 0x05d0001f, MWA32_RAM },

	/* VDP2 */
	{ 0x05e00000 ,0x05e1ffff, a0_vdp2_w, &stv_a0_vram },
	{ 0x05e20000 ,0x05e3ffff, a1_vdp2_w, &stv_a1_vram },
	{ 0x05e40000 ,0x05e5ffff, b0_vdp2_w, &stv_b0_vram },
	{ 0x05e60000 ,0x05e7ffff, b1_vdp2_w, &stv_b1_vram },
//	{ 0x05e00000, 0x05e7ffff, stv_vram_w, &stv_vram }, /* VRAM */
	{ 0x05f00000, 0x05f0ffff, stv_palette_w, &stv_cram }, /* CRAM */
	{ 0x05f80000, 0x05fbffff, stv_vdp2_regs_w32 ,&stv_vdp2_regs }, /* REGS */

	{ 0x05fe0000, 0x05fe00cf, stv_scu_w32, &stv_scu },

	{ 0x06000000, 0x060fffff, MWA32_RAM, &stv_workram_h },
//	{ 0x06100000, 0x07ffffff, MWA32_NOP },
MEMORY_END

#define STV_PLAYER_INPUTS(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_##_b1_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_##_b2_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_##_b3_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_##_b4_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER##_n_ )

INPUT_PORTS_START( stv )
	PORT_START
	STV_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	STV_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
/*
	PORT_START
	STV_PLAYER_INPUTS(3, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	STV_PLAYER_INPUTS(4, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
*/
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "xx" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "2" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END



DRIVER_INIT ( stv )
{
	unsigned char *ROM = memory_region(REGION_USER1);
	cpu_setbank(1,&ROM[0x000000]);

	smpc_ram = auto_malloc (0x80);

	nmi_enabled = 0;
}

static MACHINE_INIT( stv )
{
}

static struct GfxLayout tiles8x8x4_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout tiles8x8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8x4_layout,   0x00, 0x100  },
	{ REGION_GFX2, 0, &tiles8x8x4_layout,   0x00, 0x100  },
	{ REGION_GFX3, 0, &tiles8x8x4_layout,   0x00, 0x100  },
	{ REGION_GFX4, 0, &tiles8x8x4_layout,   0x00, 0x100  },
	{ REGION_GFX1, 0, &tiles8x8x8_layout,   0x00, 0x100  },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( stv )

	/* basic machine hardware */
	MDRV_CPU_ADD(SH2, 28000000) // 28MHz
	MDRV_CPU_MEMORY(stv_readmem,stv_writemem)
	MDRV_CPU_VBLANK_INT(stv_interrupt,3)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(stv)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(128*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 352-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(0x10000/4)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(stv)
	MDRV_VIDEO_UPDATE(stv)

MACHINE_DRIVER_END

ROM_START( stvbios )
	ROM_REGION( 0x1000000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) // jp
	ROM_LOAD16_WORD_SWAP( "mp17951a.s",     0x000000, 0x080000, CRC(2672f9d8) SHA1(63cf4a6432f6c87952f9cf3ab0f977aed2367303) ) // jp alt?
	ROM_LOAD16_WORD_SWAP( "mp17952a.s",     0x000000, 0x080000, CRC(d1be2adf) SHA1(eaf1c3e5d602e1139d2090a78d7e19f04f916794) ) // us?
	ROM_LOAD16_WORD_SWAP( "20091.bin",      0x000000, 0x080000, CRC(59ed40f4) SHA1(eff0f54c70bce05ff3a289bf30b1027e1c8cd117) ) // newer?

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

/* the roms marked as bad almost certainly aren't bad, theres some very weird
   mirroring going on, or maybe its meant to transfer the rom data to the region it
   tests from rearranging it a bit (dma?)

   comments merely indicate the status the rom gets in the rom check at the moment

*/

/*

there appears to only be one main cartridge layout, just some having different positions populated if you use the ic named in
the test mode you have the following

some of the rom names were using something else and have been renamed to match test mode, old extension left in comments

( add 0x2000000 for real memory map location )

0x0000000 - 0x01fffff IC13 Header can be read from here .. *IC13 ALWAYS fails on the games if they have one, something weird going on
0x0200000 - 0x03fffff IC7  .. or here (some games have both ic7 and ic13 but the header is in ic13 in these cases)
0x0400000 - 0x07fffff IC2
0x0800000 - 0x0bfffff IC3
0x0c00000 - 0x0ffffff IC4
0x1000000 - 0x13fffff IC5
0x1400000 - 0x17fffff IC6
0x1800000 - 0x1bfffff IC1
0x1c00000 - 0x1ffffff IC8
0x2000000 - 0x23fffff IC9
0x2400000 - 0x27fffff IC10
0x2800000 - 0x2bfffff IC11
0x2c00000 - 0x2ffffff IC12

*/


ROM_START( astrass )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20825.13",                0x0000000, 0x0100000, CRC(94a9ad8f) SHA1(861311c14cfa9f560752aa5b023c147a539cf135) ) // ic13 bad?! (was .24)
	ROM_LOAD16_WORD_SWAP( "mpr20827.2",     0x0400000, 0x0400000, CRC(65cabbb3) SHA1(5e7cb090101dc42207a4084465e419f4311b6baf) ) // good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr20828.3",     0x0800000, 0x0400000, CRC(3934d44a) SHA1(969406b8bfac43b30f4d732702ca8cffeeefffb9) ) // good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr20829.4",     0x0c00000, 0x0400000, CRC(814308c3) SHA1(45c3f551690224c95acd156ae8f8397667927a04) ) // good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr20830.5",     0x1000000, 0x0400000, CRC(ff97fd19) SHA1(f37bcdce5f3f522527a44d59f1b8184ef290f829) ) // good (was .15)
	ROM_LOAD16_WORD_SWAP( "mpr20831.6",     0x1400000, 0x0400000, CRC(4408e6fb) SHA1(d4228cad8a1128e9426dac9ac62e9513a7a0117b) ) // good (was .16)
	ROM_LOAD16_WORD_SWAP( "mpr20826.1",     0x1800000, 0x0400000, CRC(bdc4b941) SHA1(c5e8b1b186324c2ccab617915f7bdbfe6897ca9f) ) // good (was .17)
	ROM_LOAD16_WORD_SWAP( "mpr20832.8",     0x1c00000, 0x0400000, CRC(af1b0985) SHA1(d7a0e4e0a8b0556915f924bdde8c3d14e5b3423e) ) // good (was .18s)
	ROM_LOAD16_WORD_SWAP( "mpr20833.9",     0x2000000, 0x0400000, CRC(cb6af231) SHA1(4a2e5d7c2fd6179c19cdefa84a03f9a34fbb9e70) ) // good (was .19s)

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( bakubaku )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr17969.13",               0x0000000, 0x0100000, CRC(bee327e5) SHA1(1d226db72d6ef68fd294f60659df7f882b25def6) ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr17970.2",    0x0400000, 0x0400000, CRC(bc4d6f91) SHA1(dcc241dcabea59325decfba3fd5e113c07958422) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17971.3",    0x0800000, 0x0400000, CRC(c780a3b3) SHA1(99587eea528a6413cacc3e4d3d1dbfff57b03dca) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17972.4",    0x0c00000, 0x0400000, CRC(8f29815a) SHA1(e86acd8096f2aee5f5e3ddfd3abb4f5c2b11df66) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17973.5",    0x1000000, 0x0400000, CRC(5f6e0e8b) SHA1(eeb5efb5216ab8b8fdee4656774bbd5a2a5b2d42) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( colmns97 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0xc00000, REGION_USER1, 0 ) /* SH2 code */
	/* it tests .13 at 0x000000 - 0x1fffff but reports as bad even if we put the rom there */
	ROM_LOAD( "fpr19553.13",    0x000000, 0x100000, CRC(d4fb6a5e) SHA1(bd3cfb4f451b6c9612e42af5ddcbffa14f057329) ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr19554.2",     0x400000, 0x400000, CRC(5a3ebcac) SHA1(46e3d1cf515a7ff8a8f97e5050b29dbbeb5060c0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19555.3",     0x800000, 0x400000, CRC(74f6e6b8) SHA1(8080860550eb770e04447e344fb337748a249761) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( cotton2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr20122.7",    0x0200000, 0x0200000, CRC(d616f78a) SHA1(8039dcdfdafb8327a19a1da46a67c0b3f7eee53a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20117.2",    0x0400000, 0x0400000, CRC(893656ea) SHA1(11e3160083ba018fbd588f07061a4e55c1efbebb) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20118.3",    0x0800000, 0x0400000, CRC(1b6a1d4c) SHA1(6b234d6b2d24df7f6d400a56698c0af2f78ce0e7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20119.4",    0x0c00000, 0x0400000, CRC(5a76e72b) SHA1(0a058627ddf78a0bcdaba328a58712419f24e33b) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20120.5",    0x1000000, 0x0400000, CRC(7113dd7b) SHA1(f86add67c4e1349a9b9ebcd0145a30b1667df811) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20121.6",    0x1400000, 0x0400000, CRC(8c8fd521) SHA1(c715681330b5ed37a8506ac58ee2143baa721206) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20116.1",    0x1800000, 0x0400000, CRC(d30b0175) SHA1(2da5c3c02d68b8324948a8cdc93946d97fccdd8f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20123.8",    0x1c00000, 0x0400000, CRC(35f1b89f) SHA1(1d6007c380f817def734fc3030d4fe56df4a15be) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( cottonbm )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr21075.7",    0x0200000, 0x0200000, CRC(200b58ba) SHA1(6daad6d70a3a41172e8d9402af775c03e191232d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21070.2",    0x0400000, 0x0400000, CRC(56c0bf1d) SHA1(c2b564ce536c637bb723ed96683b27596e87ebe7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21071.3",    0x0800000, 0x0400000, CRC(2bb18df2) SHA1(e900adb94ad3f48be00a4ce33e915147dc6a8737) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21072.4",    0x0c00000, 0x0400000, CRC(7c7cb977) SHA1(376dfb8014050605b00b6545520bd544768f5828) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21073.5",    0x1000000, 0x0400000, CRC(f2e5a5b7) SHA1(9258d508ef6f6529efc4ad172fd29e69877a99eb) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21074.6",    0x1400000, 0x0400000, CRC(6a7e7a7b) SHA1(a0b1e7a85e623b59886b28797281df1d65b8a5aa) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21069.1",    0x1800000, 0x0400000, CRC(6a28e3c5) SHA1(60454b71db49b872e0cb89fae2259fed601588bd) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( decathlt )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18967.13",               0x0000000, 0x0100000, CRC(c0446674) SHA1(4917089d95613c9d2a936ed9fe3ebd22f461aa4f) ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr18968.2",    0x0400000, 0x0400000, CRC(11a891de) SHA1(1a4fa8d7e07e1d8fdc8122ef8a5b93723c007cda) ) // good (was .1)
	ROM_LOAD16_WORD_SWAP( "mpr18969.3",    0x0800000, 0x0400000, CRC(199cc47d) SHA1(d78f7c6be7e9b43e208244c5c8722245f4c653e1) ) // good (was .2)
	ROM_LOAD16_WORD_SWAP( "mpr18970.4",    0x0c00000, 0x0400000, CRC(8b7a509e) SHA1(8f4d36a858231764ed09b26a1141d1f055eee092) ) // good (was .3)
	ROM_LOAD16_WORD_SWAP( "mpr18971.5",    0x1000000, 0x0400000, CRC(c87c443b) SHA1(f2fedb35c80e5c4855c7aebff88186397f4d51bc) ) // good (was .4)
	ROM_LOAD16_WORD_SWAP( "mpr18972.6",    0x1400000, 0x0400000, CRC(45c64fca) SHA1(ae2f678b9885426ce99b615b7f62a451f9ef83f9) ) // good (was .5)

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( diehard )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mp17952a.s",     0x000000, 0x080000, CRC(d1be2adf) SHA1(eaf1c3e5d602e1139d2090a78d7e19f04f916794) ) /* requires us bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr19119.13",               0x0000000, 0x0100000, CRC(de5c4f7c) SHA1(35f670a15e9c86edbe2fe718470f5a75b5b096ac) ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr19115.2",    0x0400000, 0x0400000, CRC(6fe06a30) SHA1(dedb90f800bae8fd9df1023eb5bec7fb6c9d0179) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19116.3",    0x0800000, 0x0400000, CRC(af9e627b) SHA1(a53921c3185a93ec95299bf1c29e744e2fa3b8c0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19117.4",    0x0c00000, 0x0400000, CRC(74520ff1) SHA1(16c1acf878664b3bd866c9b94f3695ae892ac12f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19118.5",    0x1000000, 0x0400000, CRC(2c9702f0) SHA1(5c2c66de83f2ccbe97d3b1e8c7e65999e1fa2de1) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( dnmtdeka )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr19114.13",               0x0000000, 0x0100000, CRC(1fd22a5f) SHA1(c3d9653b12354a73a3e15f23a2ab7992ffb83e46) ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr19115.2",    0x0400000, 0x0400000, CRC(6fe06a30) SHA1(dedb90f800bae8fd9df1023eb5bec7fb6c9d0179) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19116.3",    0x0800000, 0x0400000, CRC(af9e627b) SHA1(a53921c3185a93ec95299bf1c29e744e2fa3b8c0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19117.4",    0x0c00000, 0x0400000, CRC(74520ff1) SHA1(16c1acf878664b3bd866c9b94f3695ae892ac12f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19118.5",    0x1000000, 0x0400000, CRC(2c9702f0) SHA1(5c2c66de83f2ccbe97d3b1e8c7e65999e1fa2de1) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( ejihon )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18137.13",               0x0000000, 0x0080000, CRC(151aa9bc) SHA1(0959c60f31634816825acb57413838dcddb17d31) ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr18138.2",    0x0400000, 0x0400000, CRC(f5567049) SHA1(6eb35e4b5fbda39cf7e8c42b6a568bd53a364d6d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18139.3",    0x0800000, 0x0400000, CRC(f36b4878) SHA1(e3f63c0046bd37b7ab02fb3865b8ebcf4cf68e75) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18140.4",    0x0c00000, 0x0400000, CRC(228850a0) SHA1(d83f7fa7df08407fa45a13661393679b88800805) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18141.5",    0x1000000, 0x0400000, CRC(b51eef36) SHA1(2745cba48dc410d6d31327b956886ec284b9eac3) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18142.6",    0x1400000, 0x0400000, CRC(cf259541) SHA1(51e2c8d16506d6074f6511112ec4b6b44bed4886) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( elandore )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr21307.7",    0x0200000, 0x0200000, CRC(966ad472) SHA1(d6db41d1c40d08eb6bce8a8a2f491e7533daf670) ) // good (was .11s)
	ROM_LOAD16_WORD_SWAP( "mpr21301.2",    0x0400000, 0x0400000, CRC(1a23b0a0) SHA1(f9dbc7ba96dadfb00e5827622b557080449acd83) ) // good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr21302.3",    0x0800000, 0x0400000, CRC(1c91ca33) SHA1(ae11209088e3bf8fc4a92dca850d7303ce949b29) ) // good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr21303.4",    0x0c00000, 0x0400000, CRC(07b2350e) SHA1(f32f63fd8bec4e667f61da203d63be9a27798dfe) ) // good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr21304.5",    0x1000000, 0x0400000, CRC(cfea52ae) SHA1(4b6d27e0b2a95300ee9e07ebcdc4953d77c4efbe) ) // good (was .15)
	ROM_LOAD16_WORD_SWAP( "mpr21305.6",    0x1400000, 0x0400000, CRC(46cfc2a2) SHA1(8ca26bf8fa5ced040e815c125c13dd06d599e189) ) // good (was .16)
	ROM_LOAD16_WORD_SWAP( "mpr21306.1",    0x1800000, 0x0400000, CRC(87a5929c) SHA1(b259341d7b0e1fa98959bf52d23db5c308a8efdd) ) // good (was .17)
	ROM_LOAD16_WORD_SWAP( "mpr21308.8",    0x1c00000, 0x0400000, CRC(336ec1a4) SHA1(20d1fce050cf6132d284b91853a4dd5626372ef0) ) // good (was .18s)

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( ffreveng )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "opr21872.7",   0x0200000, 0x0200000, CRC(32d36fee) SHA1(441c4254ef2e9301e1006d69462a850ce339314b) ) // good (was .11s)
	ROM_LOAD16_WORD_SWAP( "mpr21873.2",   0x0400000, 0x0400000, CRC(dac5bd98) SHA1(6102035ce9eb2f83d7d9b20f989a151f45087c67) ) // good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr21874.3",   0x0800000, 0x0400000, CRC(0a7be2f1) SHA1(e2d13f36e54d1e2cb9d584db829c04a6ff65108c) ) // good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr21875.4",   0x0c00000, 0x0400000, CRC(ccb75029) SHA1(9611a08a2ad0e0e82137ded6205440a948a339a4) ) // good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr21876.5",   0x1000000, 0x0400000, CRC(bb92a7fc) SHA1(d9e0fab1104a46adeb0a0cfc0d070d4c63a28d55) ) // good (was .15)
	ROM_LOAD16_WORD_SWAP( "mpr21877.6",   0x1400000, 0x0400000, CRC(c22a4a75) SHA1(3276bc0628e71b432f21ba9a4f5ff7ccc8769cd9) ) // good (was .16)
	ROM_LOAD16_WORD_SWAP( "opr21878.1",   0x1800000, 0x0200000, CRC(2ea4a64d) SHA1(928a973dce5eba0a1628d61ba56a530de990a946) ) // good (was .17)

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

/* set system to 1 player to test rom */
ROM_START( fhboxers )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	/* maybe there is some banking on this one, or the roms are in the wrong places */
	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fr18541a.13",               0x0000000, 0x0100000, CRC(8c61a17c) SHA1(a8aef27b53482923a506f7daa4b7a38653b4d8a4) ) // ic13 bad?! (header is read from here, not ic7 even if both are populated on this board)
	ROM_LOAD16_WORD_SWAP( "mpr18538.7",    0x0200000, 0x0200000, CRC(7b5230c5) SHA1(70cebc3281580b43adf42c37318e12159c28a13d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18533.2",    0x0400000, 0x0400000, CRC(7181fe51) SHA1(646f95e1a5b64d721e961352cee6fd5adfd031ec) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18534.3",    0x0800000, 0x0400000, CRC(c87ef125) SHA1(c9ced130faf6dd9e626074b6519615654d8beb19) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18535.4",    0x0c00000, 0x0400000, CRC(929a64cf) SHA1(206dfc2a46befbcea974df1e27515c5759d88d00) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18536.5",    0x1000000, 0x0400000, CRC(51b9f64e) SHA1(bfbdfb73d24f26ce1cc5294c23a1712fb9631691) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18537.6",    0x1400000, 0x0400000, CRC(c364f6a7) SHA1(4db21bcf6ea3e75f9eb34f067b56a417589271c0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18532.1",    0x1800000, 0x0400000, CRC(39528643) SHA1(e35f4c35c9eb13e1cdcc26cb2599bb846f2c1af7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18539.8",    0x1c00000, 0x0400000, CRC(62b3908c) SHA1(3f00e49beb0e5575cc4250a25c41f04dc91d6ed0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18540.9",    0x2000000, 0x0400000, CRC(4c2b59a4) SHA1(4d15503fcff0e9e0d1ed3bac724278102b506da0) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

/* set system to 1 player to test rom */
ROM_START( findlove )
	ROM_REGION( 0x340000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20424.13",               0x0000000, 0x0100000, CRC(4e61fa46) SHA1(e34624d98cbdf2dd04d997167d3c4decd2f208f7) ) // ic13 bad?! (header is read from here, not ic7 even if both are populated on this board)
	ROM_LOAD16_WORD_SWAP( "mpr20431.7",    0x0200000, 0x0200000, CRC(ea656ced) SHA1(b2d6286081bd46a89d1284a2757b87d0bca1bbde) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20426.2",    0x0400000, 0x0400000, CRC(897d1747) SHA1(f3fb2c4ef8bc2c1658907e822f2ee2b88582afdd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20427.3",    0x0800000, 0x0400000, CRC(a488a694) SHA1(80ec81f32e4b5712a607208b2a45cfdf6d5e1849) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20428.4",    0x0c00000, 0x0400000, CRC(4353b3b6) SHA1(f5e56396b345ff65f57a23f391b77d401f1f58b5) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20429.5",    0x1000000, 0x0400000, CRC(4f566486) SHA1(5b449288e33f02f2362ebbd515c87ea11cc02633) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20430.6",    0x1400000, 0x0400000, CRC(d1e11979) SHA1(14405997eefac22c42f0c86dca9411ba1dee9bf9) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20425.1",    0x1800000, 0x0400000, CRC(67f104c4) SHA1(8e965d2ce554ba8d37254f6bf3931dff4bce1a43) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20432.8",    0x1c00000, 0x0400000, CRC(79fcdecd) SHA1(df8e7733a51e24196914fc66a024515ee1565599) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20433.9",    0x2000000, 0x0400000, CRC(82289f29) SHA1(fb6a1015621b1afa3913da162ae71ded6b674649) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20434.10",   0x2400000, 0x0400000, CRC(85c94afc) SHA1(dfc2f16614bc499747ea87567a21c86e7bddce45) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20435.11",   0x2800000, 0x0400000, CRC(263a2e48) SHA1(27ef4bf577d240e36dcb6e6a09b9c5f24e59ce8c) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20436.12",   0x2c00000, 0x0400000, CRC(e3823f49) SHA1(754d48635bd1d4fb01ff665bfe2a71593d92f688) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( finlarch )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "finlarch.13",               0x0000000, 0x0100000, CRC(4505fa9e) SHA1(96c6399146cf9c8f1d27a8fb6a265f937258004a) ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr18257.2",    0x0400000, 0x0400000, CRC(137fdf55) SHA1(07a02fe531b3707e063498f5bc9749bd1b4cadb3) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18258.3",    0x0800000, 0x0400000, CRC(f519c505) SHA1(5cad39314e46b98c24a71f1c2c10c682ef3bdcf3) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18259.4",    0x0c00000, 0x0400000, CRC(5cabc775) SHA1(84383a4cbe3b1a9dcc6c140cff165425666dc780) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18260.5",    0x1000000, 0x0400000, CRC(f5b92082) SHA1(806ad85a187a23a5cf867f2f3dea7d8150065b8e) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END


ROM_START( gaxeduel )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr17766.13",               0x0000000, 0x0080000, CRC(a83fcd62) SHA1(4ce77ebaa0e93c6553ad8f7fb87cbdc32433402b) ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr17768.2",    0x0400000, 0x0400000, CRC(d6808a7d) SHA1(83a97bbe1160cb45b3bdcbde8adc0d9bae5ded60) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17769.3",    0x0800000, 0x0400000, CRC(3471dd35) SHA1(24febddfe70984cebc0e6948ad718e0e6957fa82) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17770.4",    0x0c00000, 0x0400000, CRC(06978a00) SHA1(a8d1333a9f4322e28b23724937f595805315b136) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17771.5",    0x1000000, 0x0400000, CRC(aea2ea3b) SHA1(2fbe3e10d3f5a3b3099a7ed5b38b93b6e22e19b8) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17772.6",    0x1400000, 0x0400000, CRC(b3dc0e75) SHA1(fbe2790c84466d186ea3e9d41edfcb7afaf54bea) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17767.1",    0x1800000, 0x0400000, CRC(9ba1e7b1) SHA1(f297c3697d2e8ba4476d672267163f91f371b362) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( grdforce )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr20844.7",    0x0200000, 0x0200000, CRC(283e7587) SHA1(477fabc27cfe149ad17757e31f10665dcf8c0860) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20839.2",    0x0400000, 0x0400000, CRC(facd4dd8) SHA1(2582894c98b31ab719f1865d4623dad6736dc877) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20840.3",    0x0800000, 0x0400000, CRC(fe0158e6) SHA1(73460effe69fb8f16dd952271542b7803471a599) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20841.4",    0x0c00000, 0x0400000, CRC(d87ac873) SHA1(35b8fa3862e09dca530e9597f983f5a22919cf08) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20842.5",    0x1000000, 0x0400000, CRC(baebc506) SHA1(f5f59f9263956d0c49c729729cf6db31dc861d3b) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20843.6",    0x1400000, 0x0400000, CRC(263e49cc) SHA1(67979861ca2784b3ce39d87e7994e6e7351b40e5) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( groovef )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr19820.7",    0x0200000, 0x0100000, CRC(e93c4513) SHA1(f9636529224880c49bd2cc5572bd5bf41dbf911a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19815.2",    0x0400000, 0x0400000, CRC(1b9b14e6) SHA1(b1828c520cb108e2927a23273ebd2939dca52304) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19816.3",    0x0800000, 0x0400000, CRC(83f5731c) SHA1(2f645737f945c59a1a2fabf3b21a761be9e8c8a6) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19817.4",    0x0c00000, 0x0400000, CRC(525bd6c7) SHA1(2db2501177fb0b44d0fad2054eddf356c4ea08f2) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19818.5",    0x1000000, 0x0400000, CRC(66723ba8) SHA1(0a8379e46a8f8cab11befeadd9abdf59dba68e27) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19819.6",    0x1400000, 0x0400000, CRC(ee8c55f4) SHA1(f6d86b2c2ab43ec5baefb8ccc25e11af4d82712d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19814.1",    0x1800000, 0x0400000, CRC(8f20e9f7) SHA1(30ff5ad0427208e7265cb996e870c4dc0fbbf7d2) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19821.8",    0x1c00000, 0x0400000, CRC(f69a76e6) SHA1(b7e41f34d8b787bf1b4d587e5d8bddb241c043a8) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19822.9",    0x2000000, 0x0200000, CRC(5e8c4b5f) SHA1(1d146fbe3d0bfa68993135ba94ef18081ab65d31) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( hanagumi )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr20143.7",    0x0200000, 0x0100000, CRC(7bfc38d0) SHA1(66f223e7ff2b5456a6f4185b7ab36f9cd833351a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20138.2",    0x0400000, 0x0400000, CRC(fdcf1046) SHA1(cbb1f03879833c17feffdd6f5a4fbff06e1059a2) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20139.3",    0x0800000, 0x0400000, CRC(7f0140e5) SHA1(f2f7de7620d66a596d552e1af491a0592ebc4e51) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20140.4",    0x0c00000, 0x0400000, CRC(2fa03852) SHA1(798ce008f6fc24a00f85298188c8d0d01933640d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20141.5",    0x1000000, 0x0400000, CRC(45d6d21b) SHA1(fe0f0b2195b74e79b8efb6a7c0b7bedca7194c48) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20142.6",    0x1400000, 0x0400000, CRC(e38561ec) SHA1(c04c400be033bc74a7bb2a60f6ae00853a2220d4) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20137.1",    0x1800000, 0x0400000, CRC(181d2688) SHA1(950059f89eda30d8a5bce145421f507e226b8b3e) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20144.8",    0x1c00000, 0x0400000, CRC(235b43f6) SHA1(e35d9bf15ac805513ab3edeca4f264647a2dc0b0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20145.9",    0x2000000, 0x0400000, CRC(aeaac7a1) SHA1(5c75ecce49a5c53dbb0b07e75f3a76e6db9976d0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20146.10",   0x2400000, 0x0400000, CRC(39bab9a2) SHA1(077132e6a03afd181ee9ca9ca4f7c9cbf418e57e) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20147.11",   0x2800000, 0x0400000, CRC(294ab997) SHA1(aeba269ae7d056f07edecf96bc138231c66c3637) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20148.12",   0x2c00000, 0x0400000, CRC(5337ccb0) SHA1(a998bb116eb10c4044410f065c5ddeb845f9dab5) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( introdon )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18937.13",               0x0000000, 0x0080000, CRC(1f40d766) SHA1(35d9751c1b23cfbf448f2a9e9cf3b121929368ae) ) // ic13 bad
	ROM_LOAD16_WORD_SWAP( "mpr18944.7",    0x0200000, 0x0100000, CRC(f7f75ce5) SHA1(0787ece9f89cc1847889adbf08ba5d3ccbc405de) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18939.2",    0x0400000, 0x0400000, CRC(ef95a6e6) SHA1(3026c52ad542997d5b0e621b389c0e01240cb486) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18940.3",    0x0800000, 0x0400000, CRC(cabab4cd) SHA1(b251609573c4b0ccc933188f32226855b25fd9da) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18941.4",    0x0c00000, 0x0400000, CRC(f4a33a20) SHA1(bf0f33495fb5c9de4ae5036cedda65b3ece217e8) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18942.5",    0x1000000, 0x0400000, CRC(8dd0a446) SHA1(a75e3552b0fb99e0b253c0906f62fabcf204b735) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18943.6",    0x1400000, 0x0400000, CRC(d8702a9e) SHA1(960dd3cb0b9eb1f18b8d0bc0da532b600d583ceb) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18938.1",    0x1800000, 0x0400000, CRC(580ecb83) SHA1(6c59f7da408b53f9fa7aa32c1b53328b5fd6334d) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

/* set system to 1 player to test rom */
ROM_START( kiwames )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18737.13",               0x0000000, 0x0080000, CRC(cfad6c49) SHA1(fc69980a351ed13307706db506c79c774eabeb66) ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr18738.2",    0x0400000, 0x0400000, CRC(4b3c175a) SHA1(b6d2438ae1d3d51950a7ed1eaadf2dae45c4e7b1) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18739.3",    0x0800000, 0x0400000, CRC(eb41fa67) SHA1(d12acebb1df9eafd17aff1841087f5017225e7e7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18740.4",    0x0c00000, 0x0200000, CRC(9ca7962f) SHA1(a09e0db2246b34ca7efa3165afbc5ba292a95398) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( maruchan )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20416.13",               0x0000000, 0x0100000, CRC(8bf0176d) SHA1(5bd468e2ffed042ee84e2ceb8712ff5883a1d824) ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr20417.2",    0x0400000, 0x0400000, CRC(636c2a08) SHA1(47986b71d68f6a1852e4e2b03ca7b6e48e83718b) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20418.3",    0x0800000, 0x0400000, CRC(3f0d9e34) SHA1(2ec81e40ebf689d17b6421820bfb0a1280a8ef25) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20419.4",    0x0c00000, 0x0400000, CRC(ec969815) SHA1(b59782174051f5717b06f43e57dd8a2a6910d95f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20420.5",    0x1000000, 0x0400000, CRC(f2902c88) SHA1(df81e137e8aa4bd37e1d14fce4d593cfd14608f0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20421.6",    0x1400000, 0x0400000, CRC(cd0b477c) SHA1(5169cc47fae465b11bc50f5e8410d84c2b2eee42) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20422.1",    0x1800000, 0x0400000, CRC(66335049) SHA1(59f1968001d1e9fe30990a56309bae18033eee62) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20423.8",    0x1c00000, 0x0400000, CRC(2bd55832) SHA1(1a1a510f30882d4d726b594a6541a12c552fafb4) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20443.9",    0x2000000, 0x0400000, CRC(8ac288f5) SHA1(0c08874e6ab2b07b17438721fb535434a626115f) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

/* set system to 1 player to test rom */
ROM_START( myfairld )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr21000.7",    0x0200000, 0x0200000, CRC(2581c560) SHA1(5fb64f0e09583d50dfea7ad613d45aad30b677a5) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20995.2",    0x0400000, 0x0400000, CRC(1bb73f24) SHA1(8773654810de760c5dffbb561f43e259b074a61b) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20996.3",    0x0800000, 0x0400000, CRC(993c3859) SHA1(93f95e3e080a08961784482607919c1ab3eeb5e5) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20997.4",    0x0c00000, 0x0400000, CRC(f0bf64a4) SHA1(f51431f1a736bbc498fa0baa1f8570f89984d9f9) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20998.5",    0x1000000, 0x0400000, CRC(d3b19786) SHA1(1933e57272cd68cc323922fa93a9af97dcef8450) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20999.6",    0x1400000, 0x0400000, CRC(82e31f25) SHA1(0cf74af14abb6ede21d19bc22041214232751594) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20994.1",    0x1800000, 0x0400000, CRC(a69243a0) SHA1(e5a1b6ec62bdd5b015ed6cf48f5a6aabaf4bd837) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21001.8",    0x1c00000, 0x0400000, CRC(95fbe549) SHA1(8cfb48f353b2849600373d66f293f103bca700df) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( othellos )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr20967.7",    0x0200000, 0x0200000, CRC(efc05b97) SHA1(a533366c3aaba90dcac8f3654db9ad902efca258) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20963.2",    0x0400000, 0x0400000, CRC(2cc4f141) SHA1(8bd1998aff8615b34d119fab3637a08ed6e8e1e4) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20964.3",    0x0800000, 0x0400000, CRC(5f5cda94) SHA1(616be219a2512e80c875eddf05137c23aedf6f65) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20965.4",    0x0c00000, 0x0400000, CRC(37044f3e) SHA1(cbc071554cfd8bb12a337c04b169de6c6309c3ab) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20966.5",    0x1000000, 0x0400000, CRC(b94b83de) SHA1(ba1b3135d0ad057f0786f94c9d06b5e347bedea8) ) // good


	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( pblbeach )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18852.13",               0x0000000, 0x0080000, CRC(d12414ec) SHA1(0f42ec9e41983781b6892622b00398a102072aa7) ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr18853.2",    0x0400000, 0x0400000, CRC(b9268c97) SHA1(8734e3f0e6b2849d173e3acc9d0308084a4e84fd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18854.3",    0x0800000, 0x0400000, CRC(3113c8bc) SHA1(4e4600646ddd1978988d27430ffdf0d1d405b804) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18855.4",    0x0c00000, 0x0400000, CRC(daf6ad0c) SHA1(2a14a6a42e4eb68abb7a427e43062dfde2d13c5c) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18856.5",    0x1000000, 0x0400000, CRC(214cef24) SHA1(f62b462170b377cff16bb6c6126cbba00b013a87) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( prikura )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr19337.7",    0x0200000, 0x0200000, CRC(76f69ff3) SHA1(5af2e1eb3288d70c2a1c71d0b6370125d65c7757) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19333.2",    0x0400000, 0x0400000, CRC(eb57a6a6) SHA1(cdacaa7a2fb1a343195e2ac5fd02eabf27f89ccd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19334.3",    0x0800000, 0x0400000, CRC(c9979981) SHA1(be491a4ac118d5025d6a6f2d9267a6d52f21d2b6) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19335.4",    0x0c00000, 0x0400000, CRC(9e000140) SHA1(9b7dc3dc7f9dc048d2fcbc2b44ae79a631ceb381) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19336.5",    0x1000000, 0x0400000, CRC(2363fa4b) SHA1(f45e53352520be4ea313eeab87bcab83f479d5a8) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( puyosun )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr19531.13",               0x0000000, 0x0080000, CRC(ac81024f) SHA1(b22c7c1798fade7ae992ff83b138dd23e6292d3f) ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr19533.2",    0x0400000, 0x0400000, CRC(17ec54ba) SHA1(d4cdc86926519291cc78980ec513e1cfc677e76e) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19534.3",    0x0800000, 0x0400000, CRC(820e4781) SHA1(7ea5626ad4e1929a5ec28a99ec12bc364df8f70d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19535.4",    0x0c00000, 0x0400000, CRC(94fadfa4) SHA1(a7d0727cf601e00f1ea31e6bf3e591349c3f6030) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19536.5",    0x1000000, 0x0400000, CRC(5765bc9c) SHA1(b217c292e7cc8ed73a39a3ae7009bc9dd031e376) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19537.6",    0x1400000, 0x0400000, CRC(8b736686) SHA1(aec347c0f3e5dd8646e85f68d71ca9acc3bf62c3) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19532.1",    0x1800000, 0x0400000, CRC(985f0c9d) SHA1(de1ad42ef3cf3f4f071e9801696407be7ae29d21) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19538.8",    0x1c00000, 0x0400000, CRC(915a723e) SHA1(96480441a69d6aad3887ed6f46b0a6bebfb752aa) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19539.9",    0x2000000, 0x0400000, CRC(72a297e5) SHA1(679987e62118dd1bf7c074f4b88678e1a1187437) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( rsgun )
	ROM_REGION( 0x1400000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr20958.7",   0x0200000, 0x0200000, CRC(cbe5a449) SHA1(b4744ab71ccbadda1921ba43dd1148e57c0f84c5) ) // good (was .11s)
	ROM_LOAD16_WORD_SWAP( "mpr20959.2",   0x0400000, 0x0400000, CRC(a953330b) SHA1(965274a7297cb88e281fcbdd3ec5025c6463cc7b) ) // good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr20960.3",   0x0800000, 0x0400000, CRC(b5ab9053) SHA1(87c5d077eb1219c35fa65b4e11d5b62e826f5236) ) // good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr20961.4",   0x0c00000, 0x0400000, CRC(0e06295c) SHA1(0ec2842622f3e9dc5689abd58aeddc7e5603b97a) ) // good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr20962.5",   0x1000000, 0x0400000, CRC(f1e6c7fc) SHA1(0ba0972f1bc7c56f4e0589d3e363523cea988bb0) ) // good (was .15)

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( sandor )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "sando-r.13",               0x0000000, 0x0100000, CRC(fe63a239) SHA1(01502d4494f968443581cd2c74f25967d41f775e) ) // ic13 bad
	ROM_LOAD16_WORD_SWAP( "mpr18635.8",   0x1c00000, 0x0400000, CRC(441e1368) SHA1(acb2a7e8d44c2203b8d3c7a7b70e20ffb120bebf) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18636.9",   0x2000000, 0x0400000, CRC(fff1dd80) SHA1(36b8e1526a4370ae33fd4671850faf51c448bca4) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18637.10",  0x2400000, 0x0400000, CRC(83aced0f) SHA1(6cd1702b9c2655dc4f56c666607c333f62b09fc0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18638.11",  0x2800000, 0x0400000, CRC(caab531b) SHA1(a77bdcc27d183896c0ed576eeebcc1785d93669e) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( sassisu )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20542.13",               0x0000000, 0x0100000, CRC(0e632db5) SHA1(9bc52794892eec22d381387d13a0388042e30714) ) // ic13 bad
	ROM_LOAD16_WORD_SWAP( "mpr20544.2",    0x0400000, 0x0400000, CRC(661fff5e) SHA1(41f4ddda7adf004b52cc9a076606a60f31947d19) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20545.3",    0x0800000, 0x0400000, CRC(8e3a37be) SHA1(a3227cdc4f03bb088e7f9aed225b238da3283e01) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20546.4",    0x0c00000, 0x0400000, CRC(72020886) SHA1(e80bdeb11b726eb23f2283950d65d55e31a5672e) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20547.5",    0x1000000, 0x0400000, CRC(8362e397) SHA1(71f13689a60572a04b91417a9a48adfd3bd0f5dc) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20548.6",    0x1400000, 0x0400000, CRC(e37534d9) SHA1(79988cbb1537ca99fdd0288a86564fe1f714d052) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20543.1",    0x1800000, 0x0400000, CRC(1f688cdf) SHA1(a90c1011119adb50e0d9d5cd3d7616a307b2d7e8) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

/* set to 1 player to test */
ROM_START( seabass )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "seabassf.13",               0x0000000, 0x0100000, CRC(6d7c39cc) SHA1(d9d1663134420b75c65ee07d7d547254785f2f83) ) // ic13 bad
	ROM_LOAD16_WORD_SWAP( "mpr20551.2",    0x0400000, 0x0400000, CRC(9a0c6dd8) SHA1(26600372cc673ce3678945f4b5dc4e3ab31643a4) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20552.3",    0x0800000, 0x0400000, CRC(5f46b0aa) SHA1(1aa576b15971c0ffb4e08d4802246841b31b6f35) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20553.4",    0x0c00000, 0x0400000, CRC(c0f8a6b6) SHA1(2038b9231a950450267be0db24b31d8035db79ad) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20554.5",    0x1000000, 0x0400000, CRC(215fc1f9) SHA1(f042145622ba4bbbcce5f050a4c9eae42cb7adcd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20555.6",    0x1400000, 0x0400000, CRC(3f5186a9) SHA1(d613f307ab150a7eae358aa449206af05db5f9d7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20550.1",    0x1800000, 0x0400000, CRC(083e1ca8) SHA1(03944dd8fe86f305ca4bd2d71e2140e03798ffc9) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20556.8",    0x1c00000, 0x0400000, CRC(1fd70c6c) SHA1(d9d2e362d13238216f4f7e10095fb8383bbd91e8) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20557.9",    0x2000000, 0x0400000, CRC(3c9ba442) SHA1(2e5b795cf4cdc11ab3e4887b2f77c7147c6e3eec) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( shanhigw )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x0800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr18341.7",    0x0200000, 0x0200000, CRC(cc5e8646) SHA1(a733616c118140ff3887d30d595533f9a1beae06) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18340.2",    0x0400000, 0x0200000, CRC(8db23212) SHA1(85d604a5c6ab97188716dbcd77d365af12a238fe) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( shienryu )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x0c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr19631.7",    0x0200000, 0x0200000, CRC(3a4b1abc) SHA1(3b14b7fdebd4817da32ea374c15a38c695ffeff1) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19632.2",    0x0400000, 0x0400000, CRC(985fae46) SHA1(f953bde91805b97b60d2ab9270f9d2933e064d95) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19633.3",    0x0800000, 0x0400000, CRC(e2f0b037) SHA1(97861d09e10ce5d2b10bf5559574b3f489e28077) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( sleague )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mp17952a.s",     0x000000, 0x080000, CRC(d1be2adf) SHA1(eaf1c3e5d602e1139d2090a78d7e19f04f916794) ) /* requires us bios */

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18777.13",               0x0000000, 0x0080000, CRC(8d180866) SHA1(d47ebabab6e06400312d39f68cd818852e496b96) ) // ic13 bad
	ROM_LOAD16_WORD_SWAP( "mpr18778.8",    0x1c00000, 0x0400000, CRC(25e1300e) SHA1(64f3843f62cee34a47244ad5ee78fb2aa35289e3) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18779.9",    0x2000000, 0x0400000, CRC(51e2fabd) SHA1(3aa361149af516f16d7d422596ee82014a183c2b) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18780.10",   0x2400000, 0x0400000, CRC(8cd4dd74) SHA1(9ffec1280b3965d52f643894bdfecdd792028191) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18781.11",   0x2800000, 0x0400000, CRC(13ee41ae) SHA1(cdbaeac4c90b5ee84233c299612f7f28280a6ba6) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18782.12",   0x2c00000, 0x0200000, CRC(9be2270a) SHA1(f2de5cd6b269f123305e30bed2b474019e4f05b8) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( sokyugrt )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr19188.13",               0x0000000, 0x0100000, CRC(45a27e32) SHA1(96e1bab8bdadf7071afac2a0a6dd8fd8989f12a6) ) // ic13 bad
	ROM_LOAD16_WORD_SWAP( "mpr19189.2",    0x0400000, 0x0400000, CRC(0b202a3e) SHA1(6691b5af2cacd6092ec03886b78c2565953fa297) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19190.3",    0x0800000, 0x0400000, CRC(1777ded8) SHA1(dd332ac79f0a6d82b6bde35b795b2845003dd1a5) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19191.4",    0x0c00000, 0x0400000, CRC(ec6eb07b) SHA1(01fe4832ece8638ea6f4060099d9105fe8092c88) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19192.5",    0x1000000, 0x0200000, CRC(cb544a1e) SHA1(eb3ba9758487d0e8c4bbfc41453fe35b35cce3bf) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

/* set to 1 player to test */
ROM_START( sss )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr21488.13",               0x0000000, 0x0080000, CRC(71c9def1) SHA1(a544a0b4046307172d2c1bf426ed24845f87d894) ) // ic13 bad (was .24)
	ROM_LOAD16_WORD_SWAP( "mpr21489.2",    0x0400000, 0x0400000, CRC(4c85152b) SHA1(78f2f1c31718d5bf631d8813daf9a11ea2a0e451) ) // ic2 good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr21490.3",    0x0800000, 0x0400000, CRC(03da67f8) SHA1(02f9ba7549ca552291dc0ff1b631103015838bba) ) // ic3 good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr21491.4",    0x0c00000, 0x0400000, CRC(cf7ee784) SHA1(af823df2d60d8ef3d17628b95a04136b807ca095) ) // ic4 good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr21492.5",    0x1000000, 0x0400000, CRC(57753894) SHA1(5c51167c158443d02a53d724a5ceb73055876c06) ) // ic5 good (was .15)
	ROM_LOAD16_WORD_SWAP( "mpr21493.6",    0x1400000, 0x0400000, CRC(efb2d271) SHA1(a591e48206704fbda5fef3ce69ad279da1017ed6) ) // ic6 good (was .16)

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( suikoenb )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr17834.13",               0x0000000, 0x0100000, CRC(746ef686) SHA1(e31c317991a687662a8a2a45aed411001e5f1941) ) // ic13 bad
	ROM_LOAD16_WORD_SWAP( "mpr17836.2",    0x0400000, 0x0400000, CRC(55e9642d) SHA1(5198291cd1dce0398eb47760db2c19eae99273b0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17837.3",    0x0800000, 0x0400000, CRC(13d1e667) SHA1(cd513ceb33cc20032090113b61227638cf3b3998) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17838.4",    0x0c00000, 0x0400000, CRC(f9e70032) SHA1(8efdbcce01bdf77acfdb293545c59bf224a9c7d2) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17839.5",    0x1000000, 0x0400000, CRC(1b2762c5) SHA1(5c7d5fc8a4705249a5b0ea64d51dc3dc95d723f5) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17840.6",    0x1400000, 0x0400000, CRC(0fd4c857) SHA1(42caf22716e834d59e60d45c24f51d95734e63ae) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17835.1",    0x1800000, 0x0400000, CRC(77f5cb43) SHA1(a4f54bc08d73a56caee5b26bea06360568655bd7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17841.8",    0x1c00000, 0x0400000, CRC(f48beffc) SHA1(92f1730a206f4a0abf7fb0ee1210e083a464ad70) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17842.9",    0x2000000, 0x0400000, CRC(ac8deed7) SHA1(370eb2216b8080d3ddadbd32804db63c4ebac76f) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( twcup98 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20819.13",    0x0000000, 0x0100000, CRC(d930dfc8) SHA1(f66cc955181720661a0334fe67fa5750ddf9758b) ) // ic13 bad (was .24)
	ROM_LOAD16_WORD_SWAP( "mpr20821.2",    0x0400000, 0x0400000, CRC(2d930d23) SHA1(5fcaf4257f3639cb3aa407d2936f616499a09d97) ) // ic2 good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr20822.3",    0x0800000, 0x0400000, CRC(8b33a5e2) SHA1(d5689ac8aad63509febe9aa4077351be09b2d8d4) ) // ic3 good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr20823.4",    0x0c00000, 0x0400000, CRC(6e6d4e95) SHA1(c387d03ba27580c62ac0bf780915fdf41552df6f) ) // ic4 good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr20824.5",    0x1000000, 0x0400000, CRC(4cf18a25) SHA1(310961a5f114fea8938a3f514dffd5231e910a5a) ) // ic5 good (was .15)

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( vfkids )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr18914.13",               0x0000000, 0x0100000, CRC(cd35730a) SHA1(645b52b449766beb740ab8f99957f8f431351ceb) ) // ic13 bad
	ROM_LOAD16_WORD_SWAP( "mpr18916.4",    0x0c00000, 0x0400000, CRC(4aae3ddb) SHA1(b75479e73f1bce3f0c27fbd90820fa51eb1914a6) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18917.5",    0x1000000, 0x0400000, CRC(edf6edc3) SHA1(478e958f4f10a8126a00c83feca4a55ad6c25503) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18918.6",    0x1400000, 0x0400000, CRC(d3a95036) SHA1(e300bbbb71fb06027dc539c9bbb12946770ffc95) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18915.1",    0x1800000, 0x0400000, CRC(09cc38e5) SHA1(4dfe0e2f21f746020ec557e62487aa7558cbc1fd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18919.8",    0x1c00000, 0x0400000, CRC(4ac700de) SHA1(b1a8501f1683de380dfa49c9cabbe28bd70a5b26) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18920.9",    0x2000000, 0x0400000, CRC(0106e36c) SHA1(f7c30dc9fedb9da079dd7d52fdecbeb8721c5dee) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18921.10",   0x2400000, 0x0400000, CRC(c23d51ad) SHA1(0169b7e2df84e8caa2b349843bd0673f6de2195f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18922.11",   0x2800000, 0x0400000, CRC(99d0ab90) SHA1(e9c82a826cc76ffbe2423913645cf5d5ba2506d6) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18923.12",   0x2c00000, 0x0400000, CRC(30a41ae9) SHA1(78a3d88b5e6cf669b660460ac967daf408038883) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( vfremix )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x1c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr17944.13",               0x0000000, 0x0100000, CRC(a5bdc560) SHA1(d3830480a611b7d88760c672ce46a2ea74076487) ) // ic13 bad
	ROM_LOAD16_WORD_SWAP( "mpr17946.2",    0x0400000, 0x0400000, CRC(4cb245f7) SHA1(363d9936b27043b5858c956a45736ac05aefc54e) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17947.3",    0x0800000, 0x0400000, CRC(fef4a9fb) SHA1(1b4bd095962db769da17d3644df10f62d041e914) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17948.4",    0x0c00000, 0x0400000, CRC(3e2b251a) SHA1(be6191c18727d7cbc6399fd4c1aaae59304af30c) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17949.5",    0x1000000, 0x0400000, CRC(b2ecea25) SHA1(320c0e7ce34e81e2fe6400cbeb2cb3ca74426cc8) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17950.6",    0x1400000, 0x0400000, CRC(5b1f981d) SHA1(693b5744d210a2ac8b77e7c8c87f07ca859f8aed) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17945.1",    0x1800000, 0x0200000, CRC(03ede188) SHA1(849c7fab5b97e043fea3deb8df6cc195ccced0e0) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

/* set to 1 player to test */
ROM_START( vmahjong )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr19620.7",    0x0200000, 0x0200000, CRC(c98de7e5) SHA1(5346f884793bcb080aa01967e91b54ced4a9802f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19615.2",    0x0400000, 0x0400000, CRC(c62896da) SHA1(52a5b10ca8af31295d2d700349eca038c418b522) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19616.3",    0x0800000, 0x0400000, CRC(f62207c7) SHA1(87e60183365c6f7e62c7a0667f88df0c7f5457fd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19617.4",    0x0c00000, 0x0400000, CRC(ab667e19) SHA1(2608a567888fe052753d0679d9a831d7706dbc86) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19618.5",    0x1000000, 0x0400000, CRC(9782ceee) SHA1(405dd42706416e128b1e2fde225b5343e9330092) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19619.6",    0x1400000, 0x0400000, CRC(0b76866c) SHA1(10add2993dfe9daf757ec2ff8675390081a93c0a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19614.1",    0x1800000, 0x0400000, CRC(b83b3f03) SHA1(e5a5919ee74964633eaaf4af2fe04c38604ccf16) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19621.8",    0x1c00000, 0x0400000, CRC(f92616b3) SHA1(61a9dda92a86a02d027260e11b1bad3b0dda9f02) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( winterht )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr20108.13",    0x0000000, 0x0100000, CRC(1ef9ced0) SHA1(abc90ce341cd17bb77349d611d6879389611f0bf) ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr20110.2",    0x0400000, 0x0400000, CRC(238ef832) SHA1(20fade5730ff8e249a1450c41bfdff6e133f4768) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20111.3",    0x0800000, 0x0400000, CRC(b0a86f69) SHA1(e66427f70413ad43fccc38423962c5eeda01094f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20112.4",    0x0c00000, 0x0400000, CRC(3ba2b49b) SHA1(5ad154a8b774075479d791e29cbaf221d47557fc) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20113.5",    0x1000000, 0x0400000, CRC(8c858b41) SHA1(d05d2980363c8440863fe2fdb39274de246bd4b9) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20114.6",    0x1400000, 0x0400000, CRC(b723862c) SHA1(1e0a08669f16fc4cb647124e0c215233ccb98e5a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20109.1",    0x1800000, 0x0400000, CRC(c1a713b8) SHA1(a7fefa6e9a1e3aecff5ead41da6fd3aec2ef502a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20115.8",    0x1c00000, 0x0400000, CRC(dd01f2ad) SHA1(3bb48dc8670d9460fea2a67400ddb573472c2f4f) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( znpwfv )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* bios */

	ROM_REGION32_BE( 0x2800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20398.13",    0x0000000, 0x0100000, CRC(3fb56a0b) SHA1(13c2fa2d94b106d39e46f71d15fbce3607a5965a) ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr20400.2",    0x0400000, 0x0400000, CRC(1edfbe05) SHA1(b0edd3f3d57408101ae6eb0aec742afbb4d289ca) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20401.3",    0x0800000, 0x0400000, CRC(99e98937) SHA1(e1b4d12a0b4d0fe97a62fcc085e19cce77657c99) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20402.4",    0x0c00000, 0x0400000, CRC(4572aa60) SHA1(8b2d76ea8c6e2f472c6ee7c9b6ad6e80e6a1a85a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20403.5",    0x1000000, 0x0400000, CRC(26a8e13e) SHA1(07f5564b704598e3c3580d3d620ecc4f14549dbd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20404.6",    0x1400000, 0x0400000, CRC(0b70275d) SHA1(47b8672e19c698dc948760f7091f4c6280e728d0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20399.1",    0x1800000, 0x0400000, CRC(c178a96e) SHA1(65f4aa05187d48ba8ad4fe75ff6ffe1f8524831d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20405.8",    0x1c00000, 0x0400000, CRC(f53337b7) SHA1(09a21f81016ee54f10554ae1f790415d7436afe0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20406.9",    0x2000000, 0x0400000, CRC(b677c175) SHA1(d0de7b5a29928036df0bdfced5a8021c0999eb26) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20407.10",   0x2400000, 0x0400000, CRC(58356050) SHA1(f8fb5a14f4ec516093c785891b05d55ae345754e) ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( danchih )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mp17951a.s",    0x0000000, 0x0080000, CRC(2672f9d8) SHA1(63cf4a6432f6c87952f9cf3ab0f977aed2367303) )/* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr21974.7",    0x0200000, 0x0200000, CRC(e7472793) )// good
	ROM_LOAD16_WORD_SWAP( "mpr21970.2",    0x0400000, 0x0400000, CRC(34dd7f4d) )// good
	ROM_LOAD16_WORD_SWAP( "mpr21971.3",    0x0800000, 0x0400000, CRC(8995158c) )// good
	ROM_LOAD16_WORD_SWAP( "mpr21972.4",    0x0c00000, 0x0400000, CRC(68a39090) )// good
	ROM_LOAD16_WORD_SWAP( "mpr21973.5",    0x1000000, 0x0400000, CRC(b0f23f14) )// good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

ROM_START( mausuke )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mp17951a.s",    0x0000000, 0x0080000, CRC(2672f9d8) SHA1(63cf4a6432f6c87952f9cf3ab0f977aed2367303) )/* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD(             "ic13.bin",      0x0000000, 0x0100000, CRC(b456f4cd) )
	ROM_LOAD16_WORD_SWAP( "mcj-00.2",      0x0400000, 0x0200000, CRC(4eeacd6f) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-01.3",      0x0800000, 0x0200000, CRC(365a494b) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-02.4",      0x0c00000, 0x0200000, CRC(8b8e4931) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-03.5",      0x1000000, 0x0200000, CRC(9015a0e7) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-04.6",      0x1400000, 0x0200000, CRC(9d1beaee) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-05.1",      0x1800000, 0x0200000, CRC(a7626a82) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-06.8",      0x1c00000, 0x0200000, CRC(1ab8e90e) )// good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */
ROM_END

/* acclaim game, not a standard cart ... */
ROM_START( batmanfr )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mp17952a.s",     0x000000, 0x080000, CRC(d1be2adf) SHA1(eaf1c3e5d602e1139d2090a78d7e19f04f916794) )

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	/* the batman forever rom test is more useless than most, i'm not really sure how
	   the roms should map, it doesn't even appear to test enough roms nor the right sizes!
	   everything fails for now
       range      tested as
	   040 - 04f  ic2
	   0c0 - 0ff  ic4
	   180 - 1bf  ic1
	   1c0 - 1ff  ic8
	   200 - 20f  ic9
	   2c0 - 2df  ic12
	   000 - ?    ic13

	   */
	ROM_LOAD16_BYTE( "350-mpa1.u19",    0x0000000, 0x0100000, CRC(2a5a8c3a) )
	ROM_LOAD16_BYTE( "350-mpa1.u16",    0x0000001, 0x0100000, CRC(735e23ab) )
	ROM_LOAD16_WORD_SWAP( "gfx0.u1",    0x0400000, 0x0400000, CRC(a82d0b7e) )
	ROM_LOAD16_WORD_SWAP( "gfx1.u3",    0x0c00000, 0x0400000, CRC(a41e55d9) )
	ROM_LOAD16_WORD_SWAP( "gfx2.u5",    0x1800000, 0x0400000, CRC(4c1ebeb7) )
	ROM_LOAD16_WORD_SWAP( "gfx3.u8",    0x1c00000, 0x0400000, CRC(f679a3e7) )
	ROM_LOAD16_WORD_SWAP( "gfx4.u12",   0x0800000, 0x0400000, CRC(52d95242) )
	ROM_LOAD16_WORD_SWAP( "gfx5.u15",   0x2000000, 0x0400000, CRC(e201f830) )
	ROM_LOAD16_WORD_SWAP( "gfx6.u18",   0x2c00000, 0x0400000, CRC(c6b381a3) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX B1 */
	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* GFX B0 */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* GFX A1 */
	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* GFX A0 */

	/* it also has an extra adsp sound board, i guess this isn't tested */
	ROM_REGION( 0x080000, REGION_USER2, 0 ) /* ADSP code */
	ROM_LOAD( "350snda1.u52",   0x000000, 0x080000, CRC(9027e7a0) )

	ROM_REGION( 0x800000, REGION_USER3, 0 ) /* Sound */
	ROM_LOAD( "snd0.u48",   0x000000, 0x200000, CRC(02b1927c) )
	ROM_LOAD( "snd1.u49",   0x200000, 0x200000, CRC(58b18eda) )
	ROM_LOAD( "snd2.u50",   0x400000, 0x200000, CRC(51d626d6) )
	ROM_LOAD( "snd3.u51",   0x600000, 0x200000, CRC(31af26ae) )
ROM_END

GAMEX( 1996, stvbios,  0,        stv, stv,  stv,  ROT0, "Sega",    "ST-V Bios", NOT_A_DRIVER )

GAMEX( 1998, astrass,   stvbios, stv, stv,  stv,  ROT0, "Sunsoft", 	"Astra SuperStars", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, bakubaku,  stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Baku Baku Animal", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, colmns97,  stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Columns 97", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, cotton2,   stvbios, stv, stv,  stv,  ROT0, "Success", 	"Cotton 2", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, cottonbm,  stvbios, stv, stv,  stv,  ROT0, "Success", 	"Cotton Boomerang", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, decathlt,  stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Decathlete", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, diehard,   stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Die Hard Arcade (US)", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, dnmtdeka,  diehard, stv, stv,  stv,  ROT0, "Sega", 	"Dynamite Deka (Japan)", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, ejihon,    stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Ejihon Tantei Jimusyo", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, elandore,  stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Fighting Dragon Legend Elan Doree", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1999, ffreveng,  stvbios, stv, stv,  stv,  ROT0, "Capcom", 	"Final Fight Revenge", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, fhboxers,  stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Funky Head Boxers", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, findlove,  stvbios, stv, stv,  stv,  ROT0, "Daiki",	"Find Love", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, finlarch,  stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Final Arch (Japan)", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1994, gaxeduel,  stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Golden Axe - The Duel", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, grdforce,  stvbios, stv, stv,  stv,  ROT0, "Success", 	"Guardian Force", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, groovef,   stvbios, stv, stv,  stv,  ROT0, "Atlus", 	"Power Instinct 3 - Groove On Fight", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, hanagumi,  stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Hanagumi Taisen Columns - Sakura Wars", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, introdon,  stvbios, stv, stv,  stv,  ROT0, "Sunsoft / Success", "Karaoke Quiz Intro Don Don!", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, kiwames,   stvbios, stv, stv,  stv,  ROT0, "Athena", 	"Pro Mahjong Kiwame S", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, maruchan,  stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Maru-Chan de Goo!", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, myfairld,  stvbios, stv, stv,  stv,  ROT0, "Micronet", "Virtual Mahjong 2 - My Fair Lady", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, othellos,  stvbios, stv, stv,  stv,  ROT0, "Tsukuda Original",	"Othello Shiyouyo", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, pblbeach,  stvbios, stv, stv,  stv,  ROT0, "T&E Soft", "Pebble Beach - The Great Shot", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, prikura,   stvbios, stv, stv,  stv,  ROT0, "Atlus", 	"Purikura Daisakusen", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, puyosun,   stvbios, stv, stv,  stv,  ROT0, "Compile",	"Puyo Puyo Sun", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, rsgun,     stvbios, stv, stv,  stv,  ROT0, "Treasure", "Radiant Silvergun", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, sandor,    stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Sando-R", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, sassisu,   stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Taisen Tanto-R 'Sasshissu!'", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, seabass,   stvbios, stv, stv,  stv,  ROT0, "A Wave inc. (Able license)", "Sea Bass Fishing", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, shanhigw,  stvbios, stv, stv,  stv,  ROT0, "Sunsoft / Activision", "Shanghai - The Great Wall", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, shienryu,  stvbios, stv, stv,  stv,  ROT0, "Warashi", 	"Shienryu", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, sleague,   stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Super Major League (US)", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, sokyugrt,  stvbios, stv, stv,  stv,  ROT0, "Raizing", 	"Soukyugurentai / Terra Diver", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, sss,       stvbios, stv, stv,  stv,  ROT0, "Victor / Cave / Capcom", "Steep Slope Sliders", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, suikoenb,  stvbios, stv, stv,  stv,  ROT0, "Data East","Suikoenbu", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, twcup98,   stvbios, stv, stv,  stv,  ROT0, "Tecmo", 	"Tecmo World Cup '98", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, vfkids,    stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Virtua Fighter Kids", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, vfremix,   stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Virtua Fighter Remix", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, vmahjong,  stvbios, stv, stv,  stv,  ROT0, "Micronet", "Virtual Mahjong", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, winterht,  stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Winter Heat", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, znpwfv,    stvbios, stv, stv,  stv,  ROT0, "Sega", 	"Zen Nippon Pro-Wrestling Featuring Virtua", GAME_NO_SOUND | GAME_NOT_WORKING )

GAMEX( 1999, danchih,   stvbios, stv, stv,  stv,  ROT0, "Altron (distributed by Tecmo)", "Danchi de Hanafuda", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, mausuke,   stvbios, stv, stv,  stv,  ROT0, "Data East","Mausuke no Ojama the World", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, batmanfr,  stvbios, stv, stv,  stv,  ROT0, "Acclaim", 	"Batman Forever", GAME_NO_SOUND | GAME_NOT_WORKING )

/* there are probably a bunch of other games (some fishing games with cd-rom,Print Club 2 etc.) */
