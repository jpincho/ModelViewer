#include "StringBuilder.h"
#include <Platform/Platform.h>
#include <Platform/ArrayUtils.h>
#include <Platform/defines.h>

#if defined ( PLATFORM_COMPILER_GNU)
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#elif defined ( PLATFORM_COMPILER_MSVC )
#include <stdarg.h>
#include <string.h>
#endif

bool StringBuilder_Initialize ( StringBuilder *Builder )
	{
	Builder->Data = NULL;
	Builder->Length = 0;
	Builder->Capacity = 0;
	return true;
	}

void StringBuilder_Destroy ( StringBuilder *Builder )
	{
	SAFE_DEL_C ( Builder->Data );
	Builder->Length = 0;
	Builder->Capacity = 0;
	}

void StringBuilder_Clear ( StringBuilder *Builder )
	{
	Builder->Length = 0;
	}

bool StringBuilder_Reserve ( StringBuilder *Builder, const unsigned NeededCapacity )
	{
	if ( Builder->Capacity >= NeededCapacity )
		return true;
	unsigned NewCapacity = ( Builder->Capacity == 0 ) ? 256 : Builder->Capacity;
	while ( NewCapacity < NeededCapacity )
		NewCapacity *= 2;
	if ( Memory_Realloc ( ( void ** ) &Builder->Data, NewCapacity ) == false )
		return false;
	Builder->Capacity = NewCapacity;
	return true;
	}

void StringBuilder_ShrinkToFit ( StringBuilder *Builder )
	{
	if ( Builder == NULL )
		return;
	if ( Builder->Capacity == Builder->Length )
		return;
	if ( Memory_Realloc ( ( void ** ) &Builder->Data, Builder->Length + 1 ) == false )
		return;
	Builder->Capacity = Builder->Length + 1;
	}

bool StringBuilder_Append ( StringBuilder *Builder, const char *Text )
	{
	if ( Text == NULL )
		return true;
	const unsigned TextLength = ( unsigned ) strlen ( Text );
	if ( StringBuilder_Reserve ( Builder, Builder->Length + TextLength + 1 ) == false )
		return false;
	memcpy ( Builder->Data + Builder->Length, Text, TextLength + 1 );
	Builder->Length += TextLength;
	return true;
	}

bool StringBuilder_Appendf ( StringBuilder *Builder, const char *Format, ... )
	{
	va_list Arguments;
	va_start ( Arguments, Format );
	va_list ArgumentsCopy;
	va_copy ( ArgumentsCopy, Arguments );
	const int Needed = vsnprintf ( NULL, 0, Format, Arguments );
	va_end ( Arguments );
	if ( Needed < 0 )
		{
		va_end ( ArgumentsCopy );
		return false;
		}
	if ( StringBuilder_Reserve ( Builder, Builder->Length + ( unsigned ) Needed + 1 ) == false )
		{
		va_end ( ArgumentsCopy );
		return false;
		}
	vsnprintf ( Builder->Data + Builder->Length, ( unsigned ) Needed + 1, Format, ArgumentsCopy );
	va_end ( ArgumentsCopy );
	Builder->Length += ( unsigned ) Needed;
	return true;
	}

char *StringBuilder_Take ( StringBuilder *Builder )
	{
	StringBuilder_ShrinkToFit ( Builder );
	char *Result = Builder->Data;
	Builder->Data = NULL;
	Builder->Length = 0;
	Builder->Capacity = 0;
	if ( Result == NULL )
		return strdup ( "" );
	return Result;
	}
