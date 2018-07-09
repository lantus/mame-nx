/*
 * standalone MIPS disassembler by smf
 *
 * based on DIS68k by Aaron Giles
 *
 */


#include "osd_cpu.h"
static UINT8 *filebuf;
static UINT32 offset;
static UINT8 order[] = { 3, 2, 1, 0 };

#define STANDALONE
#include "mipsdasm.c"

static char *Options[]=
{
	"begin","end","offset","order",0
};

static void usage (void)
{
	printf ("Usage: DISMIPS [options] <filename>\n\n"
			"Available options are:\n"
			" -begin  - Specify begin offset in file to disassemble in bytes [0]\n"
			" -end    - Specify end offset in file to disassemble in bytes [none]\n"
			" -offset - Specify address to load program in bytes [0]\n"
			" -order  - Specify byte order [3210]\n\n"
			"All values should be entered in hexadecimal\n");
	exit (1);
}

int main (int argc,char *argv[])
{
	UINT8 i,j,n,p;
	char *filename=0,buf[80];
	FILE *f;
	UINT32 begin=0,end=(UINT32)-1,filelen,len,pc;

	n=0;
	for (i=1;i<argc;++i)
	{
		if (argv[i][0]!='-')
		{
			switch (++n)
			{
			case 1:  filename=argv[i]; break;
			default: usage();
			}
		}
		else
		{
			for (j=0;Options[j];++j)
				if (!strcmp(argv[i]+1,Options[j])) break;

			switch (j)
			{
			case 0: ++i; if (i>argc) usage();
				begin=strtoul(argv[i],0,16);
				break;
			case 1:  ++i; if (i>argc) usage();
				end=strtoul(argv[i],0,16);
				break;
			case 2:  ++i; if (i>argc) usage();
				offset=strtoul(argv[i],0,16);
				break;
			case 3: ++i; if (i>argc) usage();
				if( strlen( argv[ i ] ) != 4 )
				{
					usage();
				}
				for( p=0; p < 4; p++ )
				{
					if( argv[ i ][ p ] < '0' || argv[ i ][ p ] > '3' )
					{
						usage();
					}
					order[ p ] = argv[ i ][ p ] - '0';
				}
				break;
			default: usage();
			}
		}
	}

	if (!filename)
	{
		usage();
		return 1;
	}
	f=fopen (filename,"rb");
	if (!f)
	{
		printf ("Unable to open %s\n",filename);
		return 2;
	}
	fseek (f,0,SEEK_END);
	filelen=ftell (f);
	fseek (f,begin,SEEK_SET);
	len=(filelen>end)? (end-begin+1):(filelen-begin);
	filebuf=malloc(len+16);
	if (!filebuf)
	{
		printf ("Memory allocation error\n");
		fclose (f);
		return 3;
	}
	memset (filebuf,0,len+16);
	if (fread(filebuf,1,len,f)!=len)
	{
		printf ("Read error\n");
		fclose (f);
		free (filebuf);
		return 4;
	}
	fclose (f);
	pc = 0;
	while( pc < len )
	{
		i = DasmMIPS( buf, pc + offset );

		printf( "%08x: ", pc + offset );
		for( j = 0; j < i ;j++ )
		{
			printf( "%02x ", filebuf[ ( pc & ~3 ) + order[ pc & 3 ] ] );
			pc++;
		}
		while( j < 10 )
		{
			printf( "   " );
			j++;
		}
		printf( "%s\n", buf );
	}
	free (filebuf);
	return 0;
}
