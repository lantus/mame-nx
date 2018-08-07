 
#include "osdepend.h"
#include "osd_cpu.h"
 

#include "fileio.h"
 
 
#include <stdio.h>
 
 
typedef enum
{
	kPlainFile,
	kRAMFile,
	kZippedFile
}	eFileType;

typedef struct
{
	FILE *file;
	unsigned char *data;
	unsigned int offset;
	unsigned int length;
	eFileType type;
	unsigned int crc;
}	FakeFileHandle;

//---------------------------------------------------------------------
//	osd_get_path_count
//---------------------------------------------------------------------
int osd_get_path_count( int pathtype )
{
	 
	return 1;
}

//---------------------------------------------------------------------
//	osd_get_path_info
//---------------------------------------------------------------------
int osd_get_path_info( int pathtype, int pathindex, const char *filename )
{
	if( pathtype == FILETYPE_ROM )
    {
		return PATH_IS_FILE;
	}
	else
		return PATH_NOT_FOUND;
 
	
}
 
//---------------------------------------------------------------------
//	osd_fopen
//---------------------------------------------------------------------
osd_file *osd_fopen( int pathtype, int pathindex, const char *filename, const char *mode )
{
 
	char name[256];
	char *gamename; 
	int indx; 
	FakeFileHandle *f;
	int pathc;
	char **pathv;
	
	f = (FakeFileHandle *) malloc(sizeof (FakeFileHandle));
	if( !f )
	{
		logerror("osd_fopen: failed to mallocFakeFileHandle!\n");
        return 0;
	}
	memset (f, 0, sizeof (FakeFileHandle));

	sprintf(name,"roms/%s",filename);
	 
	f->file = fopen(name, mode);

	if( !f->file)
	{		 
		free(f);
		return 0;
	}

	return f;
}
 


//---------------------------------------------------------------------
//  osd_fflush
//---------------------------------------------------------------------
void osd_fflush( osd_file *file )
{
 
}


//---------------------------------------------------------------------
//	osd_fseek
//---------------------------------------------------------------------
INT32 osd_fseek( osd_file *file, INT64 offset, int whence )
{
	FakeFileHandle *f = (FakeFileHandle *) file;
	 

	return fseek (f->file, offset, whence);
}

//---------------------------------------------------------------------
//	osd_ftell
//---------------------------------------------------------------------
UINT64 osd_ftell( osd_file *file )
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	return ftell(f->file);
}

//---------------------------------------------------------------------
//	osd_feof
//---------------------------------------------------------------------
int osd_feof( osd_file *file )
{
      
	FakeFileHandle *f = (FakeFileHandle *) file;

	return feof(f->file);   
}

//---------------------------------------------------------------------
//	osd_fread
//---------------------------------------------------------------------
UINT32 osd_fread( osd_file *file, void *buffer, UINT32 length )
{	
	FakeFileHandle *f = (FakeFileHandle *) file;
	return fread (buffer, 1, length, f->file);  
}

//---------------------------------------------------------------------
//	osd_fwrite
//---------------------------------------------------------------------
UINT32 osd_fwrite( osd_file *file, const void *buffer, UINT32 length )
{
	FakeFileHandle *f = (FakeFileHandle *) file;
	return fwrite (buffer, 1, length, ((FakeFileHandle *) file)->file);
}

//---------------------------------------------------------------------
//	osd_fclose
//---------------------------------------------------------------------
void osd_fclose( osd_file *file )
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	fclose (f->file);
	free(f); 
  
}
 