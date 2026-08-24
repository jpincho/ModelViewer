#include "MaterialDescriptor.h"
#include <Platform/Logger.h>
#include <string.h>

void LogMaterialDescriptor ( const MaterialDescriptor *Descriptor, const int Indentation )
	{
	LOG_DEBUG ( "%*sMaterial '%s'", Indentation, "", Descriptor->MaterialName );
	LOG_DEBUG ( "%*sDiffuse %.02f %.02f %.02f", Indentation + 1, "", Descriptor->Diffuse, Descriptor->Diffuse.y, Descriptor->Diffuse.z );
	LOG_DEBUG ( "%*sSpecular %.02f %.02f %.02f", Indentation + 1, "", Descriptor->Specular.x, Descriptor->Specular.y, Descriptor->Specular.z );
	LOG_DEBUG ( "%*sEmissive %.02f %.02f %.02f", Indentation + 1, "", Descriptor->Emissive.x, Descriptor->Emissive.y, Descriptor->Emissive.z );
	LOG_DEBUG ( "%*sShininess - %.02f", Indentation + 1, "", Descriptor->Shininess );
	for ( unsigned TextureType = 0; TextureType < ( unsigned ) MatTexType_Count; ++TextureType )
		{
		LOG_DEBUG ( "%*s%s map - %s", Indentation + 1, "", Stringify_MatTexType ( ( MatTexType ) TextureType ), Descriptor->Textures[TextureType].TextureName == NULL ? "<NONE>" : Descriptor->Textures[TextureType].TextureName );
		}
	}

void DestroyMaterialDescriptor ( MaterialDescriptor *Material )
	{
	SAFE_DEL_C ( Material->MaterialName );
	for ( unsigned index = 0; index < MatTexType_Count; ++index )
		SAFE_DEL_C ( Material->Textures[index].TextureName );
	}

void CopyMaterialDescriptor ( MaterialDescriptor *Output, const MaterialDescriptor *Input )
	{
	memcpy ( Output, Input, sizeof ( MaterialDescriptor ) );
	Output->MaterialName = Input->MaterialName ? strdup ( Input->MaterialName ) : NULL;
	for ( unsigned TextureType = 0; TextureType < ( unsigned ) MatTexType_Count; ++TextureType )
		{
		if ( Input->Textures[TextureType].TextureName != NULL )
			Output->Textures[TextureType].TextureName = strdup ( Input->Textures[TextureType].TextureName );
		else
			Output->Textures[TextureType].TextureName = NULL;
		}
	}

const char *Stringify_MatTexType ( const MatTexType Value )
	{
	switch ( Value )
		{
#define STRINGIFY(X) case MatTexType_##X: return #X;
			STRINGIFY ( Diffuse );
			STRINGIFY ( Specular );
			STRINGIFY ( Ambient );
			STRINGIFY ( Emissive );
			STRINGIFY ( Height );
			STRINGIFY ( Normal );
			STRINGIFY ( Shininess );
			STRINGIFY ( Opacity );
			STRINGIFY ( Displacement );
			STRINGIFY ( Lightmap );
			STRINGIFY ( Reflection );
#undef STRINGIFY
		}
	return "unknown";
	}
