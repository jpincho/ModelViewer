#pragma once
#include <stdbool.h>

typedef struct
	{
	char *Data;
	unsigned Length;
	unsigned Capacity;
	} StringBuilder;

bool StringBuilder_Initialize ( StringBuilder *Builder );
void StringBuilder_Destroy ( StringBuilder *Builder );
void StringBuilder_Clear ( StringBuilder *Builder );
bool StringBuilder_Reserve ( StringBuilder *Builder, const unsigned NeededCapacity );
void StringBuilder_ShrinkToFit ( StringBuilder *Builder );
bool StringBuilder_Append ( StringBuilder *Builder, const char *Text );
bool StringBuilder_Appendf ( StringBuilder *Builder, const char *Format, ... );
char *StringBuilder_Take ( StringBuilder *Builder );