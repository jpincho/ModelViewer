#include <Platform/Platform.h>
#include <Platform/Logger.h>
#include <Platform/ArrayUtils.h>
#include <Platform/PlatformCPU.h>
#include "ThreadedFileLoader.h"
#include <Platform/ThreadPool.h>
#include <C_CrossRenderer/CommonLoaders.h>
#include "AssimpLoader.h"
#include <threads.h>
#include <string.h>
#include <assert.h>

#if defined (PLATFORM_COMPILER_MSVC)
#include <malloc.h>
#elif defined (PLATFORM_COMPILER_GNU)
#include <alloca.h>
#endif

typedef struct
	{
	DataFileContentDescriptor *Descriptor;
	int TaskID;
	ThreadedFileLoader_State State;
	} DataFileEntry;

typedef struct
	{
	int TaskID;
	int Index;
	} DataFileThreadData;

static ThreadPool *ResourceLoadingThreadpool = NULL;
static mtx_t ResourceAccessMutex;
static struct
	{
	DataFileEntry *Data;
	unsigned Used, Capacity;
	} DataFiles;

static int LoadAssimpFileThreadFunction ( void *Param )
	{
	DataFileThreadData *ThreadData = ( DataFileThreadData * ) Param;

	// load the file with assimp
	bool success = LoadFileWithAssimp ( DataFiles.Data[ThreadData->Index].Descriptor->Filename, DataFiles.Data[ThreadData->Index].Descriptor );
	if ( success == false )
		{
		DataFiles.Data[ThreadData->Index].State = HUB_STATE_LOADING_ERROR;
		return -1;
		}
	/*
		// Queue loading all textures needed for materials
		for ( unsigned MaterialIterator = 0; MaterialIterator < DataFiles.Data[ThreadData->Index].Descriptor->MaterialCount; ++MaterialIterator )
			{
			for ( unsigned TextureIterator = 0; TextureIterator < MatTexType_Count; ++TextureIterator )
				{
				if ( DataFiles.Data[ThreadData->Index].Descriptor->Materials[MaterialIterator].Textures == NULL )
					continue;
				if ( DataFiles.Data[ThreadData->Index].Descriptor->Materials[MaterialIterator].Textures[TextureIterator].TextureName == NULL )
					continue;

				// replace the filename from the model's path, with the texture filename, and load it
				unsigned PathLength = strlen ( DataFiles.Data[ThreadData->Index].Descriptor->Filename ) + strlen ( DataFiles.Data[ThreadData->Index].Descriptor->Materials[MaterialIterator].Textures[TextureIterator].TextureName ) + 1;
				if ( PathLength == 0 ) continue;
				char *FullPath = calloc ( PathLength, sizeof ( char ) );
				if ( FullPath == NULL )
					{
					return -1;
					}
				const char *LastSlash = strrchr ( DataFiles.Data[ThreadData->Index].Descriptor->Filename, '/' );
				if ( LastSlash )
					{
					strncpy ( FullPath, DataFiles.Data[ThreadData->Index].Descriptor->Filename, LastSlash - DataFiles.Data[ThreadData->Index].Descriptor->Filename + 1 ); // +1 to include the slash
					strcat ( FullPath, DataFiles.Data[ThreadData->Index].Descriptor->Materials[MaterialIterator].Textures[TextureIterator].TextureName );
					}
				else
					{
					strcpy ( FullPath, DataFiles.Data[ThreadData->Index].Descriptor->Filename );
					}

				// Update this material with the full path to the texture
				free ( DataFiles.Data[ThreadData->Index].Descriptor->Materials[MaterialIterator].Textures[TextureIterator].TextureName );
				DataFiles.Data[ThreadData->Index].Descriptor->Materials[MaterialIterator].Textures[TextureIterator].TextureName = strdup ( FullPath );
				ThreadedFileLoader_LoadFile ( FullPath );
				free ( FullPath );
				}
			}*/
	DataFiles.Data[ThreadData->Index].State = HUD_STATE_LOADED_OK;
	return 0;
	}

static int LoadTextureFileThreadFunction ( void *Param )
	{
	DataFileThreadData *ThreadData = ( DataFileThreadData * ) Param;
	LOG_DEBUG ( "Loading '%s'", DataFiles.Data[ThreadData->Index].Descriptor->Filename );

	// load the texture
	crTextureDescriptor *NewDescriptor = calloc ( 1, sizeof ( crTextureDescriptor ) );
	if ( crLoadTextureDescriptorFromFile ( DataFiles.Data[ThreadData->Index].Descriptor->Filename, true, NewDescriptor ) == false )
		{
		DataFiles.Data[ThreadData->Index].State = HUB_STATE_LOADING_ERROR;
		free ( NewDescriptor );
		return -1;
		}
	DataFiles.Data[ThreadData->Index].Descriptor->Textures = NewDescriptor;
	DataFiles.Data[ThreadData->Index].Descriptor->TextureCount = 1;
	DataFiles.Data[ThreadData->Index].State = HUD_STATE_LOADED_OK;
	LOG_DEBUG ( "Finished loading '%s'", DataFiles.Data[ThreadData->Index].Descriptor->Filename );
	return 0;
	}

bool ThreadedFileLoader_Initialize ( void )
	{
	mtx_init ( &ResourceAccessMutex, mtx_plain );
	ResourceLoadingThreadpool = ThreadPool_Create ();
	if ( ResourceLoadingThreadpool == NULL )
		{
		mtx_destroy ( &ResourceAccessMutex );
		return false;
		}
	DataFiles.Capacity = DataFiles.Used = 0;
	DataFiles.Data = NULL;
	IsValidModelFile ( "" ); // Just to get the initial list of extensions
	return true;
	}

