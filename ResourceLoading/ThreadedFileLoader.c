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

static void ClearFileSlot ( const int Index )
	{
	if ( DataFiles.Data[Index].Descriptor != NULL )
		{
		DestroyDataFileContentDescriptor ( DataFiles.Data[Index].Descriptor );
		free ( DataFiles.Data[Index].Descriptor );
		DataFiles.Data[Index].Descriptor = NULL;
		}
	if ( DataFiles.Data[Index].State != HUB_STATE_EMPTY )
		{
		if ( DataFiles.Used > 0 )
			--DataFiles.Used;
		}
	DataFiles.Data[Index].State = HUB_STATE_EMPTY;
	DataFiles.Data[Index].TaskID = -1;
	}

static int LoadAssimpFileThreadFunction ( void *Param )
	{
	DataFileThreadData *ThreadData = ( DataFileThreadData * ) Param;
	const int Index = ThreadData->Index;

	mtx_lock ( &ResourceAccessMutex );
	if ( ( Index < 0 ) || ( ( unsigned ) Index >= DataFiles.Capacity ) || ( DataFiles.Data[Index].State != HUB_STATE_QUEUED ) || ( DataFiles.Data[Index].Descriptor == NULL ) )
		{
		mtx_unlock ( &ResourceAccessMutex );
		free ( ThreadData );
		return -1;
		}
	if ( DataFiles.Data[Index].Descriptor->Filename == NULL )
		{
		DataFiles.Data[Index].State = HUB_STATE_LOADING_ERROR;
		mtx_unlock ( &ResourceAccessMutex );
		free ( ThreadData );
		return -1;
		}

	DataFiles.Data[Index].State = HUB_STATE_LOADING;
	DataFileContentDescriptor *Descriptor = DataFiles.Data[Index].Descriptor;
	mtx_unlock ( &ResourceAccessMutex );

	int Result = -1;
	LOG_DEBUG ( "Loading '%s'", DataFiles.Data[Index].Descriptor->Filename );
	if ( LoadFileWithAssimp ( DataFiles.Data[Index].Descriptor->Filename, DataFiles.Data[Index].Descriptor ) == true )
		{
		mtx_lock ( &ResourceAccessMutex );
		DataFiles.Data[Index].State = HUD_STATE_LOADED_OK;
		mtx_unlock ( &ResourceAccessMutex );
		Result = 0;
		}
	else
		{
		mtx_lock ( &ResourceAccessMutex );
		DataFiles.Data[Index].State = HUB_STATE_LOADING_ERROR;
		mtx_unlock ( &ResourceAccessMutex );
		Result = -1;
		}
	free ( ThreadData );
	LOG_DEBUG ( "Finished loading '%s'. Result %d", DataFiles.Data[Index].Descriptor->Filename, Result );
	return Result;
	}

