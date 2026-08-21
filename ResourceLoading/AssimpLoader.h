#pragma once
#include <stdbool.h>
#include "DataFileContentDescriptor.h"

BEGIN_C_DECLARATIONS
bool LoadFileWithAssimp ( const char *Filename, DataFileContentDescriptor *FileData );
bool IsValidModelFile ( const char *Filename );
END_C_DECLARATIONS
