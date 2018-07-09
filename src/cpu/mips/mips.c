/*
 * MIPS emulator for the MAME project written by smf
 *
 * The only type of processor currently emulated is the one
 * from the PSX. This is a custom r3000a by LSI Logic with a
 * geometry transform engine, no mmu & disabled data cache
 * ( it's still there and memory mapped as the scratchpad ).
 *
 * This is in a constant state of flux.
 *
 */

#include <stdio.h>
#include "cpuintrf.h"
#include "memory.h"
#include "mamedbg.h"
#include "mips.h"

#define EXC_INT ( 0 )
#define EXC_ADEL ( 4 )
#define EXC_ADES ( 5 )
#define EXC_SYS ( 8 )
#define EXC_BP ( 9 )
#define EXC_RI ( 10 )
#define EXC_CPU ( 11 )
#define EXC_OVF ( 12 )

#define CP0_RANDOM ( 1 )
#define CP0_BADVADDR ( 8 )
#define CP0_SR ( 12 )
#define CP0_CAUSE ( 13 )
#define CP0_EPC ( 14 )
#define CP0_PRID ( 15 )

#define SR_IEC ( 1L << 0 )
#define SR_KUC ( 1L << 1 )
#define SR_ISC ( 1L << 16 )
#define SR_SWC ( 1L << 17 )
#define SR_TS  ( 1L << 21 )
#define SR_BEV ( 1L << 22 )
#define SR_RE ( 1L << 25 )
#define SR_CU0 ( 1L << 28 )
#define SR_CU1 ( 1L << 29 )
#define SR_CU2 ( 1L << 30 )
#define SR_CU3 ( 1L << 31 )

#define CAUSE_EXC ( 31L << 2 )
#define CAUSE_IP ( 255L << 8 )
#define CAUSE_IP2 ( 1L << 10 )
#define CAUSE_IP3 ( 1L << 11 )
#define CAUSE_IP4 ( 1L << 12 )
#define CAUSE_IP5 ( 1L << 13 )
#define CAUSE_IP6 ( 1L << 14 )
#define CAUSE_IP7 ( 1L << 15 )
#define CAUSE_CE ( 3L << 28 )
#define CAUSE_CE0 ( 0L << 28 )
#define CAUSE_CE1 ( 1L << 28 )
#define CAUSE_CE2 ( 2L << 28 )
#define CAUSE_BD ( 1L << 31 )

static UINT8 mips_reg_layout[] =
{
	MIPS_PC, -1,
	MIPS_DELAYPC, MIPS_DELAY, -1,
	MIPS_HI, MIPS_LO, -1,
	-1,
	MIPS_R0, MIPS_R1, -1,
	MIPS_R2, MIPS_R3, -1,
	MIPS_R4, MIPS_R5, -1,
	MIPS_R6, MIPS_R7, -1,
	MIPS_R8, MIPS_R9, -1,
	MIPS_R10, MIPS_R11, -1,
	MIPS_R12, MIPS_R13, -1,
	MIPS_R14, MIPS_R15, -1,
	MIPS_R16, MIPS_R17, -1,
	MIPS_R18, MIPS_R19, -1,
	MIPS_R20, MIPS_R21, -1,
	MIPS_R22, MIPS_R23, -1,
	MIPS_R24, MIPS_R25, -1,
	MIPS_R26, MIPS_R27, -1,
	MIPS_R28, MIPS_R29, -1,
	MIPS_R30, MIPS_R31, -1,
	-1,
	MIPS_CP0R0, MIPS_CP0R1, -1,
	MIPS_CP0R2, MIPS_CP0R3, -1,
	MIPS_CP0R4, MIPS_CP0R5, -1,
	MIPS_CP0R6, MIPS_CP0R7, -1,
	MIPS_CP0R8, MIPS_CP0R9, -1,
	MIPS_CP0R10, MIPS_CP0R11, -1,
	MIPS_CP0R12, MIPS_CP0R13, -1,
	MIPS_CP0R14, MIPS_CP0R15, -1,
	MIPS_CP0R16, MIPS_CP0R17, -1,
	MIPS_CP0R18, MIPS_CP0R19, -1,
	MIPS_CP0R20, MIPS_CP0R21, -1,
	MIPS_CP0R22, MIPS_CP0R23, -1,
	MIPS_CP0R24, MIPS_CP0R25, -1,
	MIPS_CP0R26, MIPS_CP0R27, -1,
	MIPS_CP0R28, MIPS_CP0R29, -1,
	MIPS_CP0R30, MIPS_CP0R31, -1,
	-1,
	MIPS_CP2DR0, MIPS_CP2DR1, -1,
	MIPS_CP2DR2, MIPS_CP2DR3, -1,
	MIPS_CP2DR4, MIPS_CP2DR5, -1,
	MIPS_CP2DR6, MIPS_CP2DR7, -1,
	MIPS_CP2DR8, MIPS_CP2DR9, -1,
	MIPS_CP2DR10, MIPS_CP2DR11, -1,
	MIPS_CP2DR12, MIPS_CP2DR13, -1,
	MIPS_CP2DR14, MIPS_CP2DR15, -1,
	MIPS_CP2DR16, MIPS_CP2DR17, -1,
	MIPS_CP2DR18, MIPS_CP2DR19, -1,
	MIPS_CP2DR20, MIPS_CP2DR21, -1,
	MIPS_CP2DR22, MIPS_CP2DR23, -1,
	MIPS_CP2DR24, MIPS_CP2DR25, -1,
	MIPS_CP2DR26, MIPS_CP2DR27, -1,
	MIPS_CP2DR28, MIPS_CP2DR29, -1,
	MIPS_CP2DR30, MIPS_CP2DR31, -1,
	-1,
	MIPS_CP2CR0, MIPS_CP2CR1, -1,
	MIPS_CP2CR2, MIPS_CP2CR3, -1,
	MIPS_CP2CR4, MIPS_CP2CR5, -1,
	MIPS_CP2CR6, MIPS_CP2CR7, -1,
	MIPS_CP2CR8, MIPS_CP2CR9, -1,
	MIPS_CP2CR10, MIPS_CP2CR11, -1,
	MIPS_CP2CR12, MIPS_CP2CR13, -1,
	MIPS_CP2CR14, MIPS_CP2CR15, -1,
	MIPS_CP2CR16, MIPS_CP2CR17, -1,
	MIPS_CP2CR18, MIPS_CP2CR19, -1,
	MIPS_CP2CR20, MIPS_CP2CR21, -1,
	MIPS_CP2CR22, MIPS_CP2CR23, -1,
	MIPS_CP2CR24, MIPS_CP2CR25, -1,
#if 0
/* the debugger interface is limited to 127 & the kludge no longer works */
	MIPS_CP2CR26, MIPS_CP2CR27, -1,
	MIPS_CP2CR28, MIPS_CP2CR29, -1,
	MIPS_CP2CR30, MIPS_CP2CR31,
#endif
	0
};

static UINT8 mips_win_layout[] = {
	45, 0,35,13,	/* register window (top right) */
	 0, 0,44,13,	/* disassembler window (left, upper) */
	 0,14,44, 8,	/* memory #1 window (left, middle) */
	45,14,35, 8,	/* memory #2 window (lower) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};

typedef struct
{
	UINT32 op;
	UINT32 pc;
	UINT32 delaypc;
	UINT32 delay;
	UINT32 hi;
	UINT32 lo;
	UINT32 r[ 32 ];
	UINT32 cp0r[ 32 ];
	PAIR cp2cr[ 32 ];
	PAIR cp2dr[ 32 ];
	int (*irq_callback)(int irqline);
} mips_cpu_context;

mips_cpu_context mipscpu;

int mips_ICount = 0;

static UINT32 mips_mtc0_writemask[]=
{
	0xffffffff, /* INDEX */
	0x00000000, /* RANDOM */
	0xffffff00, /* ENTRYLO */
	0x00000000,
	0xffe00000, /* CONTEXT */
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000, /* BADVADDR */
	0x00000000,
	0xffffffc0, /* ENTRYHI */
	0x00000000,
	0xf27fff3f, /* SR */
	0x00000300, /* CAUSE */
	0x00000000, /* EPC */
	0x00000000, /* PRID */
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000
};

#if 0
#include "usrintrf.h"
#define GTELOG( a ) logerror( "%08x: GTE: %08x " a "\n", mipscpu.pc, INS_COFUN( mipscpu.op ) ); /* usrintf_showmessage_secs( 1, "GTE" ); */
#define GTELOGX( a, ... ) logerror( "%08x: GTE: %08x " a "\n", mipscpu.pc, INS_COFUN( mipscpu.op ), ## __VA_ARGS__ ); /* usrintf_showmessage_secs( 1, "GTE" ); */
#else
#define GTELOG( a )
#define GTELOGX( a, ... )
#endif

static UINT32 getcp2dr( int n_reg );
static void setcp2dr( int n_reg, UINT32 n_value );
static UINT32 getcp2cr( int n_reg );
static void setcp2cr( int n_reg, UINT32 n_value );
static void docop2( int gteop );
static void mips_exception( int exception );

void mips_stop( void )
{
#ifdef MAME_DEBUG
	extern int debug_key_pressed;
	debug_key_pressed = 1;
	CALL_MAME_DEBUG;
#endif
}

INLINE void mips_set_cp0r( int reg, UINT32 value )
{
	mipscpu.cp0r[ reg ] = value;
	if( reg == CP0_SR || reg == CP0_CAUSE )
	{
		if( ( mipscpu.cp0r[ CP0_SR ] & SR_IEC ) != 0 && ( mipscpu.cp0r[ CP0_SR ] & mipscpu.cp0r[ CP0_CAUSE ] & CAUSE_IP ) != 0 )
		{
			mips_exception( EXC_INT );
		}
		else if( !mipscpu.delay && ( mipscpu.pc & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 3 ) ) != 0 )
		{
			mips_exception( EXC_ADEL );
			mips_set_cp0r( CP0_BADVADDR, mipscpu.pc );
		}
	}
}

INLINE void mips_delayed_branch( UINT32 adr )
{
	if( ( adr & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 3 ) ) != 0 )
	{
		mips_exception( EXC_ADEL );
		mips_set_cp0r( CP0_BADVADDR, adr );
	}
	else
	{
		mipscpu.delaypc = adr;
		mipscpu.delay = 1;
		mipscpu.pc += 4;
	}
}

INLINE void mips_set_pc( unsigned val )
{
	mipscpu.pc = val;
	change_pc32ledw( val );
	mipscpu.delaypc = 0;
	mipscpu.delay = 0;
}

INLINE void mips_advance_pc( void )
{
	if( mipscpu.delay )
	{
		mips_set_pc( mipscpu.delaypc );
	}
	else
	{
		mipscpu.pc += 4;
	}
}

static void mips_exception( int exception )
{
	mips_set_cp0r( CP0_SR, ( mipscpu.cp0r[ CP0_SR ] & ~0x3f ) | ( ( mipscpu.cp0r[ CP0_SR ] << 2 ) & 0x3f ) );
	if( mipscpu.delay )
	{
		mips_set_cp0r( CP0_EPC, mipscpu.pc - 4 );
		mips_set_cp0r( CP0_CAUSE, ( mipscpu.cp0r[ CP0_CAUSE ] & ~CAUSE_EXC ) | CAUSE_BD | ( exception << 2 ) );
	}
	else
	{
		mips_set_cp0r( CP0_EPC, mipscpu.pc );
		mips_set_cp0r( CP0_CAUSE, ( mipscpu.cp0r[ CP0_CAUSE ] & ~( CAUSE_EXC | CAUSE_BD ) ) | ( exception << 2 ) );
	}
	if( mipscpu.cp0r[ CP0_SR ] & SR_BEV )
	{
		mips_set_pc( 0xbfc00180 );
	}
	else
	{
		mips_set_pc( 0x80000080 );
	}
}

void mips_init( void )
{
}

void mips_reset( void *param )
{
	mips_set_cp0r( CP0_SR, ( mipscpu.cp0r[ CP0_SR ] & ~( SR_TS | SR_SWC | SR_KUC | SR_IEC ) ) | SR_BEV );
	mips_set_cp0r( CP0_RANDOM, 63 ); /* todo: */
	mips_set_cp0r( CP0_PRID, 0x00000200 ); /* todo: */
	mips_set_pc( 0xbfc00000 );
}

void mips_exit( void )
{
}

