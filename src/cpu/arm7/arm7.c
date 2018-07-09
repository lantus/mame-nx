/*****************************************************************************
 *
 *   arm7.c
 *   Portable ARM7TDMI CPU Emulator
 *
 *   Copyright (c) 2004 Steve Ellenoff, all rights reserved.
 *
 *   - This source code is released as freeware for non-commercial purposes.
 *   - You are free to use and redistribute this code in modified or
 *     unmodified form, provided you list me in the credits.
 *   - If you modify this source code, you must add a notice to each modified
 *     source file that it has been changed.  If you're a nice person, you
 *     will clearly mark each change too.  :)
 *   - If you wish to use this for commercial purposes, please contact me at
 *     sellenoff@hotmail.com
 *   - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *  This work is based on:
 *  #1) 'Atmel Corporation ARM7TDMI (Thumb) Datasheet - January 1999'
 *  #2) Arm 2/3/6 emulator By Bryan McPhail (bmcphail@tendril.co.uk) and Phil Stroffolino (MAME CORE 0.76)
 *
 *****************************************************************************/

/******************************************************************************
 *  Notes:

    ** This is a plain vanilla implementation of an ARM7 cpu which incorporates my ARM7 core.
       It can be used as is, or used to demonstrate how to utilize the arm7 core to create a cpu
       that uses the core, since there are numerous different mcu packages that incorporate an arm7 core.

       See the notes in the arm7core.c file itself regarding issues/limitations of the arm7 core.
    **
*****************************************************************************/
#include <stdio.h>
#include "arm7.h"
#include "state.h"
#include "mamedbg.h"
#include "arm7core.h"   //include arm7 core

/* Example for showing how Co-Proc functions work */
#define TEST_COPROC_FUNCS 1

/*prototypes*/
#if TEST_COPROC_FUNCS
static WRITE32_HANDLER(test_do_callback);
static READ32_HANDLER(test_rt_r_callback);
static WRITE32_HANDLER(test_rt_w_callback);
static void test_dt_r_callback (data32_t insn, data32_t* prn, data32_t (*read32)(int addr));
static void test_dt_w_callback (data32_t insn, data32_t* prn, void (*write32)(int addr, data32_t data));
#ifdef MAME_DEBUG
char *Spec_RT( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0);
char *Spec_DT( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0);
char *Spec_DO( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0);
#endif
#endif

/* Macros that can be re-defined for custom cpu implementations - The core expects these to be defined */
/* In this case, we are using the default arm7 handlers (supplied by the core)
   - but simply changes these and define your own if needed for cpu implementation specific needs */
#define READ8(addr)         arm7_cpu_read8(addr)
#define WRITE8(addr,data)   arm7_cpu_write8(addr,data)
#define READ16(addr)        arm7_cpu_read16(addr)
#define WRITE16(addr,data)  arm7_cpu_write16(addr,data)
#define READ32(addr)        arm7_cpu_read32(addr)
#define WRITE32(addr,data)  arm7_cpu_write32(addr,data)
#define PTR_READ32          &arm7_cpu_read32
#define PTR_WRITE32         &arm7_cpu_write32

/* Macros that need to be defined according to the cpu implementation specific need */
#define ARM7REG(reg)        arm7.sArmRegister[reg]
#define ARM7                arm7
#define ARM7_ICOUNT         arm7_icount

/* CPU Registers */
typedef struct
{
    ARM7CORE_REGS               //these must be included in your cpu specific register implementation
} ARM7_REGS;

static ARM7_REGS arm7;
static int ARM7_ICOUNT;

/* include the arm7 core */
#include "arm7core.c"

/***************************************************************************
 * CPU SPECIFIC IMPLEMENTATIONS
 **************************************************************************/
static void arm7_init(void)
{
    //must call core
    arm7_core_init("arm7");

#if TEST_COPROC_FUNCS
    //setup co-proc callbacks example
    arm7_coproc_do_callback = test_do_callback;
    arm7_coproc_rt_r_callback = test_rt_r_callback;
    arm7_coproc_rt_w_callback = test_rt_w_callback;
    arm7_coproc_dt_r_callback = test_dt_r_callback;
    arm7_coproc_dt_w_callback = test_dt_w_callback;
#ifdef MAME_DEBUG
    //setup dasm callbacks - direct method example
    arm7_dasm_cop_dt_callback = Spec_DT;
    arm7_dasm_cop_rt_callback = Spec_RT;
    arm7_dasm_cop_do_callback = Spec_DO;
#endif
#endif

    return;
}

