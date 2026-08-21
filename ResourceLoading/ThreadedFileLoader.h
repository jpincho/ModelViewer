#pragma once
#include <Platform/defines.h>
#include <stdbool.h>
#include "DataFileContentDescriptor.h"

BEGIN_C_DECLARATIONS
typedef enum
	{
	HUB_STATE_EMPTY,
	HUB_STATE_QUEUED,
	HUB_STATE_LOADING,
	HUD_STATE_LOADED_OK,
	HUB_STATE_LOADING_ERROR
	} ThreadedFileLoader_State;

bool ThreadedFileLoader_Initialize ( void );
void ThreadedFileLoader_Shutdown ( void );
int ThreadedFileLoader_LoadFile ( const char *Filename );
int ThreadedFileLoader_GetResourceFileIndex ( const char *Filename );
void ThreadedFileLoader_UnloadResourceFile ( const int Index );
DataFileContentDescriptor *ThreadedFileLoader_GetFileContentDescriptor ( const int Index );
ThreadedFileLoader_State ThreadedFileLoader_GetFileLoadState ( const int Index );
bool ThreadedFileLoader_WaitUntilAllLoaded ( void );

END_C_DECLARATIONS