int mips_execute( int cycles )
{
	mips_ICount = cycles;
	do
	{
		CALL_MAME_DEBUG;

		mipscpu.op = cpu_readop32( mipscpu.pc );
		switch( INS_OP( mipscpu.op ) )
		{
		case OP_SPECIAL:
			switch( INS_FUNCT( mipscpu.op ) )
			{
			case FUNCT_SLL:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.r[ INS_RT( mipscpu.op ) ] << INS_SHAMT( mipscpu.op );
				}
				mips_advance_pc();
				break;
			case FUNCT_SRL:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.r[ INS_RT( mipscpu.op ) ] >> INS_SHAMT( mipscpu.op );
				}
				mips_advance_pc();
				break;
			case FUNCT_SRA:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = (INT32)mipscpu.r[ INS_RT( mipscpu.op ) ] >> INS_SHAMT( mipscpu.op );
				}
				mips_advance_pc();
				break;
			case FUNCT_SLLV:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.r[ INS_RT( mipscpu.op ) ] << ( mipscpu.r[ INS_RS( mipscpu.op ) ] & 31 );
				}
				mips_advance_pc();
				break;
			case FUNCT_SRLV:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.r[ INS_RT( mipscpu.op ) ] >> ( mipscpu.r[ INS_RS( mipscpu.op ) ] & 31 );
				}
				mips_advance_pc();
				break;
			case FUNCT_SRAV:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = (INT32)mipscpu.r[ INS_RT( mipscpu.op ) ] >> ( mipscpu.r[ INS_RS( mipscpu.op ) ] & 31 );
				}
				mips_advance_pc();
				break;
			case FUNCT_JR:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					mips_delayed_branch( mipscpu.r[ INS_RS( mipscpu.op ) ] );
				}
				break;
			case FUNCT_JALR:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.pc + 8;
				}
				mips_delayed_branch( mipscpu.r[ INS_RS( mipscpu.op ) ] );
				break;
			case FUNCT_SYSCALL:
				mips_exception( EXC_SYS );
				break;
			case FUNCT_BREAK:
				mips_exception( EXC_BP );
				break;
			case FUNCT_MFHI:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.hi;
				}
				mips_advance_pc();
				break;
			case FUNCT_MTHI:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					mipscpu.hi = mipscpu.r[ INS_RS( mipscpu.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_MFLO:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.lo;
				}
				mips_advance_pc();
				break;
			case FUNCT_MTLO:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					mipscpu.lo = mipscpu.r[ INS_RS( mipscpu.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_MULT:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					INT64 n_res;
					n_res = MUL_64_32_32( (INT32)mipscpu.r[ INS_RS( mipscpu.op ) ], (INT32)mipscpu.r[ INS_RT( mipscpu.op ) ] );
					mipscpu.lo = LO32_32_64( n_res );
					mipscpu.hi = HI32_32_64( n_res );
					mips_advance_pc();
				}
				break;
			case FUNCT_MULTU:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					UINT64 n_res;
					n_res = MUL_U64_U32_U32( mipscpu.r[ INS_RS( mipscpu.op ) ], mipscpu.r[ INS_RT( mipscpu.op ) ] );
					mipscpu.lo = LO32_U32_U64( n_res );
					mipscpu.hi = HI32_U32_U64( n_res );
					mips_advance_pc();
				}
				break;
			case FUNCT_DIV:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					mipscpu.lo = (INT32)mipscpu.r[ INS_RS( mipscpu.op ) ] / (INT32)mipscpu.r[ INS_RT( mipscpu.op ) ];
					mipscpu.hi = (INT32)mipscpu.r[ INS_RS( mipscpu.op ) ] % (INT32)mipscpu.r[ INS_RT( mipscpu.op ) ];
					mips_advance_pc();
				}
				break;
			case FUNCT_DIVU:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mips_exception( EXC_RI );
				}
				else
				{
					mipscpu.lo = mipscpu.r[ INS_RS( mipscpu.op ) ] / mipscpu.r[ INS_RT( mipscpu.op ) ];
					mipscpu.hi = mipscpu.r[ INS_RS( mipscpu.op ) ] % mipscpu.r[ INS_RT( mipscpu.op ) ];
					mips_advance_pc();
				}
				break;
			case FUNCT_ADD:
				{
					UINT32 n_res;
					n_res = mipscpu.r[ INS_RS( mipscpu.op ) ] + mipscpu.r[ INS_RT( mipscpu.op ) ];
					if( ( ( mipscpu.r[ INS_RS( mipscpu.op ) ] ^ n_res ) & ( mipscpu.r[ INS_RT( mipscpu.op ) ] ^ n_res ) & ( 1 << 31 ) ) != 0 )
					{
						mips_exception( EXC_OVF );
					}
					else
					{
						if( INS_RD( mipscpu.op ) != 0 )
						{
							mipscpu.r[ INS_RD( mipscpu.op ) ] = n_res;
						}
						mips_advance_pc();
					}
				}
				break;
			case FUNCT_ADDU:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.r[ INS_RS( mipscpu.op ) ] + mipscpu.r[ INS_RT( mipscpu.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_SUB:
				{
					UINT32 n_res;
					n_res = mipscpu.r[ INS_RS( mipscpu.op ) ] - mipscpu.r[ INS_RT( mipscpu.op ) ];
					if( ( ( mipscpu.r[ INS_RS( mipscpu.op ) ] ^ n_res ) & ( mipscpu.r[ INS_RT( mipscpu.op ) ] ^ n_res ) & ( 1 << 31 ) ) != 0 )
					{
						mips_exception( EXC_OVF );
					}
					else
					{
						if( INS_RD( mipscpu.op ) != 0 )
						{
							mipscpu.r[ INS_RD( mipscpu.op ) ] = n_res;
						}
						mips_advance_pc();
					}
				}
				break;
			case FUNCT_SUBU:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.r[ INS_RS( mipscpu.op ) ] - mipscpu.r[ INS_RT( mipscpu.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_AND:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.r[ INS_RS( mipscpu.op ) ] & mipscpu.r[ INS_RT( mipscpu.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_OR:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.r[ INS_RS( mipscpu.op ) ] | mipscpu.r[ INS_RT( mipscpu.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_XOR:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.r[ INS_RS( mipscpu.op ) ] ^ mipscpu.r[ INS_RT( mipscpu.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_NOR:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = ~( mipscpu.r[ INS_RS( mipscpu.op ) ] | mipscpu.r[ INS_RT( mipscpu.op ) ] );
				}
				mips_advance_pc();
				break;
			case FUNCT_SLT:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = (INT32)mipscpu.r[ INS_RS( mipscpu.op ) ] < (INT32)mipscpu.r[ INS_RT( mipscpu.op ) ];
				}
				mips_advance_pc();
				break;
			case FUNCT_SLTU:
				if( INS_RD( mipscpu.op ) != 0 )
				{
					mipscpu.r[ INS_RD( mipscpu.op ) ] = mipscpu.r[ INS_RS( mipscpu.op ) ] < mipscpu.r[ INS_RT( mipscpu.op ) ];
				}
				mips_advance_pc();
				break;
			default:
				mips_exception( EXC_RI );
				break;
			}
			break;
		case OP_REGIMM:
			switch( INS_RT( mipscpu.op ) )
			{
			case RT_BLTZ:
				if( (INT32)mipscpu.r[ INS_RS( mipscpu.op ) ] < 0 )
				{
					mips_delayed_branch( mipscpu.pc + 4 + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) ) << 2 ) );
				}
				else
				{
					mips_advance_pc();
				}
				break;
			case RT_BGEZ:
				if( (INT32)mipscpu.r[ INS_RS( mipscpu.op ) ] >= 0 )
				{
					mips_delayed_branch( mipscpu.pc + 4 + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) ) << 2 ) );
				}
				else
				{
					mips_advance_pc();
				}
				break;
			case RT_BLTZAL:
				mipscpu.r[ 31 ] = mipscpu.pc + 8;
				if( (INT32)mipscpu.r[ INS_RS( mipscpu.op ) ] < 0 )
				{
					mips_delayed_branch( mipscpu.pc + 4 + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) ) << 2 ) );
				}
				else
				{
					mips_advance_pc();
				}
				break;
			case RT_BGEZAL:
				mipscpu.r[ 31 ] = mipscpu.pc + 8;
				if( (INT32)mipscpu.r[ INS_RS( mipscpu.op ) ] >= 0 )
				{
					mips_delayed_branch( mipscpu.pc + 4 + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) ) << 2 ) );
				}
				else
				{
					mips_advance_pc();
				}
				break;
			}
			break;
		case OP_J:
			mips_delayed_branch( ( ( mipscpu.pc + 4 ) & 0xf0000000 ) + ( INS_TARGET( mipscpu.op ) << 2 ) );
			break;
		case OP_JAL:
			mipscpu.r[ 31 ] = mipscpu.pc + 8;
			mips_delayed_branch( ( ( mipscpu.pc + 4 ) & 0xf0000000 ) + ( INS_TARGET( mipscpu.op ) << 2 ) );
			break;
		case OP_BEQ:
			if( mipscpu.r[ INS_RS( mipscpu.op ) ] == mipscpu.r[ INS_RT( mipscpu.op ) ] )
			{
				mips_delayed_branch( mipscpu.pc + 4 + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) ) << 2 ) );
			}
			else
			{
				mips_advance_pc();
			}
			break;
		case OP_BNE:
			if( mipscpu.r[ INS_RS( mipscpu.op ) ] != mipscpu.r[ INS_RT( mipscpu.op ) ] )
			{
				mips_delayed_branch( mipscpu.pc + 4 + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) ) << 2 ) );
			}
			else
			{
				mips_advance_pc();
			}
			break;
		case OP_BLEZ:
			if( INS_RT( mipscpu.op ) != 0 )
			{
				mips_exception( EXC_RI );
			}
			else if( (INT32)mipscpu.r[ INS_RS( mipscpu.op ) ] <= 0 )
			{
				mips_delayed_branch( mipscpu.pc + 4 + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) ) << 2 ) );
			}
			else
			{
				mips_advance_pc();
			}
			break;
		case OP_BGTZ:
			if( INS_RT( mipscpu.op ) != 0 )
			{
				mips_exception( EXC_RI );
			}
			else if( (INT32)mipscpu.r[ INS_RS( mipscpu.op ) ] > 0 )
			{
				mips_delayed_branch( mipscpu.pc + 4 + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) ) << 2 ) );
			}
			else
			{
				mips_advance_pc();
			}
			break;
		case OP_ADDI:
			{
				UINT32 n_res;
				UINT32 n_imm;
				n_imm = MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				n_res = mipscpu.r[ INS_RS( mipscpu.op ) ] + n_imm;
				if( ( ( mipscpu.r[ INS_RS( mipscpu.op ) ] ^ n_res ) & ( n_imm ^ n_res ) & ( 1 << 31 ) ) != 0 )
				{
					mips_exception( EXC_OVF );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = n_res;
					}
					mips_advance_pc();
				}
			}
			break;
		case OP_ADDIU:
			if( INS_RT( mipscpu.op ) != 0 )
			{
				mipscpu.r[ INS_RT( mipscpu.op ) ] = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
			}
			mips_advance_pc();
			break;
		case OP_SLTI:
			if( INS_RT( mipscpu.op ) != 0 )
			{
				mipscpu.r[ INS_RT( mipscpu.op ) ] = (INT32)mipscpu.r[ INS_RS( mipscpu.op ) ] < MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
			}
			mips_advance_pc();
			break;
		case OP_SLTIU:
			if( INS_RT( mipscpu.op ) != 0 )
			{
				mipscpu.r[ INS_RT( mipscpu.op ) ] = mipscpu.r[ INS_RS( mipscpu.op ) ] < (UINT32)MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
			}
			mips_advance_pc();
			break;
		case OP_ANDI:
			if( INS_RT( mipscpu.op ) != 0 )
			{
				mipscpu.r[ INS_RT( mipscpu.op ) ] = mipscpu.r[ INS_RS( mipscpu.op ) ] & INS_IMMEDIATE( mipscpu.op );
			}
			mips_advance_pc();
			break;
		case OP_ORI:
			if( INS_RT( mipscpu.op ) != 0 )
			{
				mipscpu.r[ INS_RT( mipscpu.op ) ] = mipscpu.r[ INS_RS( mipscpu.op ) ] | INS_IMMEDIATE( mipscpu.op );
			}
			mips_advance_pc();
			break;
		case OP_XORI:
			if( INS_RT( mipscpu.op ) != 0 )
			{
				mipscpu.r[ INS_RT( mipscpu.op ) ] = mipscpu.r[ INS_RS( mipscpu.op ) ] ^ INS_IMMEDIATE( mipscpu.op );
			}
			mips_advance_pc();
			break;
		case OP_LUI:
			if( INS_RT( mipscpu.op ) != 0 )
			{
				mipscpu.r[ INS_RT( mipscpu.op ) ] = INS_IMMEDIATE( mipscpu.op ) << 16;
			}
			mips_advance_pc();
			break;
		case OP_COP0:
			if( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) != 0 && ( mipscpu.cp0r[ CP0_SR ] & SR_CU0 ) == 0 )
			{
				mips_exception( EXC_CPU );
				mips_set_cp0r( CP0_CAUSE, ( mipscpu.cp0r[ CP0_CAUSE ] & ~CAUSE_CE ) | CAUSE_CE0 );
			}
			else
			{
				switch( INS_RS( mipscpu.op ) )
				{
				case RS_MFC:
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = mipscpu.cp0r[ INS_RD( mipscpu.op ) ];
					}
					mips_advance_pc();
					break;
				case RS_CFC:
					/* todo: */
					mips_stop();
					mips_advance_pc();
					break;
				case RS_MTC:
					mips_advance_pc();
					mips_set_cp0r( INS_RD( mipscpu.op ),
						( mipscpu.cp0r[ INS_RD( mipscpu.op ) ] & ~mips_mtc0_writemask[ INS_RD( mipscpu.op ) ] ) |
						( mipscpu.r[ INS_RT( mipscpu.op ) ] & mips_mtc0_writemask[ INS_RD( mipscpu.op ) ] ) );
					break;
				case RS_CTC:
					/* todo: */
					mips_stop();
					mips_advance_pc();
					break;
				case RS_BC:
					switch( INS_RT( mipscpu.op ) )
					{
					case RT_BCF:
						/* todo: */
						mips_stop();
						mips_advance_pc();
						break;
					case RT_BCT:
						/* todo: */
						mips_stop();
						mips_advance_pc();
						break;
					default:
						/* todo: */
						mips_stop();
						mips_advance_pc();
						break;
					}
					break;
				default:
					switch( INS_CO( mipscpu.op ) )
					{
					case 1:
						switch( INS_CF( mipscpu.op ) )
						{
						case CF_RFE:
							mips_advance_pc();
							mips_set_cp0r( CP0_SR, ( mipscpu.cp0r[ CP0_SR ] & ~0xf ) | ( ( mipscpu.cp0r[ CP0_SR ] >> 2 ) & 0xf ) );
							break;
						default:
							/* todo: */
							mips_stop();
							mips_advance_pc();
							break;
						}
						break;
					default:
						/* todo: */
						mips_stop();
						mips_advance_pc();
						break;
					}
					break;
				}
			}
			break;
		case OP_COP1:
			if( ( mipscpu.cp0r[ CP0_SR ] & SR_CU1 ) == 0 )
			{
				mips_exception( EXC_CPU );
				mips_set_cp0r( CP0_CAUSE, ( mipscpu.cp0r[ CP0_CAUSE ] & ~CAUSE_CE ) | CAUSE_CE1 );
			}
			else
			{
				switch( INS_RS( mipscpu.op ) )
				{
				case RS_MFC:
					/* todo: */
					mips_stop();
					mips_advance_pc();
					break;
				case RS_CFC:
					/* todo: */
					mips_stop();
					mips_advance_pc();
					break;
				case RS_MTC:
					/* todo: */
					mips_stop();
					mips_advance_pc();
					break;
				case RS_CTC:
					/* todo: */
					mips_stop();
					mips_advance_pc();
					break;
				case RS_BC:
					switch( INS_RT( mipscpu.op ) )
					{
					case RT_BCF:
						/* todo: */
						mips_stop();
						mips_advance_pc();
						break;
					case RT_BCT:
						/* todo: */
						mips_stop();
						mips_advance_pc();
						break;
					default:
						/* todo: */
						mips_stop();
						mips_advance_pc();
						break;
					}
					break;
				default:
					switch( INS_CO( mipscpu.op ) )
					{
					case 1:
						/* todo: */
						mips_stop();
						mips_advance_pc();
						break;
					default:
						/* todo: */
						mips_stop();
						mips_advance_pc();
						break;
					}
					break;
				}
			}
			break;
		case OP_COP2:
			if( ( mipscpu.cp0r[ CP0_SR ] & SR_CU2 ) == 0 )
			{
				mips_exception( EXC_CPU );
				mips_set_cp0r( CP0_CAUSE, ( mipscpu.cp0r[ CP0_CAUSE ] & ~CAUSE_CE ) | CAUSE_CE2 );
			}
			else
			{
				switch( INS_RS( mipscpu.op ) )
				{
				case RS_MFC:
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = getcp2dr( INS_RD( mipscpu.op ) );
					}
					else
					{
						getcp2dr( INS_RD( mipscpu.op ) );
					}
					mips_advance_pc();
					break;
				case RS_CFC:
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = getcp2cr( INS_RD( mipscpu.op ) );
					}
					else
					{
						getcp2cr( INS_RD( mipscpu.op ) );
					}
					mips_advance_pc();
					break;
				case RS_MTC:
					setcp2dr( INS_RD( mipscpu.op ), mipscpu.r[ INS_RT( mipscpu.op ) ] );
					mips_advance_pc();
					break;
				case RS_CTC:
					setcp2cr( INS_RD( mipscpu.op ), mipscpu.r[ INS_RT( mipscpu.op ) ] );
					mips_advance_pc();
					break;
				case RS_BC:
					switch( INS_RT( mipscpu.op ) )
					{
					case RT_BCF:
						/* todo: */
						mips_stop();
						mips_advance_pc();
						break;
					case RT_BCT:
						/* todo: */
						mips_stop();
						mips_advance_pc();
						break;
					default:
						logerror( "%08x: unknown gpu op %08x\n", mipscpu.pc, mipscpu.op );
						mips_stop();
						mips_advance_pc();
						break;
					}
					break;
				default:
					switch( INS_CO( mipscpu.op ) )
					{
					case 1:
						docop2( INS_COFUN( mipscpu.op ) );
						mips_advance_pc();
						break;
					default:
						logerror( "%08x: unknown gte op %08x\n", mipscpu.pc, mipscpu.op );
						mips_stop();
						mips_advance_pc();
						break;
					}
					break;
				}
			}
			break;
		case OP_LB:
			if( mipscpu.cp0r[ CP0_SR ] & ( SR_ISC | SR_SWC ) )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else if( ( mipscpu.cp0r[ CP0_SR ] & ( SR_RE | SR_KUC ) ) == ( SR_RE | SR_KUC ) )
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = MIPS_BYTE_EXTEND( cpu_readmem32ledw( adr ^ 3 ) );
					}
					else
					{
						cpu_readmem32ledw( adr ^ 3 );
					}
					mips_advance_pc();
				}
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = MIPS_BYTE_EXTEND( cpu_readmem32ledw( adr ) );
					}
					else
					{
						cpu_readmem32ledw( adr );
					}
					mips_advance_pc();
				}
			}
			break;
		case OP_LH:
			if( mipscpu.cp0r[ CP0_SR ] & ( SR_ISC | SR_SWC ) )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else if( ( mipscpu.cp0r[ CP0_SR ] & ( SR_RE | SR_KUC ) ) == ( SR_RE | SR_KUC ) )
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 1 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = MIPS_WORD_EXTEND( cpu_readmem32ledw_word( adr ^ 2 ) );
					}
					else
					{
						cpu_readmem32ledw_word( adr ^ 2 );
					}
					mips_advance_pc();
				}
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 1 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = MIPS_WORD_EXTEND( cpu_readmem32ledw_word( adr ) );
					}
					else
					{
						cpu_readmem32ledw_word( adr );
					}
					mips_advance_pc();
				}
			}
			break;
		case OP_LWL:
			if( mipscpu.cp0r[ CP0_SR ] & ( SR_ISC | SR_SWC ) )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else if( ( mipscpu.cp0r[ CP0_SR ] & ( SR_RE | SR_KUC ) ) == ( SR_RE | SR_KUC ) )
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						switch( adr & 3 )
						{
						case 0:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0x00ffffff ) | ( (UINT32)cpu_readmem32ledw( adr + 3 ) << 24 );
							break;
						case 1:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0x0000ffff ) | ( (UINT32)cpu_readmem32ledw_word( adr + 1 ) << 16 );
							break;
						case 2:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0x000000ff ) | ( (UINT32)cpu_readmem32ledw( adr - 1 ) << 8 ) | ( (UINT32)cpu_readmem32ledw_word( adr ) << 16 );
							break;
						case 3:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = cpu_readmem32ledw_dword( adr - 3 );
							break;
						}
					}
					else
					{
						switch( adr & 3 )
						{
						case 0:
							cpu_readmem32ledw( adr + 3 );
							break;
						case 1:
							cpu_readmem32ledw_word( adr + 1 );
							break;
						case 2:
							cpu_readmem32ledw( adr - 1 );
							cpu_readmem32ledw_word( adr );
							break;
						case 3:
							cpu_readmem32ledw_dword( adr - 3 );
							break;
						}
					}
					mips_advance_pc();
				}
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						switch( adr & 3 )
						{
						case 0:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0x00ffffff ) | ( (UINT32)cpu_readmem32ledw( adr ) << 24 );
							break;
						case 1:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0x0000ffff ) | ( (UINT32)cpu_readmem32ledw_word( adr - 1 ) << 16 );
							break;
						case 2:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0x000000ff ) | ( (UINT32)cpu_readmem32ledw_word( adr - 2 ) << 8 ) | ( (UINT32)cpu_readmem32ledw( adr ) << 24 );
							break;
						case 3:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = cpu_readmem32ledw_dword( adr - 3 );
							break;
						}
					}
					else
					{
						switch( adr & 3 )
						{
						case 0:
							cpu_readmem32ledw( adr );
							break;
						case 1:
							cpu_readmem32ledw_word( adr - 1 );
							break;
						case 2:
							cpu_readmem32ledw_word( adr - 2 );
							cpu_readmem32ledw( adr );
							break;
						case 3:
							cpu_readmem32ledw_dword( adr - 3 );
							break;
						}
					}
					mips_advance_pc();
				}
			}
			break;
		case OP_LW:
			if( mipscpu.cp0r[ CP0_SR ] & ( SR_ISC | SR_SWC ) )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 3 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = cpu_readmem32ledw_dword( adr );
					}
					else
					{
						cpu_readmem32ledw_dword( adr );
					}
					mips_advance_pc();
				}
			}
			break;
		case OP_LBU:
			if( mipscpu.cp0r[ CP0_SR ] & ( SR_ISC | SR_SWC ) )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else if( ( mipscpu.cp0r[ CP0_SR ] & ( SR_RE | SR_KUC ) ) == ( SR_RE | SR_KUC ) )
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = cpu_readmem32ledw( adr ^ 3 );
					}
					else
					{
						cpu_readmem32ledw( adr ^ 3 );
					}
					mips_advance_pc();
				}
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = cpu_readmem32ledw( adr );
					}
					else
					{
						cpu_readmem32ledw( adr );
					}
					mips_advance_pc();
				}
			}
			break;
		case OP_LHU:
			if( mipscpu.cp0r[ CP0_SR ] & ( SR_ISC | SR_SWC ) )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else if( ( mipscpu.cp0r[ CP0_SR ] & ( SR_RE | SR_KUC ) ) == ( SR_RE | SR_KUC ) )
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 1 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = cpu_readmem32ledw_word( adr ^ 2 );
					}
					else
					{
						cpu_readmem32ledw_word( adr ^ 2 );
					}
					mips_advance_pc();
				}
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 1 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						mipscpu.r[ INS_RT( mipscpu.op ) ] = cpu_readmem32ledw_word( adr );
					}
					else
					{
						cpu_readmem32ledw_word( adr );
					}
					mips_advance_pc();
				}
			}
			break;
		case OP_LWR:
			if( mipscpu.cp0r[ CP0_SR ] & ( SR_ISC | SR_SWC ) )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else if( ( mipscpu.cp0r[ CP0_SR ] & ( SR_RE | SR_KUC ) ) == ( SR_RE | SR_KUC ) )
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						switch( adr & 3 )
						{
						case 0:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = cpu_readmem32ledw_dword( adr );
							break;
						case 1:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0xff000000 ) | cpu_readmem32ledw_word( adr - 1 ) | ( (UINT32)cpu_readmem32ledw( adr + 1 ) << 16 );
							break;
						case 2:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0xffff0000 ) | cpu_readmem32ledw_word( adr - 2 );
							break;
						case 3:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0xffffff00 ) | cpu_readmem32ledw( adr - 3 );
							break;
						}
					}
					else
					{
						switch( adr & 3 )
						{
						case 0:
							cpu_readmem32ledw_dword( adr );
							break;
						case 1:
							cpu_readmem32ledw_word( adr - 1 );
							cpu_readmem32ledw( adr + 1 );
							break;
						case 2:
							cpu_readmem32ledw_word( adr - 2 );
							break;
						case 3:
							cpu_readmem32ledw( adr - 3 );
							break;
						}
					}
					mips_advance_pc();
				}
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					if( INS_RT( mipscpu.op ) != 0 )
					{
						switch( adr & 3 )
						{
						case 0:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = cpu_readmem32ledw_dword( adr );
							break;
						case 1:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0xff000000 ) | cpu_readmem32ledw( adr ) | ( (UINT32)cpu_readmem32ledw_word( adr + 1 ) << 8 );
							break;
						case 2:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0xffff0000 ) | cpu_readmem32ledw_word( adr );
							break;
						case 3:
							mipscpu.r[ INS_RT( mipscpu.op ) ] = ( mipscpu.r[ INS_RT( mipscpu.op ) ] & 0xffffff00 ) | cpu_readmem32ledw( adr );
							break;
						}
					}
					else
					{
						switch( adr & 3 )
						{
						case 0:
							cpu_readmem32ledw_dword( adr );
							break;
						case 1:
							cpu_readmem32ledw( adr );
							cpu_readmem32ledw_word( adr + 1 );
							break;
						case 2:
							cpu_readmem32ledw_word( adr );
							break;
						case 3:
							cpu_readmem32ledw( adr );
							break;
						}
					}
					mips_advance_pc();
				}
			}
			break;
		case OP_SB:
			if( mipscpu.cp0r[ CP0_SR ] & ( SR_ISC | SR_SWC ) )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else if( ( mipscpu.cp0r[ CP0_SR ] & ( SR_RE | SR_KUC ) ) == ( SR_RE | SR_KUC ) )
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADES );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					cpu_writemem32ledw( adr ^ 3, mipscpu.r[ INS_RT( mipscpu.op ) ] );
					mips_advance_pc();
				}
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADES );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					cpu_writemem32ledw( adr, mipscpu.r[ INS_RT( mipscpu.op ) ] );
					mips_advance_pc();
				}
			}
			break;
		case OP_SH:
			if( mipscpu.cp0r[ CP0_SR ] & ( SR_ISC | SR_SWC ) )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else if( ( mipscpu.cp0r[ CP0_SR ] & ( SR_RE | SR_KUC ) ) == ( SR_RE | SR_KUC ) )
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 1 ) ) != 0 )
				{
					mips_exception( EXC_ADES );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					cpu_writemem32ledw_word( adr ^ 2, mipscpu.r[ INS_RT( mipscpu.op ) ] );
					mips_advance_pc();
				}
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 1 ) ) != 0 )
				{
					mips_exception( EXC_ADES );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					cpu_writemem32ledw_word( adr, mipscpu.r[ INS_RT( mipscpu.op ) ] );
					mips_advance_pc();
				}
			}
			break;
		case OP_SWL:
			if( mipscpu.cp0r[ CP0_SR ] & ( SR_ISC | SR_SWC ) )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else if( ( mipscpu.cp0r[ CP0_SR ] & ( SR_RE | SR_KUC ) ) == ( SR_RE | SR_KUC ) )
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADES );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					switch( adr & 3 )
					{
					case 0:
						cpu_writemem32ledw( adr + 3, mipscpu.r[ INS_RT( mipscpu.op ) ] >> 24 );
						break;
					case 1:
						cpu_writemem32ledw_word( adr + 1, mipscpu.r[ INS_RT( mipscpu.op ) ] >> 16 );
						break;
					case 2:
						cpu_writemem32ledw( adr - 1, mipscpu.r[ INS_RT( mipscpu.op ) ] >> 8 );
						cpu_writemem32ledw_word( adr, mipscpu.r[ INS_RT( mipscpu.op ) ] >> 16 );
						break;
					case 3:
						cpu_writemem32ledw_dword( adr - 3, mipscpu.r[ INS_RT( mipscpu.op ) ] );
						break;
					}
					mips_advance_pc();
				}
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADES );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					switch( adr & 3 )
					{
					case 0:
						cpu_writemem32ledw( adr, mipscpu.r[ INS_RT( mipscpu.op ) ] >> 24 );
						break;
					case 1:
						cpu_writemem32ledw_word( adr - 1, mipscpu.r[ INS_RT( mipscpu.op ) ] >> 16 );
						break;
					case 2:
						cpu_writemem32ledw_word( adr - 2, mipscpu.r[ INS_RT( mipscpu.op ) ] >> 8 );
						cpu_writemem32ledw( adr, mipscpu.r[ INS_RT( mipscpu.op ) ] >> 24 );
						break;
					case 3:
						cpu_writemem32ledw_dword( adr - 3, mipscpu.r[ INS_RT( mipscpu.op ) ] );
						break;
					}
					mips_advance_pc();
				}
			}
			break;
		case OP_SW:
			if( mipscpu.cp0r[ CP0_SR ] & SR_ISC )
			{
				/* todo: */
				mips_advance_pc();
			}
			else if( mipscpu.cp0r[ CP0_SR ] & SR_SWC )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 3 ) ) != 0 )
				{
					mips_exception( EXC_ADES );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					cpu_writemem32ledw_dword( adr, mipscpu.r[ INS_RT( mipscpu.op ) ] );
					mips_advance_pc();
				}
			}
			break;
		case OP_SWR:
			if( mipscpu.cp0r[ CP0_SR ] & ( SR_ISC | SR_SWC ) )
			{
				/* todo: */
				mips_stop();
				mips_advance_pc();
			}
			else if( ( mipscpu.cp0r[ CP0_SR ] & ( SR_RE | SR_KUC ) ) == ( SR_RE | SR_KUC ) )
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADES );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					switch( adr & 3 )
					{
					case 0:
						cpu_writemem32ledw_dword( adr, mipscpu.r[ INS_RT( mipscpu.op ) ] );
						break;
					case 1:
						cpu_writemem32ledw_word( adr - 1, mipscpu.r[ INS_RT( mipscpu.op ) ] );
						cpu_writemem32ledw( adr + 1, mipscpu.r[ INS_RT( mipscpu.op ) ] >> 16 );
						break;
					case 2:
						cpu_writemem32ledw_word( adr - 2, mipscpu.r[ INS_RT( mipscpu.op ) ] );
						break;
					case 3:
						cpu_writemem32ledw( adr - 3, mipscpu.r[ INS_RT( mipscpu.op ) ] );
						break;
					}
					mips_advance_pc();
				}
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) ) != 0 )
				{
					mips_exception( EXC_ADES );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					switch( adr & 3 )
					{
					case 0:
						cpu_writemem32ledw_dword( adr, mipscpu.r[ INS_RT( mipscpu.op ) ] );
						break;
					case 1:
						cpu_writemem32ledw( adr, mipscpu.r[ INS_RT( mipscpu.op ) ] );
						cpu_writemem32ledw_word( adr + 1, mipscpu.r[ INS_RT( mipscpu.op ) ] >> 8 );
						break;
					case 2:
						cpu_writemem32ledw_word( adr, mipscpu.r[ INS_RT( mipscpu.op ) ] );
						break;
					case 3:
						cpu_writemem32ledw( adr, mipscpu.r[ INS_RT( mipscpu.op ) ] );
						break;
					}
					mips_advance_pc();
				}
			}
			break;
		case OP_LWC1:
			/* todo: */
			mips_stop();
			mips_advance_pc();
			break;
		case OP_LWC2:
			if( ( mipscpu.cp0r[ CP0_SR ] & SR_CU2 ) == 0 )
			{
				mips_exception( EXC_CPU );
				mips_set_cp0r( CP0_CAUSE, ( mipscpu.cp0r[ CP0_CAUSE ] & ~CAUSE_CE ) | CAUSE_CE2 );
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 3 ) ) != 0 )
				{
					mips_exception( EXC_ADEL );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					setcp2dr( INS_RT( mipscpu.op ), cpu_readmem32ledw_dword( adr ) );
					mips_advance_pc();
				}
			}
			break;
		case OP_SWC1:
			/* todo: */
			mips_stop();
			mips_advance_pc();
			break;
		case OP_SWC2:
			if( ( mipscpu.cp0r[ CP0_SR ] & SR_CU2 ) == 0 )
			{
				mips_exception( EXC_CPU );
				mips_set_cp0r( CP0_CAUSE, ( mipscpu.cp0r[ CP0_CAUSE ] & ~CAUSE_CE ) | CAUSE_CE2 );
			}
			else
			{
				UINT32 adr;
				adr = mipscpu.r[ INS_RS( mipscpu.op ) ] + MIPS_WORD_EXTEND( INS_IMMEDIATE( mipscpu.op ) );
				if( ( adr & ( ( ( mipscpu.cp0r[ CP0_SR ] & SR_KUC ) << 30 ) | 3 ) ) != 0 )
				{
					mips_exception( EXC_ADES );
					mips_set_cp0r( CP0_BADVADDR, adr );
				}
				else
				{
					cpu_writemem32ledw_dword( adr, getcp2dr( INS_RT( mipscpu.op ) ) );
					mips_advance_pc();
				}
			}
			break;
		default:
			mips_stop();
			mips_exception( EXC_RI );
			break;
		}
		mips_ICount--;
	} while( mips_ICount > 0 );

	return cycles - mips_ICount;
}