static void arm7_reset(void *param)
{
    //must call core reset
    arm7_core_reset(param);
}

static void arm7_exit(void)
{
    /* nothing to do here */
}

static int arm7_execute( int cycles )
{
/*include the arm7 core execute code*/
#include "arm7exec.c"
} /* arm7_execute */


static void set_irq_line(int irqline, int state)
{
    //must call core
    arm7_core_set_irq_line(irqline,state);
}

static void arm7_get_context(void *dst)
{
    if( dst )
    {
        memcpy( dst, &ARM7, sizeof(ARM7) );
    }
}

static void arm7_set_context(void *src)
{
    if (src)
    {
        memcpy( &ARM7, src, sizeof(ARM7) );
    }
}

static offs_t arm7_dasm(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
    arm7_disasm( buffer, pc, READ32(pc));
    return 4;
#else
    sprintf(buffer, "$%08x", READ32(pc));
    return 4;
#endif
}

static data8_t arm7_reg_layout[] =
{
    -1,
    ARM7_R0,  ARM7_IR13, -1,
    ARM7_R1,  ARM7_IR14, -1,
    ARM7_R2,  ARM7_ISPSR, -1,
    ARM7_R3,  -1,
    ARM7_R4,  ARM7_FR8,  -1,
    ARM7_R5,  ARM7_FR9,  -1,
    ARM7_R6,  ARM7_FR10, -1,
    ARM7_R7,  ARM7_FR11, -1,
    ARM7_R8,  ARM7_FR12, -1,
    ARM7_R9,  ARM7_FR13, -1,
    ARM7_R10, ARM7_FR14, -1,
    ARM7_R11, ARM7_FSPSR, -1,
    ARM7_R12, -1,
    ARM7_R13, ARM7_AR13, -1,
    ARM7_R14, ARM7_AR14, -1,
    ARM7_R15, ARM7_ASPSR, -1,
    -1,
    ARM7_SR13, ARM7_UR13, -1,
    ARM7_SR14, ARM7_UR14, -1,
    ARM7_SSPSR, ARM7_USPSR, 0
};


static UINT8 arm7_win_layout[] = {
     0, 0,30,17,    /* register window (top rows) */
    31, 0,49,17,    /* disassembler window (left colums) */
     0,18,48, 4,    /* memory #1 window (right, upper middle) */
    49,18,31, 4,    /* memory #2 window (right, lower middle) */
     0,23,80, 1,    /* command line window (bottom rows) */
};


/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void arm7_info(void *context, int regnum)
{
   
}

 

/* TEST COPROC CALLBACK HANDLERS - Used for example on how to implement only */
#if TEST_COPROC_FUNCS

static WRITE32_HANDLER(test_do_callback)
{
    LOG(("test_do_callback opcode=%x, =%x\n",offset,data));
}
static READ32_HANDLER(test_rt_r_callback)
{
    data32_t data=0;
    LOG(("test_rt_r_callback opcode=%x\n",offset));
    return data;
}
static WRITE32_HANDLER(test_rt_w_callback)
{
    LOG(("test_rt_w_callback opcode=%x, data from ARM7 register=%x\n",offset,data));
}
static void test_dt_r_callback (data32_t insn, data32_t* prn, data32_t (*read32)(int addr))
{
    LOG(("test_dt_r_callback: insn = %x\n",insn));
}
static void test_dt_w_callback (data32_t insn, data32_t* prn, void (*write32)(int addr, data32_t data))
{
    LOG(("test_dt_w_callback: opcode = %x\n",insn));
}

/* Custom Co-proc DASM handlers */
#ifdef MAME_DEBUG
char *Spec_RT( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0)
{
    pBuf += sprintf( pBuf, "SPECRT");
    return pBuf;
}
char *Spec_DT( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0)
{
    pBuf += sprintf( pBuf, "SPECDT");
    return pBuf;
}
char *Spec_DO( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0)
{
    pBuf += sprintf( pBuf, "SPECDO");
    return pBuf;
}
#endif
#endif