void ThreadedFileLoader_Shutdown ( void )
	{
	ThreadPool_Destroy ( ResourceLoadingThreadpool );
	ResourceLoadingThreadpool = NULL;
	mtx_destroy ( &ResourceAccessMutex );
	SAFE_DEL_C ( DataFiles.Data );
	DataFiles.Capacity = DataFiles.Used = 0;
	}

int ThreadedFileLoader_LoadFile ( const char *Filename )
	{
	if ( ResourceLoadingThreadpool == NULL )
		{
		if ( ThreadedFileLoader_Initialize() == false )
			return -1;
		}

	if ( ( IsValidModelFile ( Filename ) == false ) && ( crIsValidTextureFile ( Filename ) == false ) )
		return -1;

	// configure the thread to load onto the selected new slot
	mtx_lock ( &ResourceAccessMutex );
	LOG_DEBUG ( "Queueing loading of '%s'", Filename );

	// Make space for the new resource, if needed
	if ( DataFiles.Used == DataFiles.Capacity )
		{
		const unsigned GrowStep = 10;
		if ( Array_Resize ( ( void ** ) &DataFiles.Data, DataFiles.Used, &DataFiles.Capacity, sizeof ( DataFileEntry ), DataFiles.Capacity + GrowStep, true ) == false )
			return -1;
		}

	// Search for an empty slot
	int FreeIndex = -1;
	for ( unsigned Index = 0; Index < DataFiles.Capacity; ++Index )
		{
		if ( DataFiles.Data[Index].State == HUB_STATE_EMPTY )
			{
			FreeIndex = Index;
			break;
			}
		}
	if ( FreeIndex == -1 )
		{
		mtx_unlock ( &ResourceAccessMutex );
		return -1;
		}

	DataFiles.Data[FreeIndex].Descriptor = calloc ( 1, sizeof ( DataFileContentDescriptor ) );
	if ( DataFiles.Data[FreeIndex].Descriptor == NULL )
		{
		mtx_unlock ( &ResourceAccessMutex );
		return -1;
		}
	DataFiles.Data[FreeIndex].Descriptor->Filename = strdup ( Filename );
	DataFiles.Data[FreeIndex].State = HUB_STATE_QUEUED;
	++DataFiles.Used;

	// allocate thread Data
	DataFileThreadData *NewThreadData = ( DataFileThreadData * ) calloc ( 1, sizeof ( DataFileThreadData ) );
	if ( NewThreadData == NULL )
		{
		mtx_unlock ( &ResourceAccessMutex );
		return -1;
		}
	NewThreadData->Index = FreeIndex;

	if ( IsValidModelFile ( Filename ) )
		{
		DataFiles.Data[FreeIndex].TaskID = ThreadPool_AddTask ( ResourceLoadingThreadpool, LoadAssimpFileThreadFunction, NewThreadData );
		}
	else if ( crIsValidTextureFile ( Filename ) )
		{
		DataFiles.Data[FreeIndex].TaskID = ThreadPool_AddTask ( ResourceLoadingThreadpool, LoadTextureFileThreadFunction, NewThreadData );
		}
	LOG_DEBUG ( "Queued loading of '%s' to slot %u", Filename, FreeIndex );
	mtx_unlock ( &ResourceAccessMutex );
	return FreeIndex;
	}

int ThreadedFileLoader_GetResourceFileIndex ( const char *Filename )
	{
	mtx_lock ( &ResourceAccessMutex );
	int Result = -1;
	for ( unsigned Index = 0; Index < DataFiles.Used; ++Index )
		{
		if ( strcmp ( Filename, DataFiles.Data[Index].Descriptor->Filename ) == 0 )
			{
			Result = Index;
			break;
			}
		}
	mtx_unlock ( &ResourceAccessMutex );
	return Result;
	}

void ThreadedFileLoader_UnloadResourceFile ( const int Index )
	{
	mtx_lock ( &ResourceAccessMutex );
	if ( Index >= DataFiles.Capacity )
		goto done;

	switch ( DataFiles.Data[Index].State )
		{
		case HUB_STATE_EMPTY:
			goto done;
		case HUB_STATE_LOADING:
			ThreadedFileLoader_WaitUntilAllLoaded();
			// TODO
			//if (Resources.Data[Index].TaskID != -1)
			//	threadpool_wait_for_task(ResourceLoadingThreadpool, Resources.Data[Index].TaskID);
			break;
		case HUB_STATE_QUEUED:
			if ( DataFiles.Data[Index].TaskID != -1 )
				ThreadPool_CancelTask ( ResourceLoadingThreadpool, DataFiles.Data[Index].TaskID );
			break;
		}
	DestroyDataFileContentDescriptor ( DataFiles.Data[Index].Descriptor );
	free ( DataFiles.Data[Index].Descriptor );
	DataFiles.Data[Index].State = HUB_STATE_EMPTY;
	DataFiles.Data[Index].TaskID = -1;
done:
	mtx_unlock ( &ResourceAccessMutex );
	}

DataFileContentDescriptor *ThreadedFileLoader_GetFileContentDescriptor ( const int Index )
	{
	return DataFiles.Data[Index].Descriptor;
	}

ThreadedFileLoader_State ThreadedFileLoader_GetFileLoadState ( const int Index )
	{
	return DataFiles.Data[Index].State;
	}

bool ThreadedFileLoader_WaitUntilAllLoaded ( void )
	{
	return ThreadPool_WaitForAllTasks ( ResourceLoadingThreadpool );
	}