unsigned mips_get_context( void *dst )
{
	if( dst )
	{
		*(mips_cpu_context *)dst = mipscpu;
	}
	return sizeof( mips_cpu_context );
}

void mips_set_context( void *src )
{
	if( src )
	{
		mipscpu = *(mips_cpu_context *)src;
		change_pc32ledw( mipscpu.pc );
	}
}

unsigned mips_get_reg( int regnum )
{
	switch( regnum )
	{
	case REG_PC:		return mipscpu.pc;
	case MIPS_PC:		return mipscpu.pc;
	case REG_SP:
		/* because there is no hardware stack and the pipeline causes the cpu to execute the
		instruction after a subroutine call before the subroutine is executed there is little
		chance of cmd_step_over() in mamedbg.c working. */
						return 0;
	case MIPS_DELAYPC:	return mipscpu.delaypc;
	case MIPS_DELAY:	return mipscpu.delay;
	case MIPS_HI:		return mipscpu.hi;
	case MIPS_LO:		return mipscpu.lo;
	case MIPS_R0:		return mipscpu.r[ 0 ];
	case MIPS_R1:		return mipscpu.r[ 1 ];
	case MIPS_R2:		return mipscpu.r[ 2 ];
	case MIPS_R3:		return mipscpu.r[ 3 ];
	case MIPS_R4:		return mipscpu.r[ 4 ];
	case MIPS_R5:		return mipscpu.r[ 5 ];
	case MIPS_R6:		return mipscpu.r[ 6 ];
	case MIPS_R7:		return mipscpu.r[ 7 ];
	case MIPS_R8:		return mipscpu.r[ 8 ];
	case MIPS_R9:		return mipscpu.r[ 9 ];
	case MIPS_R10:		return mipscpu.r[ 10 ];
	case MIPS_R11:		return mipscpu.r[ 11 ];
	case MIPS_R12:		return mipscpu.r[ 12 ];
	case MIPS_R13:		return mipscpu.r[ 13 ];
	case MIPS_R14:		return mipscpu.r[ 14 ];
	case MIPS_R15:		return mipscpu.r[ 15 ];
	case MIPS_R16:		return mipscpu.r[ 16 ];
	case MIPS_R17:		return mipscpu.r[ 17 ];
	case MIPS_R18:		return mipscpu.r[ 18 ];
	case MIPS_R19:		return mipscpu.r[ 19 ];
	case MIPS_R20:		return mipscpu.r[ 20 ];
	case MIPS_R21:		return mipscpu.r[ 21 ];
	case MIPS_R22:		return mipscpu.r[ 22 ];
	case MIPS_R23:		return mipscpu.r[ 23 ];
	case MIPS_R24:		return mipscpu.r[ 24 ];
	case MIPS_R25:		return mipscpu.r[ 25 ];
	case MIPS_R26:		return mipscpu.r[ 26 ];
	case MIPS_R27:		return mipscpu.r[ 27 ];
	case MIPS_R28:		return mipscpu.r[ 28 ];
	case MIPS_R29:		return mipscpu.r[ 29 ];
	case MIPS_R30:		return mipscpu.r[ 30 ];
	case MIPS_R31:		return mipscpu.r[ 31 ];
	case MIPS_CP0R0:	return mipscpu.cp0r[ 0 ];
	case MIPS_CP0R1:	return mipscpu.cp0r[ 1 ];
	case MIPS_CP0R2:	return mipscpu.cp0r[ 2 ];
	case MIPS_CP0R3:	return mipscpu.cp0r[ 3 ];
	case MIPS_CP0R4:	return mipscpu.cp0r[ 4 ];
	case MIPS_CP0R5:	return mipscpu.cp0r[ 5 ];
	case MIPS_CP0R6:	return mipscpu.cp0r[ 6 ];
	case MIPS_CP0R7:	return mipscpu.cp0r[ 7 ];
	case MIPS_CP0R8:	return mipscpu.cp0r[ 8 ];
	case MIPS_CP0R9:	return mipscpu.cp0r[ 9 ];
	case MIPS_CP0R10:	return mipscpu.cp0r[ 10 ];
	case MIPS_CP0R11:	return mipscpu.cp0r[ 11 ];
	case MIPS_CP0R12:	return mipscpu.cp0r[ 12 ];
	case MIPS_CP0R13:	return mipscpu.cp0r[ 13 ];
	case MIPS_CP0R14:	return mipscpu.cp0r[ 14 ];
	case MIPS_CP0R15:	return mipscpu.cp0r[ 15 ];
	case MIPS_CP0R16:	return mipscpu.cp0r[ 16 ];
	case MIPS_CP0R17:	return mipscpu.cp0r[ 17 ];
	case MIPS_CP0R18:	return mipscpu.cp0r[ 18 ];
	case MIPS_CP0R19:	return mipscpu.cp0r[ 19 ];
	case MIPS_CP0R20:	return mipscpu.cp0r[ 20 ];
	case MIPS_CP0R21:	return mipscpu.cp0r[ 21 ];
	case MIPS_CP0R22:	return mipscpu.cp0r[ 22 ];
	case MIPS_CP0R23:	return mipscpu.cp0r[ 23 ];
	case MIPS_CP0R24:	return mipscpu.cp0r[ 24 ];
	case MIPS_CP0R25:	return mipscpu.cp0r[ 25 ];
	case MIPS_CP0R26:	return mipscpu.cp0r[ 26 ];
	case MIPS_CP0R27:	return mipscpu.cp0r[ 27 ];
	case MIPS_CP0R28:	return mipscpu.cp0r[ 28 ];
	case MIPS_CP0R29:	return mipscpu.cp0r[ 29 ];
	case MIPS_CP0R30:	return mipscpu.cp0r[ 30 ];
	case MIPS_CP0R31:	return mipscpu.cp0r[ 31 ];
	case MIPS_CP2DR0:	return mipscpu.cp2dr[ 0 ].d;
	case MIPS_CP2DR1:	return mipscpu.cp2dr[ 1 ].d;
	case MIPS_CP2DR2:	return mipscpu.cp2dr[ 2 ].d;
	case MIPS_CP2DR3:	return mipscpu.cp2dr[ 3 ].d;
	case MIPS_CP2DR4:	return mipscpu.cp2dr[ 4 ].d;
	case MIPS_CP2DR5:	return mipscpu.cp2dr[ 5 ].d;
	case MIPS_CP2DR6:	return mipscpu.cp2dr[ 6 ].d;
	case MIPS_CP2DR7:	return mipscpu.cp2dr[ 7 ].d;
	case MIPS_CP2DR8:	return mipscpu.cp2dr[ 8 ].d;
	case MIPS_CP2DR9:	return mipscpu.cp2dr[ 9 ].d;
	case MIPS_CP2DR10:	return mipscpu.cp2dr[ 10 ].d;
	case MIPS_CP2DR11:	return mipscpu.cp2dr[ 11 ].d;
	case MIPS_CP2DR12:	return mipscpu.cp2dr[ 12 ].d;
	case MIPS_CP2DR13:	return mipscpu.cp2dr[ 13 ].d;
	case MIPS_CP2DR14:	return mipscpu.cp2dr[ 14 ].d;
	case MIPS_CP2DR15:	return mipscpu.cp2dr[ 15 ].d;
	case MIPS_CP2DR16:	return mipscpu.cp2dr[ 16 ].d;
	case MIPS_CP2DR17:	return mipscpu.cp2dr[ 17 ].d;
	case MIPS_CP2DR18:	return mipscpu.cp2dr[ 18 ].d;
	case MIPS_CP2DR19:	return mipscpu.cp2dr[ 19 ].d;
	case MIPS_CP2DR20:	return mipscpu.cp2dr[ 20 ].d;
	case MIPS_CP2DR21:	return mipscpu.cp2dr[ 21 ].d;
	case MIPS_CP2DR22:	return mipscpu.cp2dr[ 22 ].d;
	case MIPS_CP2DR23:	return mipscpu.cp2dr[ 23 ].d;
	case MIPS_CP2DR24:	return mipscpu.cp2dr[ 24 ].d;
	case MIPS_CP2DR25:	return mipscpu.cp2dr[ 25 ].d;
	case MIPS_CP2DR26:	return mipscpu.cp2dr[ 26 ].d;
	case MIPS_CP2DR27:	return mipscpu.cp2dr[ 27 ].d;
	case MIPS_CP2DR28:	return mipscpu.cp2dr[ 28 ].d;
	case MIPS_CP2DR29:	return mipscpu.cp2dr[ 29 ].d;
	case MIPS_CP2DR30:	return mipscpu.cp2dr[ 30 ].d;
	case MIPS_CP2DR31:	return mipscpu.cp2dr[ 31 ].d;
	case MIPS_CP2CR0:	return mipscpu.cp2cr[ 0 ].d;
	case MIPS_CP2CR1:	return mipscpu.cp2cr[ 1 ].d;
	case MIPS_CP2CR2:	return mipscpu.cp2cr[ 2 ].d;
	case MIPS_CP2CR3:	return mipscpu.cp2cr[ 3 ].d;
	case MIPS_CP2CR4:	return mipscpu.cp2cr[ 4 ].d;
	case MIPS_CP2CR5:	return mipscpu.cp2cr[ 5 ].d;
	case MIPS_CP2CR6:	return mipscpu.cp2cr[ 6 ].d;
	case MIPS_CP2CR7:	return mipscpu.cp2cr[ 7 ].d;
	case MIPS_CP2CR8:	return mipscpu.cp2cr[ 8 ].d;
	case MIPS_CP2CR9:	return mipscpu.cp2cr[ 9 ].d;
	case MIPS_CP2CR10:	return mipscpu.cp2cr[ 10 ].d;
	case MIPS_CP2CR11:	return mipscpu.cp2cr[ 11 ].d;
	case MIPS_CP2CR12:	return mipscpu.cp2cr[ 12 ].d;
	case MIPS_CP2CR13:	return mipscpu.cp2cr[ 13 ].d;
	case MIPS_CP2CR14:	return mipscpu.cp2cr[ 14 ].d;
	case MIPS_CP2CR15:	return mipscpu.cp2cr[ 15 ].d;
	case MIPS_CP2CR16:	return mipscpu.cp2cr[ 16 ].d;
	case MIPS_CP2CR17:	return mipscpu.cp2cr[ 17 ].d;
	case MIPS_CP2CR18:	return mipscpu.cp2cr[ 18 ].d;
	case MIPS_CP2CR19:	return mipscpu.cp2cr[ 19 ].d;
	case MIPS_CP2CR20:	return mipscpu.cp2cr[ 20 ].d;
	case MIPS_CP2CR21:	return mipscpu.cp2cr[ 21 ].d;
	case MIPS_CP2CR22:	return mipscpu.cp2cr[ 22 ].d;
	case MIPS_CP2CR23:	return mipscpu.cp2cr[ 23 ].d;
	case MIPS_CP2CR24:	return mipscpu.cp2cr[ 24 ].d;
	case MIPS_CP2CR25:	return mipscpu.cp2cr[ 25 ].d;
	case MIPS_CP2CR26:	return mipscpu.cp2cr[ 26 ].d;
	case MIPS_CP2CR27:	return mipscpu.cp2cr[ 27 ].d;
	case MIPS_CP2CR28:	return mipscpu.cp2cr[ 28 ].d;
	case MIPS_CP2CR29:	return mipscpu.cp2cr[ 29 ].d;
	case MIPS_CP2CR30:	return mipscpu.cp2cr[ 30 ].d;
	case MIPS_CP2CR31:	return mipscpu.cp2cr[ 31 ].d;
	}
	return 0;
}

