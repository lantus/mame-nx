 
#include "osdepend.h"
#include "osd_cpu.h"
#include "fileio.h"
 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
 
#define TRUE  (1==1)
#define FALSE (!TRUE)

typedef enum
{
	kPlainFile,
	kRAMFile,
	kZippedFile
}	eFileType;


struct _osd_file
{

    FILE* file;

};



enum stat_mode
{
   IS_DIRECTORY = 0,
   IS_CHARACTER_SPECIAL,
   IS_VALID
};


//---------------------------------------------------------------------
//	osd_get_path_count
//---------------------------------------------------------------------
int osd_get_path_count( int pathtype )
{
	 
	return 1;
}

static int path_stat(const char *path, enum stat_mode mode, int32_t *size)
{
 
   struct stat buf;
   if (stat(path, &buf) < 0)
      return FALSE;
 

   if (size)
      *size = (int32_t)buf.st_size;

   switch (mode)
   {
      case IS_DIRECTORY: 
         return S_ISCHR(buf.st_mode); 
      case IS_VALID:
         return TRUE;
   }

   return FALSE;
}

int path_is_directory(const char *path)
{
   return path_stat(path, IS_DIRECTORY, NULL);
}


//---------------------------------------------------------------------
//	osd_get_path_info
//---------------------------------------------------------------------
int osd_get_path_info( int pathtype, int pathindex, const char *filename )
{
   char buffer[256];
   char currDir[256];

   osd_get_path(pathtype, currDir);
   sprintf(buffer, "%s/%s", currDir, filename);

  debugload("osd_get_path_info (buffer = [%s]), (directory: [%s]), (path type: [%d]), (filename: [%s]) \n", buffer, currDir, pathtype, filename);

   if (path_is_directory(buffer))
   { 
	  debugload("PATH_IS_DIRECTORY\n");
      return PATH_IS_DIRECTORY;
   }
   else if (access(buffer, F_OK) == 0)
   {
	  debugload("PATH_IS_FILE\n");
      return PATH_IS_FILE;	  
   }

   return PATH_NOT_FOUND;
}
 
void osd_get_path(int pathtype, char* path)
{
   switch (pathtype)
   {
      case FILETYPE_ROM:
      case FILETYPE_IMAGE:
         sprintf(path, "roms");
         break;             
      case FILETYPE_IMAGE_DIFF:
         sprintf(path,"diff");
         break;     
      case FILETYPE_NVRAM:
         sprintf(path,"nvram");
         break;
	  case FILETYPE_HIGHSCORE_DB:
      case FILETYPE_HIGHSCORE:
         sprintf(path,"hi");
         break;
      case FILETYPE_CONFIG:
         sprintf(path,"cfg");
         break;
      case FILETYPE_MEMCARD:
         sprintf(path,"memcard");
         break;
      case FILETYPE_CTRLR:
         sprintf(path,"ctrlr");
         break;              
      case FILETYPE_ARTWORK:
         sprintf(path,"artwork");
         break;
      case FILETYPE_SAMPLE:
         sprintf(path,"samples");
         break;
      default:          
         sprintf(path,"");
   }    
} 
//---------------------------------------------------------------------
//	osd_fopen
//---------------------------------------------------------------------
osd_file *osd_fopen( int pathtype, int pathindex, const char *filename, const char *mode )
{
 
    char buffer[256];
    char currDir[256];
	 
	osd_file* f = malloc(sizeof(osd_file));
	 		 
	if( !f )
	{
		logerror("osd_fopen: failed to mallocFakeFileHandle!\n");
        return NULL;
	}
	 	 
	osd_get_path(pathtype, currDir);
	sprintf(buffer, "%s/%s", currDir,  filename);
	
	debugload(buffer);
	
	if (path_is_directory(currDir) == FALSE)
		mkdir(currDir, 0750);
	 
	f->file = fopen(buffer, mode);

	if( !f->file)
	{	
		if (f)
		{
			free(f);
			f = NULL;
		}
		
		return NULL;
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
 
	return fseek (file->file, offset, whence);
}

//---------------------------------------------------------------------
//	osd_ftell
//---------------------------------------------------------------------
UINT64 osd_ftell( osd_file *file )
{
	 
	return ftell(file->file);
}

//---------------------------------------------------------------------
//	osd_feof
//---------------------------------------------------------------------
int osd_feof( osd_file *file )
{ 
	return feof(file->file);   
}

//---------------------------------------------------------------------
//	osd_fread
//---------------------------------------------------------------------
UINT32 osd_fread( osd_file *file, void *buffer, UINT32 length )
{		 
	return fread (buffer, 1, length, file->file);  
}

//---------------------------------------------------------------------
//	osd_fwrite
//---------------------------------------------------------------------
UINT32 osd_fwrite( osd_file *file, const void *buffer, UINT32 length )
{
	 
	return fwrite (buffer, 1, length, file->file);
}

//---------------------------------------------------------------------
//	osd_fclose
//---------------------------------------------------------------------
void osd_fclose( osd_file *file )
{
	 
	if (file->file)
	{
		fclose (file->file);
		file->file = NULL;
		free(file); 
		file = NULL;
	}
  
}
 