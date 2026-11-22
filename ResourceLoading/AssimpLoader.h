#pragma once
#include <stdbool.h>
#include "DataFileContentDescriptor.h"
#include <Platform/PointerArray.h>

BEGIN_C_DECLARATIONS
struct ModelFileLoadOptions
	{
	bool FlipTexturesVertically;
	};

struct ModelFileLoadOptions GetDefaultModelFileLoadOptions ( void );
bool LoadFileWithAssimp ( const char *Filename, const struct ModelFileLoadOptions Options, struct DataFileContentDescriptor *FileData );
bool IsValidModelFile ( const char *Filename );
const PointerArray GetSupportedModelFileExtensions ( void );
END_C_DECLARATIONS