void mips_set_reg( int regnum, unsigned val )
{
	switch( regnum )
	{
	case REG_PC:		mips_set_pc( val );				break;
	case MIPS_PC:		mips_set_pc( val );				break;
	case REG_SP:		/* no stack */					break;
	case MIPS_DELAYPC:	mipscpu.delaypc = val;			break;
	case MIPS_DELAY:	mipscpu.delay = val & 1;		break;
	case MIPS_HI:		mipscpu.hi = val;				break;
	case MIPS_LO:		mipscpu.lo = val;				break;
	case MIPS_R0:		mipscpu.r[ 0 ] = val;			break;
	case MIPS_R1:		mipscpu.r[ 1 ] = val;			break;
	case MIPS_R2:		mipscpu.r[ 2 ] = val;			break;
	case MIPS_R3:		mipscpu.r[ 3 ] = val;			break;
	case MIPS_R4:		mipscpu.r[ 4 ] = val;			break;
	case MIPS_R5:		mipscpu.r[ 5 ] = val;			break;
	case MIPS_R6:		mipscpu.r[ 6 ] = val;			break;
	case MIPS_R7:		mipscpu.r[ 7 ] = val;			break;
	case MIPS_R8:		mipscpu.r[ 8 ] = val;			break;
	case MIPS_R9:		mipscpu.r[ 9 ] = val;			break;
	case MIPS_R10:		mipscpu.r[ 10 ] = val;			break;
	case MIPS_R11:		mipscpu.r[ 11 ] = val;			break;
	case MIPS_R12:		mipscpu.r[ 12 ] = val;			break;
	case MIPS_R13:		mipscpu.r[ 13 ] = val;			break;
	case MIPS_R14:		mipscpu.r[ 14 ] = val;			break;
	case MIPS_R15:		mipscpu.r[ 15 ] = val;			break;
	case MIPS_R16:		mipscpu.r[ 16 ] = val;			break;
	case MIPS_R17:		mipscpu.r[ 17 ] = val;			break;
	case MIPS_R18:		mipscpu.r[ 18 ] = val;			break;
	case MIPS_R19:		mipscpu.r[ 19 ] = val;			break;
	case MIPS_R20:		mipscpu.r[ 20 ] = val;			break;
	case MIPS_R21:		mipscpu.r[ 21 ] = val;			break;
	case MIPS_R22:		mipscpu.r[ 22 ] = val;			break;
	case MIPS_R23:		mipscpu.r[ 23 ] = val;			break;
	case MIPS_R24:		mipscpu.r[ 24 ] = val;			break;
	case MIPS_R25:		mipscpu.r[ 25 ] = val;			break;
	case MIPS_R26:		mipscpu.r[ 26 ] = val;			break;
	case MIPS_R27:		mipscpu.r[ 27 ] = val;			break;
	case MIPS_R28:		mipscpu.r[ 28 ] = val;			break;
	case MIPS_R29:		mipscpu.r[ 29 ] = val;			break;
	case MIPS_R30:		mipscpu.r[ 30 ] = val;			break;
	case MIPS_R31:		mipscpu.r[ 31 ] = val;			break;
	case MIPS_CP0R0:	mips_set_cp0r( 0, val );		break;
	case MIPS_CP0R1:	mips_set_cp0r( 1, val );		break;
	case MIPS_CP0R2:	mips_set_cp0r( 2, val );		break;
	case MIPS_CP0R3:	mips_set_cp0r( 3, val );		break;
	case MIPS_CP0R4:	mips_set_cp0r( 4, val );		break;
	case MIPS_CP0R5:	mips_set_cp0r( 5, val );		break;
	case MIPS_CP0R6:	mips_set_cp0r( 6, val );		break;
	case MIPS_CP0R7:	mips_set_cp0r( 7, val );		break;
	case MIPS_CP0R8:	mips_set_cp0r( 8, val );		break;
	case MIPS_CP0R9:	mips_set_cp0r( 9, val );		break;
	case MIPS_CP0R10:	mips_set_cp0r( 10, val );		break;
	case MIPS_CP0R11:	mips_set_cp0r( 11, val );		break;
	case MIPS_CP0R12:	mips_set_cp0r( 12, val );		break;
	case MIPS_CP0R13:	mips_set_cp0r( 13, val );		break;
	case MIPS_CP0R14:	mips_set_cp0r( 14, val );		break;
	case MIPS_CP0R15:	mips_set_cp0r( 15, val );		break;
	case MIPS_CP0R16:	mips_set_cp0r( 16, val );		break;
	case MIPS_CP0R17:	mips_set_cp0r( 17, val );		break;
	case MIPS_CP0R18:	mips_set_cp0r( 18, val );		break;
	case MIPS_CP0R19:	mips_set_cp0r( 19, val );		break;
	case MIPS_CP0R20:	mips_set_cp0r( 20, val );		break;
	case MIPS_CP0R21:	mips_set_cp0r( 21, val );		break;
	case MIPS_CP0R22:	mips_set_cp0r( 22, val );		break;
	case MIPS_CP0R23:	mips_set_cp0r( 23, val );		break;
	case MIPS_CP0R24:	mips_set_cp0r( 24, val );		break;
	case MIPS_CP0R25:	mips_set_cp0r( 25, val );		break;
	case MIPS_CP0R26:	mips_set_cp0r( 26, val );		break;
	case MIPS_CP0R27:	mips_set_cp0r( 27, val );		break;
	case MIPS_CP0R28:	mips_set_cp0r( 28, val );		break;
	case MIPS_CP0R29:	mips_set_cp0r( 29, val );		break;
	case MIPS_CP0R30:	mips_set_cp0r( 30, val );		break;
	case MIPS_CP0R31:	mips_set_cp0r( 31, val );		break;
	case MIPS_CP2DR0:	mipscpu.cp2dr[ 0 ].d = val;		break;
	case MIPS_CP2DR1:	mipscpu.cp2dr[ 1 ].d = val;		break;
	case MIPS_CP2DR2:	mipscpu.cp2dr[ 2 ].d = val;		break;
	case MIPS_CP2DR3:	mipscpu.cp2dr[ 3 ].d = val;		break;
	case MIPS_CP2DR4:	mipscpu.cp2dr[ 4 ].d = val;		break;
	case MIPS_CP2DR5:	mipscpu.cp2dr[ 5 ].d = val;		break;
	case MIPS_CP2DR6:	mipscpu.cp2dr[ 6 ].d = val;		break;
	case MIPS_CP2DR7:	mipscpu.cp2dr[ 7 ].d = val;		break;
	case MIPS_CP2DR8:	mipscpu.cp2dr[ 8 ].d = val;		break;
	case MIPS_CP2DR9:	mipscpu.cp2dr[ 9 ].d = val;		break;
	case MIPS_CP2DR10:	mipscpu.cp2dr[ 10 ].d = val;	break;
	case MIPS_CP2DR11:	mipscpu.cp2dr[ 11 ].d = val;	break;
	case MIPS_CP2DR12:	mipscpu.cp2dr[ 12 ].d = val;	break;
	case MIPS_CP2DR13:	mipscpu.cp2dr[ 13 ].d = val;	break;
	case MIPS_CP2DR14:	mipscpu.cp2dr[ 14 ].d = val;	break;
	case MIPS_CP2DR15:	mipscpu.cp2dr[ 15 ].d = val;	break;
	case MIPS_CP2DR16:	mipscpu.cp2dr[ 16 ].d = val;	break;
	case MIPS_CP2DR17:	mipscpu.cp2dr[ 17 ].d = val;	break;
	case MIPS_CP2DR18:	mipscpu.cp2dr[ 18 ].d = val;	break;
	case MIPS_CP2DR19:	mipscpu.cp2dr[ 19 ].d = val;	break;
	case MIPS_CP2DR20:	mipscpu.cp2dr[ 20 ].d = val;	break;
	case MIPS_CP2DR21:	mipscpu.cp2dr[ 21 ].d = val;	break;
	case MIPS_CP2DR22:	mipscpu.cp2dr[ 22 ].d = val;	break;
	case MIPS_CP2DR23:	mipscpu.cp2dr[ 23 ].d = val;	break;
	case MIPS_CP2DR24:	mipscpu.cp2dr[ 24 ].d = val;	break;
	case MIPS_CP2DR25:	mipscpu.cp2dr[ 25 ].d = val;	break;
	case MIPS_CP2DR26:	mipscpu.cp2dr[ 26 ].d = val;	break;
	case MIPS_CP2DR27:	mipscpu.cp2dr[ 27 ].d = val;	break;
	case MIPS_CP2DR28:	mipscpu.cp2dr[ 28 ].d = val;	break;
	case MIPS_CP2DR29:	mipscpu.cp2dr[ 29 ].d = val;	break;
	case MIPS_CP2DR30:	mipscpu.cp2dr[ 30 ].d = val;	break;
	case MIPS_CP2DR31:	mipscpu.cp2dr[ 31 ].d = val;	break;
	case MIPS_CP2CR0:	mipscpu.cp2cr[ 0 ].d = val;		break;
	case MIPS_CP2CR1:	mipscpu.cp2cr[ 1 ].d = val;		break;
	case MIPS_CP2CR2:	mipscpu.cp2cr[ 2 ].d = val;		break;
	case MIPS_CP2CR3:	mipscpu.cp2cr[ 3 ].d = val;		break;
	case MIPS_CP2CR4:	mipscpu.cp2cr[ 4 ].d = val;		break;
	case MIPS_CP2CR5:	mipscpu.cp2cr[ 5 ].d = val;		break;
	case MIPS_CP2CR6:	mipscpu.cp2cr[ 6 ].d = val;		break;
	case MIPS_CP2CR7:	mipscpu.cp2cr[ 7 ].d = val;		break;
	case MIPS_CP2CR8:	mipscpu.cp2cr[ 8 ].d = val;		break;
	case MIPS_CP2CR9:	mipscpu.cp2cr[ 9 ].d = val;		break;
	case MIPS_CP2CR10:	mipscpu.cp2cr[ 10 ].d = val;	break;
	case MIPS_CP2CR11:	mipscpu.cp2cr[ 11 ].d = val;	break;
	case MIPS_CP2CR12:	mipscpu.cp2cr[ 12 ].d = val;	break;
	case MIPS_CP2CR13:	mipscpu.cp2cr[ 13 ].d = val;	break;
	case MIPS_CP2CR14:	mipscpu.cp2cr[ 14 ].d = val;	break;
	case MIPS_CP2CR15:	mipscpu.cp2cr[ 15 ].d = val;	break;
	case MIPS_CP2CR16:	mipscpu.cp2cr[ 16 ].d = val;	break;
	case MIPS_CP2CR17:	mipscpu.cp2cr[ 17 ].d = val;	break;
	case MIPS_CP2CR18:	mipscpu.cp2cr[ 18 ].d = val;	break;
	case MIPS_CP2CR19:	mipscpu.cp2cr[ 19 ].d = val;	break;
	case MIPS_CP2CR20:	mipscpu.cp2cr[ 20 ].d = val;	break;
	case MIPS_CP2CR21:	mipscpu.cp2cr[ 21 ].d = val;	break;
	case MIPS_CP2CR22:	mipscpu.cp2cr[ 22 ].d = val;	break;
	case MIPS_CP2CR23:	mipscpu.cp2cr[ 23 ].d = val;	break;
	case MIPS_CP2CR24:	mipscpu.cp2cr[ 24 ].d = val;	break;
	case MIPS_CP2CR25:	mipscpu.cp2cr[ 25 ].d = val;	break;
	case MIPS_CP2CR26:	mipscpu.cp2cr[ 26 ].d = val;	break;
	case MIPS_CP2CR27:	mipscpu.cp2cr[ 27 ].d = val;	break;
	case MIPS_CP2CR28:	mipscpu.cp2cr[ 28 ].d = val;	break;
	case MIPS_CP2CR29:	mipscpu.cp2cr[ 29 ].d = val;	break;
	case MIPS_CP2CR30:	mipscpu.cp2cr[ 30 ].d = val;	break;
	case MIPS_CP2CR31:	mipscpu.cp2cr[ 31 ].d = val;	break;
	}
}