static int LoadTextureFileThreadFunction ( void *Param )
	{
	DataFileThreadData *ThreadData = ( DataFileThreadData * ) Param;
	const int Index = ThreadData->Index;

	mtx_lock ( &ResourceAccessMutex );
	if ( ( Index < 0 ) || ( ( unsigned ) Index >= DataFiles.Capacity ) || ( DataFiles.Data[Index].State != HUB_STATE_QUEUED ) || ( DataFiles.Data[Index].Descriptor == NULL ) )
		{
		mtx_unlock ( &ResourceAccessMutex );
		free ( ThreadData );
		return -1;
		}

	if ( DataFiles.Data[Index].Descriptor->Filename == NULL )
		{
		DataFiles.Data[Index].State = HUB_STATE_LOADING_ERROR;
		mtx_unlock ( &ResourceAccessMutex );
		free ( ThreadData );
		return -1;
		}

	DataFiles.Data[Index].State = HUB_STATE_LOADING;
	DataFileContentDescriptor *Descriptor = DataFiles.Data[Index].Descriptor;
	mtx_unlock ( &ResourceAccessMutex );

	int Result = -1;
	LOG_DEBUG ( "Loading '%s'", DataFiles.Data[Index].Descriptor->Filename );
	crTextureDescriptor *NewDescriptor = calloc ( 1, sizeof ( crTextureDescriptor ) );
	if ( ( NewDescriptor != NULL ) && ( crLoadTextureDescriptorFromFile ( DataFiles.Data[Index].Descriptor->Filename, true, NewDescriptor ) == true ) )
		{
		mtx_lock ( &ResourceAccessMutex );
		DataFiles.Data[Index].Descriptor->Textures = NewDescriptor;
		DataFiles.Data[Index].Descriptor->TextureCount = 1;
		DataFiles.Data[Index].State = HUD_STATE_LOADED_OK;
		mtx_unlock ( &ResourceAccessMutex );
		Result = 0;
		}
	else
		{
		free ( NewDescriptor );
		mtx_lock ( &ResourceAccessMutex );
		DataFiles.Data[Index].State = HUB_STATE_LOADING_ERROR;
		mtx_unlock ( &ResourceAccessMutex );
		Result = -1;
		}
	free ( ThreadData );
	LOG_DEBUG ( "Finished loading '%s'. Result %d", DataFiles.Data[Index].Descriptor->Filename, Result );
	return Result;
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
	if ( DataFiles.Data != NULL )
		{
		for ( unsigned Index = 0; Index < DataFiles.Capacity; ++Index )
			{
			if ( DataFiles.Data[Index].Descriptor != NULL )
				{
				DestroyDataFileContentDescriptor ( DataFiles.Data[Index].Descriptor );
				free ( DataFiles.Data[Index].Descriptor );
				}
			}
		}
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
			{
			mtx_unlock ( &ResourceAccessMutex );
			return -1;
			}
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
	DataFiles.Data[FreeIndex].TaskID = -1;
	++DataFiles.Used;

	// allocate thread Data
	DataFileThreadData *NewThreadData = ( DataFileThreadData * ) calloc ( 1, sizeof ( DataFileThreadData ) );
	if ( NewThreadData == NULL )
		{
		ClearFileSlot ( FreeIndex );
		mtx_unlock ( &ResourceAccessMutex );
		return -1;
		}
	NewThreadData->Index = FreeIndex;

	int TaskID = -1;
	if ( IsValidModelFile ( Filename ) )
		TaskID = ThreadPool_AddTask ( ResourceLoadingThreadpool, LoadAssimpFileThreadFunction, NewThreadData );
	else if ( crIsValidTextureFile ( Filename ) )
		TaskID = ThreadPool_AddTask ( ResourceLoadingThreadpool, LoadTextureFileThreadFunction, NewThreadData );

	if ( TaskID < 0 )
		{
		free ( NewThreadData );
		ClearFileSlot ( FreeIndex );
		mtx_unlock ( &ResourceAccessMutex );
		return -1;
		}

	DataFiles.Data[FreeIndex].TaskID = TaskID;
	NewThreadData->TaskID = TaskID;
	LOG_DEBUG ( "Queued loading of '%s' to slot %u", Filename, FreeIndex );
	mtx_unlock ( &ResourceAccessMutex );
	return FreeIndex;
	}

int ThreadedFileLoader_GetResourceFileIndex ( const char *Filename )
	{
	mtx_lock ( &ResourceAccessMutex );
	int Result = -1;
	for ( unsigned Index = 0; Index < DataFiles.Capacity; ++Index )
		{
		if ( ( DataFiles.Data[Index].State == HUB_STATE_EMPTY ) || ( DataFiles.Data[Index].Descriptor == NULL ) || ( DataFiles.Data[Index].Descriptor->Filename == NULL ) )
			continue;
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
	if ( ( Index < 0 ) || ( ( unsigned ) Index >= DataFiles.Capacity ) || ( DataFiles.Data[Index].State == HUB_STATE_EMPTY ) )
		{
		mtx_unlock ( &ResourceAccessMutex );
		return;
		}

	if ( ( DataFiles.Data[Index].State == HUB_STATE_QUEUED ) || ( DataFiles.Data[Index].State == HUB_STATE_LOADING ) )
		{
		mtx_unlock ( &ResourceAccessMutex );
		ThreadedFileLoader_WaitUntilAllLoaded();
		mtx_lock ( &ResourceAccessMutex );
		}
	if ( ( ( unsigned ) Index < DataFiles.Capacity ) && ( DataFiles.Data[Index].State != HUB_STATE_EMPTY ) )
		ClearFileSlot ( Index );
	mtx_unlock ( &ResourceAccessMutex );
	}

DataFileContentDescriptor *ThreadedFileLoader_GetFileContentDescriptor ( const int Index )
	{
	if ( ( Index < 0 ) || ( ResourceLoadingThreadpool == NULL ) )
		return NULL;
	mtx_lock ( &ResourceAccessMutex );
	DataFileContentDescriptor *Result = NULL;
	if ( ( unsigned ) Index < DataFiles.Capacity )
		Result = DataFiles.Data[Index].Descriptor;
	mtx_unlock ( &ResourceAccessMutex );
	return Result;
	}

ThreadedFileLoader_State ThreadedFileLoader_GetFileLoadState ( const int Index )
	{
	if ( ( Index < 0 ) || ( ResourceLoadingThreadpool == NULL ) )
		return HUB_STATE_EMPTY;
	mtx_lock ( &ResourceAccessMutex );
	ThreadedFileLoader_State Result = HUB_STATE_EMPTY;
	if ( ( unsigned ) Index < DataFiles.Capacity )
		Result = DataFiles.Data[Index].State;
	mtx_unlock ( &ResourceAccessMutex );
	return Result;
	}

bool ThreadedFileLoader_WaitUntilAllLoaded ( void )
	{
	if ( ResourceLoadingThreadpool == NULL )
		return false;
	return ThreadPool_WaitForAllTasks ( ResourceLoadingThreadpool );
	}