void mips_set_nmi_line( int state )
{
	/* no nmi */
}

void mips_set_irq_line( int irqline, int state )
{
	UINT32 ip;

	switch( irqline )
	{
	case 0:
		ip = CAUSE_IP2;
		break;
	case 1:
		ip = CAUSE_IP3;
		break;
	case 2:
		ip = CAUSE_IP4;
		break;
	case 3:
		ip = CAUSE_IP5;
		break;
	case 4:
		ip = CAUSE_IP6;
		break;
	case 5:
		ip = CAUSE_IP7;
		break;
	default:
		return;
	}

	switch( state )
	{
	case CLEAR_LINE:
		mips_set_cp0r( CP0_CAUSE, mipscpu.cp0r[ CP0_CAUSE ] & ~ip );
		break;
	case ASSERT_LINE:
		mips_set_cp0r( CP0_CAUSE, mipscpu.cp0r[ CP0_CAUSE ] |= ip );
		if( mipscpu.irq_callback )
		{
			/* HOLD_LINE interrupts are not supported by the architecture.
			By acknowledging the interupt here they are treated like PULSE_LINE
			interrupts, so if the interrupt isn't enabled it will be ignored.
			There is also a problem with PULSE_LINE interrupts as the interrupt
			pending bits aren't latched the emulated code won't know what caused
			the interrupt. */
			(*mipscpu.irq_callback)( irqline );
		}
		break;
	}
}

void mips_set_irq_callback( int (*callback)(int irqline) )
{
	mipscpu.irq_callback = callback;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/

const char *mips_info( void *context, int regnum )
{
	static char buffer[ 64 ][ 47 + 1 ];
	static int which = 0;
	mips_cpu_context *r = context;

	which++;
	which %= 64;
	buffer[ which ][ 0 ] = '\0';
	if( !context )
	{
		static mips_cpu_context tmp;
		mips_get_context( &tmp );
		r = &tmp;
	}

	regnum = (int)(unsigned char)regnum; /* kludge */

	switch( regnum )
	{
	case CPU_INFO_REG + MIPS_PC:		sprintf( buffer[ which ], "pc      :%08x", r->pc );				break;
	case CPU_INFO_REG + MIPS_DELAYPC:	sprintf( buffer[ which ], "delay pc:%08x", r->delaypc );		break;
	case CPU_INFO_REG + MIPS_DELAY:		sprintf( buffer[ which ], "delay   :%01x", r->delay );			break;
	case CPU_INFO_REG + MIPS_HI:		sprintf( buffer[ which ], "hi      :%08x", r->hi );				break;
	case CPU_INFO_REG + MIPS_LO:		sprintf( buffer[ which ], "lo      :%08x", r->lo );				break;
	case CPU_INFO_REG + MIPS_R0:		sprintf( buffer[ which ], "zero    :%08x", r->r[ 0 ] );			break;
	case CPU_INFO_REG + MIPS_R1:		sprintf( buffer[ which ], "at      :%08x", r->r[ 1 ] );			break;
	case CPU_INFO_REG + MIPS_R2:		sprintf( buffer[ which ], "v0      :%08x", r->r[ 2 ] );			break;
	case CPU_INFO_REG + MIPS_R3:		sprintf( buffer[ which ], "v1      :%08x", r->r[ 3 ] );			break;
	case CPU_INFO_REG + MIPS_R4:		sprintf( buffer[ which ], "a0      :%08x", r->r[ 4 ] );			break;
	case CPU_INFO_REG + MIPS_R5:		sprintf( buffer[ which ], "a1      :%08x", r->r[ 5 ] );			break;
	case CPU_INFO_REG + MIPS_R6:		sprintf( buffer[ which ], "a2      :%08x", r->r[ 6 ] );			break;
	case CPU_INFO_REG + MIPS_R7:		sprintf( buffer[ which ], "a3      :%08x", r->r[ 7 ] );			break;
	case CPU_INFO_REG + MIPS_R8:		sprintf( buffer[ which ], "t0      :%08x", r->r[ 8 ] );			break;
	case CPU_INFO_REG + MIPS_R9:		sprintf( buffer[ which ], "t1      :%08x", r->r[ 9 ] );			break;
	case CPU_INFO_REG + MIPS_R10:		sprintf( buffer[ which ], "t2      :%08x", r->r[ 10 ] );		break;
	case CPU_INFO_REG + MIPS_R11:		sprintf( buffer[ which ], "t3      :%08x", r->r[ 11 ] );		break;
	case CPU_INFO_REG + MIPS_R12:		sprintf( buffer[ which ], "t4      :%08x", r->r[ 12 ] );		break;
	case CPU_INFO_REG + MIPS_R13:		sprintf( buffer[ which ], "t5      :%08x", r->r[ 13 ] );		break;
	case CPU_INFO_REG + MIPS_R14:		sprintf( buffer[ which ], "t6      :%08x", r->r[ 14 ] );		break;
	case CPU_INFO_REG + MIPS_R15:		sprintf( buffer[ which ], "t7      :%08x", r->r[ 15 ] );		break;
	case CPU_INFO_REG + MIPS_R16:		sprintf( buffer[ which ], "s0      :%08x", r->r[ 16 ] );		break;
	case CPU_INFO_REG + MIPS_R17:		sprintf( buffer[ which ], "s1      :%08x", r->r[ 17 ] );		break;
	case CPU_INFO_REG + MIPS_R18:		sprintf( buffer[ which ], "s2      :%08x", r->r[ 18 ] );		break;
	case CPU_INFO_REG + MIPS_R19:		sprintf( buffer[ which ], "s3      :%08x", r->r[ 19 ] );		break;
	case CPU_INFO_REG + MIPS_R20:		sprintf( buffer[ which ], "s4      :%08x", r->r[ 20 ] );		break;
	case CPU_INFO_REG + MIPS_R21:		sprintf( buffer[ which ], "s5      :%08x", r->r[ 21 ] );		break;
	case CPU_INFO_REG + MIPS_R22:		sprintf( buffer[ which ], "s6      :%08x", r->r[ 22 ] );		break;
	case CPU_INFO_REG + MIPS_R23:		sprintf( buffer[ which ], "s7      :%08x", r->r[ 23 ] );		break;
	case CPU_INFO_REG + MIPS_R24:		sprintf( buffer[ which ], "t8      :%08x", r->r[ 24 ] );		break;
	case CPU_INFO_REG + MIPS_R25:		sprintf( buffer[ which ], "t9      :%08x", r->r[ 25 ] );		break;
	case CPU_INFO_REG + MIPS_R26:		sprintf( buffer[ which ], "k0      :%08x", r->r[ 26 ] );		break;
	case CPU_INFO_REG + MIPS_R27:		sprintf( buffer[ which ], "k1      :%08x", r->r[ 27 ] );		break;
	case CPU_INFO_REG + MIPS_R28:		sprintf( buffer[ which ], "gp      :%08x", r->r[ 28 ] );		break;
	case CPU_INFO_REG + MIPS_R29:		sprintf( buffer[ which ], "sp      :%08x", r->r[ 29 ] );		break;
	case CPU_INFO_REG + MIPS_R30:		sprintf( buffer[ which ], "fp      :%08x", r->r[ 30 ] );		break;
	case CPU_INFO_REG + MIPS_R31:		sprintf( buffer[ which ], "ra      :%08x", r->r[ 31 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R0:		sprintf( buffer[ which ], "Index   :%08x", r->cp0r[ 0 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R1:		sprintf( buffer[ which ], "Random  :%08x", r->cp0r[ 1 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R2:		sprintf( buffer[ which ], "EntryLo :%08x", r->cp0r[ 2 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R3:		sprintf( buffer[ which ], "cp0r3   :%08x", r->cp0r[ 3 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R4:		sprintf( buffer[ which ], "Context :%08x", r->cp0r[ 4 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R5:		sprintf( buffer[ which ], "cp0r5   :%08x", r->cp0r[ 5 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R6:		sprintf( buffer[ which ], "cp0r6   :%08x", r->cp0r[ 6 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R7:		sprintf( buffer[ which ], "cp0r7   :%08x", r->cp0r[ 7 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R8:		sprintf( buffer[ which ], "BadVAddr:%08x", r->cp0r[ 8 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R9:		sprintf( buffer[ which ], "cp0r9   :%08x", r->cp0r[ 9 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R10:	sprintf( buffer[ which ], "EntryHi :%08x", r->cp0r[ 10 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R11:	sprintf( buffer[ which ], "cp0r11  :%08x", r->cp0r[ 11 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R12:	sprintf( buffer[ which ], "SR      :%08x", r->cp0r[ 12 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R13:	sprintf( buffer[ which ], "Cause   :%08x", r->cp0r[ 13 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R14:	sprintf( buffer[ which ], "EPC     :%08x", r->cp0r[ 14 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R15:	sprintf( buffer[ which ], "PRId    :%08x", r->cp0r[ 15 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R16:	sprintf( buffer[ which ], "cp0r16  :%08x", r->cp0r[ 16 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R17:	sprintf( buffer[ which ], "cp0r17  :%08x", r->cp0r[ 17 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R18:	sprintf( buffer[ which ], "cp0r18  :%08x", r->cp0r[ 18 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R19:	sprintf( buffer[ which ], "cp0r19  :%08x", r->cp0r[ 19 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R20:	sprintf( buffer[ which ], "cp0r20  :%08x", r->cp0r[ 20 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R21:	sprintf( buffer[ which ], "cp0r21  :%08x", r->cp0r[ 21 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R22:	sprintf( buffer[ which ], "cp0r22  :%08x", r->cp0r[ 22 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R23:	sprintf( buffer[ which ], "cp0r23  :%08x", r->cp0r[ 23 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R24:	sprintf( buffer[ which ], "cp0r24  :%08x", r->cp0r[ 24 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R25:	sprintf( buffer[ which ], "cp0r25  :%08x", r->cp0r[ 25 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R26:	sprintf( buffer[ which ], "cp0r26  :%08x", r->cp0r[ 26 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R27:	sprintf( buffer[ which ], "cp0r27  :%08x", r->cp0r[ 27 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R28:	sprintf( buffer[ which ], "cp0r28  :%08x", r->cp0r[ 28 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R29:	sprintf( buffer[ which ], "cp0r29  :%08x", r->cp0r[ 29 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R30:	sprintf( buffer[ which ], "cp0r30  :%08x", r->cp0r[ 30 ] );		break;
	case CPU_INFO_REG + MIPS_CP0R31:	sprintf( buffer[ which ], "cp0r31  :%08x", r->cp0r[ 31 ] );		break;
	case CPU_INFO_REG + MIPS_CP2DR0:	sprintf( buffer[ which ], "vxy0    :%08x", r->cp2dr[ 0 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR1:	sprintf( buffer[ which ], "vz0     :%08x", r->cp2dr[ 1 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR2:	sprintf( buffer[ which ], "vxy1    :%08x", r->cp2dr[ 2 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR3:	sprintf( buffer[ which ], "vz1     :%08x", r->cp2dr[ 3 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR4:	sprintf( buffer[ which ], "vxy2    :%08x", r->cp2dr[ 4 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR5:	sprintf( buffer[ which ], "vz2     :%08x", r->cp2dr[ 5 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR6:	sprintf( buffer[ which ], "rgb     :%08x", r->cp2dr[ 6 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR7:	sprintf( buffer[ which ], "otz     :%08x", r->cp2dr[ 7 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR8:	sprintf( buffer[ which ], "ir0     :%08x", r->cp2dr[ 8 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR9:	sprintf( buffer[ which ], "ir1     :%08x", r->cp2dr[ 9 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR10:	sprintf( buffer[ which ], "ir2     :%08x", r->cp2dr[ 10 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR11:	sprintf( buffer[ which ], "ir3     :%08x", r->cp2dr[ 11 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR12:	sprintf( buffer[ which ], "sxy0    :%08x", r->cp2dr[ 12 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR13:	sprintf( buffer[ which ], "sxy1    :%08x", r->cp2dr[ 13 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR14:	sprintf( buffer[ which ], "sxy2    :%08x", r->cp2dr[ 14 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR15:	sprintf( buffer[ which ], "sxyp    :%08x", r->cp2dr[ 15 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR16:	sprintf( buffer[ which ], "sz0     :%08x", r->cp2dr[ 16 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR17:	sprintf( buffer[ which ], "sz1     :%08x", r->cp2dr[ 17 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR18:	sprintf( buffer[ which ], "sz2     :%08x", r->cp2dr[ 18 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR19:	sprintf( buffer[ which ], "sz3     :%08x", r->cp2dr[ 19 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR20:	sprintf( buffer[ which ], "rgb0    :%08x", r->cp2dr[ 20 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR21:	sprintf( buffer[ which ], "rgb1    :%08x", r->cp2dr[ 21 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR22:	sprintf( buffer[ which ], "rgb2    :%08x", r->cp2dr[ 22 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR23:	sprintf( buffer[ which ], "res1    :%08x", r->cp2dr[ 23 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR24:	sprintf( buffer[ which ], "mac0    :%08x", r->cp2dr[ 24 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR25:	sprintf( buffer[ which ], "mac1    :%08x", r->cp2dr[ 25 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR26:	sprintf( buffer[ which ], "mac2    :%08x", r->cp2dr[ 26 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR27:	sprintf( buffer[ which ], "mac3    :%08x", r->cp2dr[ 27 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR28:	sprintf( buffer[ which ], "irgb    :%08x", r->cp2dr[ 28 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR29:	sprintf( buffer[ which ], "orgb    :%08x", r->cp2dr[ 29 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR30:	sprintf( buffer[ which ], "lzcs    :%08x", r->cp2dr[ 30 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2DR31:	sprintf( buffer[ which ], "lzcr    :%08x", r->cp2dr[ 31 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR0:	sprintf( buffer[ which ], "r11r12  :%08x", r->cp2cr[ 0 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR1:	sprintf( buffer[ which ], "r13r21  :%08x", r->cp2cr[ 1 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR2:	sprintf( buffer[ which ], "r22r23  :%08x", r->cp2cr[ 2 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR3:	sprintf( buffer[ which ], "r31r32  :%08x", r->cp2cr[ 3 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR4:	sprintf( buffer[ which ], "r33     :%08x", r->cp2cr[ 4 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR5:	sprintf( buffer[ which ], "trx     :%08x", r->cp2cr[ 5 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR6:	sprintf( buffer[ which ], "try     :%08x", r->cp2cr[ 6 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR7:	sprintf( buffer[ which ], "trz     :%08x", r->cp2cr[ 7 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR8:	sprintf( buffer[ which ], "l11l12  :%08x", r->cp2cr[ 8 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR9:	sprintf( buffer[ which ], "l13l21  :%08x", r->cp2cr[ 9 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR10:	sprintf( buffer[ which ], "l22l23  :%08x", r->cp2cr[ 10 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR11:	sprintf( buffer[ which ], "l31l32  :%08x", r->cp2cr[ 11 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR12:	sprintf( buffer[ which ], "l33     :%08x", r->cp2cr[ 12 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR13:	sprintf( buffer[ which ], "rbk     :%08x", r->cp2cr[ 13 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR14:	sprintf( buffer[ which ], "gbk     :%08x", r->cp2cr[ 14 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR15:	sprintf( buffer[ which ], "bbk     :%08x", r->cp2cr[ 15 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR16:	sprintf( buffer[ which ], "lr1lr2  :%08x", r->cp2cr[ 16 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR17:	sprintf( buffer[ which ], "lr31g1  :%08x", r->cp2cr[ 17 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR18:	sprintf( buffer[ which ], "lg2lg3  :%08x", r->cp2cr[ 18 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR19:	sprintf( buffer[ which ], "lb1lb2  :%08x", r->cp2cr[ 19 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR20:	sprintf( buffer[ which ], "lb3     :%08x", r->cp2cr[ 20 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR21:	sprintf( buffer[ which ], "rfc     :%08x", r->cp2cr[ 21 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR22:	sprintf( buffer[ which ], "gfc     :%08x", r->cp2cr[ 22 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR23:	sprintf( buffer[ which ], "bfc     :%08x", r->cp2cr[ 23 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR24:	sprintf( buffer[ which ], "ofx     :%08x", r->cp2cr[ 24 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR25:	sprintf( buffer[ which ], "ofy     :%08x", r->cp2cr[ 25 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR26:	sprintf( buffer[ which ], "h       :%08x", r->cp2cr[ 26 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR27:	sprintf( buffer[ which ], "dqa     :%08x", r->cp2cr[ 27 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR28:	sprintf( buffer[ which ], "dqb     :%08x", r->cp2cr[ 28 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR29:	sprintf( buffer[ which ], "zsf3    :%08x", r->cp2cr[ 29 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR30:	sprintf( buffer[ which ], "zsf4    :%08x", r->cp2cr[ 30 ].d );	break;
	case CPU_INFO_REG + MIPS_CP2CR31:	sprintf( buffer[ which ], "flag    :%08x", r->cp2cr[ 31 ].d );	break;
	case CPU_INFO_FLAGS:		return "";
	case CPU_INFO_NAME:			return "PSX CPU";
	case CPU_INFO_FAMILY:		return "mipscpu";
	case CPU_INFO_VERSION:		return "1.3";
	case CPU_INFO_FILE:			return __FILE__;
	case CPU_INFO_CREDITS:		return "Copyright 2003 smf";
	case CPU_INFO_REG_LAYOUT:	return (const char*)mips_reg_layout;
	case CPU_INFO_WIN_LAYOUT:	return (const char*)mips_win_layout;
	}
	return buffer[ which ];
}

unsigned mips_dasm( char *buffer, UINT32 pc )
{
	unsigned ret;
	change_pc32ledw( pc );
#ifdef MAME_DEBUG
	ret = DasmMIPS( buffer, pc );
#else
	sprintf( buffer, "$%08x", cpu_readop32( pc ) );
	ret = 4;
#endif
	change_pc32lew( mipscpu.pc );
	return ret;
}

/* preliminary gte code */

#define VXY0 ( mipscpu.cp2dr[ 0 ].d )
#define VX0  ( mipscpu.cp2dr[ 0 ].w.l )
#define VY0  ( mipscpu.cp2dr[ 0 ].w.h )
#define VZ0	 ( mipscpu.cp2dr[ 1 ].w.l )
#define VXY1 ( mipscpu.cp2dr[ 2 ].d )
#define VX1  ( mipscpu.cp2dr[ 2 ].w.l )
#define VY1  ( mipscpu.cp2dr[ 2 ].w.h )
#define VZ1  ( mipscpu.cp2dr[ 3 ].w.l )
#define VXY2 ( mipscpu.cp2dr[ 4 ].d )
#define VX2  ( mipscpu.cp2dr[ 4 ].w.l )
#define VY2  ( mipscpu.cp2dr[ 4 ].w.h )
#define VZ2  ( mipscpu.cp2dr[ 5 ].w.l )
#define RGB  ( mipscpu.cp2dr[ 6 ].d )
#define R    ( mipscpu.cp2dr[ 6 ].b.l )
#define G    ( mipscpu.cp2dr[ 6 ].b.h )
#define B    ( mipscpu.cp2dr[ 6 ].b.h2 )
#define CODE ( mipscpu.cp2dr[ 6 ].b.h3 )
#define OTZ  ( mipscpu.cp2dr[ 7 ].w.l )
#define IR0  ( mipscpu.cp2dr[ 8 ].d )
#define IR1  ( mipscpu.cp2dr[ 9 ].d )
#define IR2  ( mipscpu.cp2dr[ 10 ].d )
#define IR3  ( mipscpu.cp2dr[ 11 ].d )
#define SXY0 ( mipscpu.cp2dr[ 12 ].d )
#define SX0  ( mipscpu.cp2dr[ 12 ].w.l )
#define SY0  ( mipscpu.cp2dr[ 12 ].w.h )
#define SXY1 ( mipscpu.cp2dr[ 13 ].d )
#define SX1  ( mipscpu.cp2dr[ 13 ].w.l )
#define SY1  ( mipscpu.cp2dr[ 13 ].w.h )
#define SXY2 ( mipscpu.cp2dr[ 14 ].d )
#define SX2  ( mipscpu.cp2dr[ 14 ].w.l )
#define SY2  ( mipscpu.cp2dr[ 14 ].w.h )
#define SXYP ( mipscpu.cp2dr[ 15 ].d )
#define SXP  ( mipscpu.cp2dr[ 15 ].w.l )
#define SYP  ( mipscpu.cp2dr[ 15 ].w.h )
#define SZ0  ( mipscpu.cp2dr[ 16 ].w.l )
#define SZ1  ( mipscpu.cp2dr[ 17 ].w.l )
#define SZ2  ( mipscpu.cp2dr[ 18 ].w.l )
#define SZ3  ( mipscpu.cp2dr[ 19 ].w.l )
#define RGB0 ( mipscpu.cp2dr[ 20 ].d )
#define R0   ( mipscpu.cp2dr[ 20 ].b.l )
#define G0   ( mipscpu.cp2dr[ 20 ].b.h )
#define B0   ( mipscpu.cp2dr[ 20 ].b.h2 )
#define CD0  ( mipscpu.cp2dr[ 20 ].b.h3 )
#define RGB1 ( mipscpu.cp2dr[ 21 ].d )
#define R1   ( mipscpu.cp2dr[ 21 ].b.l )
#define G1   ( mipscpu.cp2dr[ 21 ].b.h )
#define B1   ( mipscpu.cp2dr[ 21 ].b.h2 )
#define CD1  ( mipscpu.cp2dr[ 21 ].b.h3 )
#define RGB2 ( mipscpu.cp2dr[ 22 ].d )
#define R2   ( mipscpu.cp2dr[ 22 ].b.l )
#define G2   ( mipscpu.cp2dr[ 22 ].b.h )
#define B2   ( mipscpu.cp2dr[ 22 ].b.h2 )
#define CD2  ( mipscpu.cp2dr[ 22 ].b.h3 )
#define RES1 ( mipscpu.cp2dr[ 23 ].d )
#define MAC0 ( mipscpu.cp2dr[ 24 ].d )
#define MAC1 ( mipscpu.cp2dr[ 25 ].d )
#define MAC2 ( mipscpu.cp2dr[ 26 ].d )
#define MAC3 ( mipscpu.cp2dr[ 27 ].d )
#define IRGB ( mipscpu.cp2dr[ 28 ].d )
#define ORGB ( mipscpu.cp2dr[ 29 ].d )
#define LZCS ( mipscpu.cp2dr[ 30 ].d )
#define LZCR ( mipscpu.cp2dr[ 31 ].d )

#define D1  ( mipscpu.cp2cr[ 0 ].d )
#define R11 ( mipscpu.cp2cr[ 0 ].w.l )
#define R12 ( mipscpu.cp2cr[ 0 ].w.h )
#define R13 ( mipscpu.cp2cr[ 1 ].w.l )
#define R21 ( mipscpu.cp2cr[ 1 ].w.h )
#define D2  ( mipscpu.cp2cr[ 2 ].d )
#define R22 ( mipscpu.cp2cr[ 2 ].w.l )
#define R23 ( mipscpu.cp2cr[ 2 ].w.h )
#define R31 ( mipscpu.cp2cr[ 3 ].w.l )
#define R32 ( mipscpu.cp2cr[ 3 ].w.h )
#define D3  ( mipscpu.cp2cr[ 4 ].d )
#define R33 ( mipscpu.cp2cr[ 4 ].w.l )
#define TRX ( mipscpu.cp2cr[ 5 ].d )
#define TRY ( mipscpu.cp2cr[ 6 ].d )
#define TRZ ( mipscpu.cp2cr[ 7 ].d )
#define L11 ( mipscpu.cp2cr[ 8 ].w.l )
#define L12 ( mipscpu.cp2cr[ 8 ].w.h )
#define L13 ( mipscpu.cp2cr[ 9 ].w.l )
#define L21 ( mipscpu.cp2cr[ 9 ].w.h )
#define L22 ( mipscpu.cp2cr[ 10 ].w.l )
#define L23 ( mipscpu.cp2cr[ 10 ].w.h )
#define L31 ( mipscpu.cp2cr[ 11 ].w.l )
#define L32 ( mipscpu.cp2cr[ 11 ].w.h )
#define L33 ( mipscpu.cp2cr[ 12 ].w.l )
#define RBK ( mipscpu.cp2cr[ 13 ].d )
#define GBK ( mipscpu.cp2cr[ 14 ].d )
#define BBK ( mipscpu.cp2cr[ 15 ].d )
#define LR1 ( mipscpu.cp2cr[ 16 ].w.l )
#define LR2 ( mipscpu.cp2cr[ 16 ].w.h )
#define LR3 ( mipscpu.cp2cr[ 17 ].w.l )
#define LG1 ( mipscpu.cp2cr[ 17 ].w.h )
#define LG2 ( mipscpu.cp2cr[ 18 ].w.l )
#define LG3 ( mipscpu.cp2cr[ 18 ].w.h )
#define LB1 ( mipscpu.cp2cr[ 19 ].w.l )
#define LB2 ( mipscpu.cp2cr[ 19 ].w.h )
#define LB3 ( mipscpu.cp2cr[ 20 ].w.l )
#define RFC ( mipscpu.cp2cr[ 21 ].d )
#define GFC ( mipscpu.cp2cr[ 22 ].d )
#define BFC ( mipscpu.cp2cr[ 23 ].d )
#define OFX ( mipscpu.cp2cr[ 24 ].d )
#define OFY ( mipscpu.cp2cr[ 25 ].d )
#define H   ( mipscpu.cp2cr[ 26 ].w.l )
#define DQA ( mipscpu.cp2cr[ 27 ].w.l )
#define DQB ( mipscpu.cp2cr[ 28 ].d )
#define ZSF3 ( mipscpu.cp2cr[ 29 ].w.l )
#define ZSF4 ( mipscpu.cp2cr[ 30 ].w.l )
#define FLAG ( mipscpu.cp2cr[ 31 ].d )

static UINT32 getcp2dr( int n_reg )
{
	if( n_reg == 1 || n_reg == 3 || n_reg == 5 || n_reg == 8 || n_reg == 9 || n_reg == 10 || n_reg == 11 )
	{
		mipscpu.cp2dr[ n_reg ].d = (INT32)(INT16)mipscpu.cp2dr[ n_reg ].d;
	}
	else if( n_reg == 17 || n_reg == 18 || n_reg == 19 )
	{
		mipscpu.cp2dr[ n_reg ].d = (UINT32)(UINT16)mipscpu.cp2dr[ n_reg ].d;
	}
	else if( n_reg == 29 )
	{
		ORGB = ( ( IR1 >> 7 ) & 0x1f ) | ( ( IR2 >> 2 ) & 0x3e0 ) | ( ( IR3 << 3 ) & 0x7c00 );
	}
	GTELOGX( "get CP2DR%u=%08x", n_reg, mipscpu.cp2dr[ n_reg ].d );
	return mipscpu.cp2dr[ n_reg ].d;
}

static void setcp2dr( int n_reg, UINT32 n_value )
{
	GTELOGX( "set CP2DR%u=%08x", n_reg, n_value );
	mipscpu.cp2dr[ n_reg ].d = n_value;

	if( n_reg == 15 )
	{
		SXY0 = SXY1;
		SXY1 = SXY2;
		SXY2 = SXYP;
	}
	else if( n_reg == 28 )
	{
		IR1 = ( IRGB & 0x1f ) << 4;
		IR2 = ( IRGB & 0x3e0 ) >> 1;
		IR3 = ( IRGB & 0x7c00 ) >> 6;
	}
	else if( n_reg == 30 )
	{
		UINT32 n_lzcs = LZCS;
		UINT32 n_lzcr = 0;

		if( ( n_lzcs & 0x80000000 ) != 0 )
		{
			n_lzcs = ~n_lzcs;
		}
		while( ( n_lzcs & 0x80000000 ) == 0 )
		{
			n_lzcr++;
			n_lzcs <<= 1;
		}
 		LZCR = n_lzcr;
	}
}

static UINT32 getcp2cr( int n_reg )
{
	GTELOGX( "get CP2CR%u=%08x", n_reg, mipscpu.cp2cr[ n_reg ].d );
	return mipscpu.cp2cr[ n_reg ].d;
}

static void setcp2cr( int n_reg, UINT32 n_value )
{
	GTELOGX( "set CP2CR%u=%08x", n_reg, n_value );
	mipscpu.cp2cr[ n_reg ].d = n_value;
}

static inline INT32 LIM( INT32 n_value, INT32 n_max, INT32 n_min, int n_bit )
{
	if( n_value > n_max )
	{
		FLAG |= 0x80000000 | ( 1 << n_bit );
		return n_max;
	}
	else if( n_value < n_min )
	{
		FLAG |= 0x80000000 | ( 1 << n_bit );
		return n_min;
	}
	return n_value;
}

static inline INT64 BOUNDS( INT64 n_value, INT64 n_max, int n_maxbit, INT64 n_min, int n_minbit )
{
	if( n_value > n_max )
	{
		FLAG |= ( 1 << n_maxbit );
	}
	else if( n_value < n_min )
	{
		FLAG |= ( 1 << n_minbit );
	}
	return n_value;
}

#define A1( a ) BOUNDS( ( a ), 0x1ffffffff, 30, -0x200000000, 27 )
#define A2( a ) BOUNDS( ( a ), 0x1ffffffff, 29, -0x200000000, 26 )
#define A3( a ) BOUNDS( ( a ), 0x1ffffffff, 28, -0x200000000, 25 )
#define	Lm_B1( a ) LIM( ( a ), 0x7fff, -0x8000, 24 )
#define	Lm_B2( a ) LIM( ( a ), 0x7fff, -0x8000, 23 )
#define	Lm_B3( a ) LIM( ( a ), 0x7fff, -0x8000, 22 )
#define Lm_D( a ) LIM( ( a ), 0xffff, 0x0000, 18 )

static __inline UINT32 Lm_E( UINT32 n_z )
{
	if( n_z < H / 2 )
	{
		n_z = H / 2;
		FLAG |= 0x80000000 | ( 1 << 17 );
	}
	if( n_z == 0 )
	{
		n_z = 1;
	}
	return n_z;
}

#define F( a ) BOUNDS( ( a ), 0x7fffffff, 16, -0x80000000, 15 )
#define Lm_G1( a ) LIM( ( a ), 0x3ff, -0x400, 14 )
#define Lm_H( a ) LIM( ( a ), 0xfff, 0x000, 12 )

static void docop2( int gteop )
{
	int n_v;
	static const UINT16 *p_n_vx[] = { &VX0, &VX1, &VX2 };
	static const UINT16 *p_n_vy[] = { &VY0, &VY1, &VY2 };
	static const UINT16 *p_n_vz[] = { &VZ0, &VZ1, &VZ2 };

	switch( gteop )
	{
	case 0x1400006:
		GTELOG( "NCLIP" );
		FLAG = 0;

		MAC0 = F(
			( (INT64)(INT16)SX0 * (INT16)SY1 ) + 
			( (INT64)(INT16)SX1 * (INT16)SY2 ) +
			( (INT64)(INT16)SX2 * (INT16)SY0 ) -
			( (INT64)(INT16)SX0 * (INT16)SY2 ) -
			( (INT64)(INT16)SX1 * (INT16)SY0 ) -
			( (INT64)(INT16)SX2 * (INT16)SY1 ) );
		break;
	case 0x0486012:
		GTELOG( "RTV0" );
		FLAG = 0;

		MAC1 = A1(
			( (INT64)(INT16)R11 * (INT16)VX0 ) +
			( (INT64)(INT16)R12 * (INT16)VY0 ) +
			( (INT64)(INT16)R13 * (INT16)VZ0 ) ) >> 12;
		MAC2 = A2(
			( (INT64)(INT16)R21 * (INT16)VX0 ) +
			( (INT64)(INT16)R22 * (INT16)VY0 ) +
			( (INT64)(INT16)R23 * (INT16)VZ0 ) ) >> 12;
		MAC3 = A3(
			( (INT64)(INT16)R31 * (INT16)VX0 ) +
			( (INT64)(INT16)R32 * (INT16)VY0 ) +
			( (INT64)(INT16)R33 * (INT16)VZ0 ) ) >> 12;
		IR1 = Lm_B1( (INT32)MAC1 );
		IR2 = Lm_B2( (INT32)MAC2 );
		IR3 = Lm_B3( (INT32)MAC3 );
		break;
	case 0x0280030:
		GTELOG( "RTPT" );
		FLAG = 0;

		for( n_v = 0; n_v < 3; n_v++ )
		{
			MAC1 = A1(
				( ( (INT64)(INT32)TRX ) << 12 ) +
				( (INT64)(INT16)R11 * (INT16)*p_n_vx[ n_v ] ) +
				( (INT64)(INT16)R12 * (INT16)*p_n_vy[ n_v ] ) +
				( (INT64)(INT16)R13 * (INT16)*p_n_vz[ n_v ] ) ) >> 12;
			MAC2 = A2(
				( ( (INT64)(INT32)TRY ) << 12 ) +
				( (INT64)(INT16)R21 * (INT16)*p_n_vx[ n_v ] ) +
				( (INT64)(INT16)R22 * (INT16)*p_n_vy[ n_v ] ) +
				( (INT64)(INT16)R23 * (INT16)*p_n_vz[ n_v ] ) ) >> 12;
			MAC3 = A3(
				( ( (INT64)(INT32)TRZ ) << 12 ) +
				( (INT64)(INT16)R31 * (INT16)*p_n_vx[ n_v ] ) +
				( (INT64)(INT16)R32 * (INT16)*p_n_vy[ n_v ] ) +
				( (INT64)(INT16)R33 * (INT16)*p_n_vz[ n_v ] ) ) >> 12;
			IR1 = Lm_B1( (INT32)MAC1 );
			IR2 = Lm_B2( (INT32)MAC2 );
			IR3 = Lm_B3( (INT32)MAC3 );
			SZ0 = SZ1;
			SZ1 = SZ2;
			SZ2 = SZ3;
			SZ3 = Lm_D( (INT32)MAC3 );
			SXY0 = SXY1;
			SXY1 = SXY2;
			SX2 = Lm_G1( F( (INT64)(INT32)OFX + ( (INT64)(INT16)IR1 * ( ( ( (UINT32)H ) << 16 ) / Lm_E( SZ3 ) ) ) ) >> 16 );
			SY2 = Lm_G1( F( (INT64)(INT32)OFY + ( (INT64)(INT16)IR2 * ( ( ( (UINT32)H ) << 16 ) / Lm_E( SZ3 ) ) ) ) >> 16 );
			MAC0 = F( (INT64)(INT32)DQB + ( (INT64)(INT16)DQA * ( ( ( (UINT32)H ) << 16 ) / Lm_E( SZ3 ) ) ) );
			IR0 = Lm_H( (INT32)MAC0 >> 12 );
		}
		break;
	case 0x00180001:
		GTELOG( "RTPS" );
		FLAG = 0;

		MAC1 = A1(
			( ( (INT64)(INT32)TRX ) << 12 ) +
			( (INT64)(INT16)R11 * (INT16)VX0 ) +
			( (INT64)(INT16)R12 * (INT16)VY0 ) +
			( (INT64)(INT16)R13 * (INT16)VZ0 ) ) >> 12;
		MAC2 = A2(
			( ( (INT64)(INT32)TRY ) << 12 ) +
			( (INT64)(INT16)R21 * (INT16)VX0 ) +
			( (INT64)(INT16)R22 * (INT16)VY0 ) +
			( (INT64)(INT16)R23 * (INT16)VZ0 ) ) >> 12;
		MAC3 = A3(
			( ( (INT64)(INT32)TRZ ) << 12 ) +
			( (INT64)(INT16)R31 * (INT16)VX0 ) +
			( (INT64)(INT16)R32 * (INT16)VY0 ) +
			( (INT64)(INT16)R33 * (INT16)VZ0 ) ) >> 12;
		IR1 = Lm_B1( (INT32)MAC1 );
		IR2 = Lm_B2( (INT32)MAC2 );
		IR3 = Lm_B3( (INT32)MAC3 );
		SZ0 = SZ1;
		SZ1 = SZ2;
		SZ2 = SZ3;
		SZ3 = Lm_D( (INT32)MAC3 );
		SXY0 = SXY1;
		SXY1 = SXY2;
		SX2 = Lm_G1( F( (INT64)(INT32)OFX + ( (INT64)(INT16)IR1 * ( ( ( (UINT32)H ) << 16 ) / Lm_E( SZ3 ) ) ) ) >> 16 );
		SY2 = Lm_G1( F( (INT64)(INT32)OFY + ( (INT64)(INT16)IR2 * ( ( ( (UINT32)H ) << 16 ) / Lm_E( SZ3 ) ) ) ) >> 16 );
		MAC0 = F( (INT64)(INT32)DQB + ( (INT64)(INT16)DQA * ( ( ( (UINT32)H ) << 16 ) / Lm_E( SZ3 ) ) ) );
		IR0 = Lm_H( (INT32)MAC0 >> 12 );
		break;
	case 0x0168002e:
		GTELOG( "AVSZ4" );
		FLAG = 0;

		MAC0 = F(
			( (INT64)ZSF4 * SZ0 ) +
			( (INT64)ZSF4 * SZ1 ) +
			( (INT64)ZSF4 * SZ2 ) +
			( (INT64)ZSF4 * SZ3 ) );
		OTZ = Lm_D( (INT32)MAC0 >> 12 );
		break;
	default:
		logerror( "%08x: GTE unknown op %08x\n", mipscpu.pc, INS_COFUN( mipscpu.op ) );
		mips_stop();
		break;
	}
}
