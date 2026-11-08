#include <Platform/Platform.h>
#include "AssimpLoader.h"
#include "MaterialDescriptor.h"
#include "MeshDescriptor.h"
#include "AnimationDescriptor.h"
#include <Platform/Logger.h>
#include <Platform/PointerArray.h>

#include <C_CrossRenderer/CommonLoaders.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>

#if defined (PLATFORM_COMPILER_GNU)
// For fnmatch definition of FNM_CASEFOLD
#define _GNU_SOURCE
#include <fnmatch.h>
#elif defined (PLATFORM_COMPILER_MSVC)
#include <shlwapi.h>
#endif

const unsigned MAX_BONE_WEIGHTS_PER_VERTEX = 4;

static crTextureWrapMode Translate_aiTextureMapMode ( const enum aiTextureMapMode input )
	{
	switch ( input )
		{
		case aiTextureMapMode_Wrap:
			return crTextureWrapMode_Repeat;
		case aiTextureMapMode_Clamp:
			return crTextureWrapMode_ClampToBorder;
		case aiTextureMapMode_Mirror:
			return crTextureWrapMode_MirroredRepeat;
		case aiTextureMapMode_Decal:
		default:
			LOG_ERROR ( "Unhandled aiTextureMapMode" );
			return crTextureWrapMode_Repeat;
		}
	}

static MatTexType Translate_aiTextureType ( const enum aiTextureType input )
	{
	switch ( input )
		{
		case aiTextureType_DIFFUSE:
			return MatTexType_Diffuse;
		case aiTextureType_SPECULAR:
			return MatTexType_Specular;
		case aiTextureType_AMBIENT:
			return MatTexType_Ambient;
		case aiTextureType_EMISSIVE:
			return MatTexType_Emissive;
		case aiTextureType_HEIGHT:
			return MatTexType_Height;
		case aiTextureType_NORMALS:
			return MatTexType_Normal;
		case aiTextureType_SHININESS:
			return MatTexType_Shininess;
		case aiTextureType_OPACITY:
			return MatTexType_Opacity;
		case aiTextureType_DISPLACEMENT:
			return MatTexType_Displacement;
		case aiTextureType_LIGHTMAP:
			return MatTexType_Lightmap;
		case aiTextureType_REFLECTION:
			return MatTexType_Reflection;

		case aiTextureType_BASE_COLOR:
		case aiTextureType_NORMAL_CAMERA:
		case aiTextureType_EMISSION_COLOR:
		case aiTextureType_METALNESS:
		case aiTextureType_DIFFUSE_ROUGHNESS:
		case aiTextureType_AMBIENT_OCCLUSION:

		default:
			return -1;
		};
	}

static enum aiTextureType TranslateMatTexTypeToAssimp ( const MatTexType TextureType )
	{
	switch ( TextureType )
		{
		case MatTexType_Diffuse:
			return aiTextureType_DIFFUSE;
		case MatTexType_Specular:
			return aiTextureType_SPECULAR;
		case MatTexType_Ambient:
			return aiTextureType_AMBIENT;
		case MatTexType_Emissive:
			return aiTextureType_EMISSIVE;
		case MatTexType_Height:
			return aiTextureType_HEIGHT;
		case MatTexType_Normal:
			return aiTextureType_NORMALS;
		case MatTexType_Shininess:
			return aiTextureType_SHININESS;
		case MatTexType_Opacity:
			return aiTextureType_OPACITY;
		case MatTexType_Displacement:
			return aiTextureType_DISPLACEMENT;
		case MatTexType_Lightmap:
			return aiTextureType_LIGHTMAP;
		case MatTexType_Reflection:
			return aiTextureType_REFLECTION;
		case MatTexType_Count:
			break;
		};
	return aiTextureType_NONE;
	}

static enum aiTextureType GetAssimpFallbackTextureType ( const MatTexType TextureType )
	{
	switch ( TextureType )
		{
		case MatTexType_Diffuse:
			return aiTextureType_BASE_COLOR;
		case MatTexType_Normal:
			return aiTextureType_NORMAL_CAMERA;
		case MatTexType_Emissive:
			return aiTextureType_EMISSION_COLOR;
		case MatTexType_Lightmap:
			return aiTextureType_AMBIENT_OCCLUSION;
		default:
			return aiTextureType_NONE;
		}
	}

static void Translate_aiMatrix4x4 ( const struct aiMatrix4x4 input, mat4 *output )
	{
	math_mat4_transpose ( output, * ( ( mat4 * ) &input.a1 ) );
	}

static void Translate_aiVector3D ( const struct aiVector3D input, vec3 *output )
	{
	output->x = input.x;
	output->y = input.y;
	output->z = input.z;
	}

static void Translate_aiQuaternion ( const struct aiQuaternion input, vec4 *output )
	{
	output->x = input.x;
	output->y = input.y;
	output->z = input.z;
	output->w = input.w;
	}

static void CountAssimpNodes ( const struct aiNode *CurrentAssimpNode, unsigned *Count )
	{
	if ( CurrentAssimpNode == NULL )
		return;

	++ ( *Count );
	for ( unsigned index = 0; index < CurrentAssimpNode->mNumChildren; ++index )
		{
		CountAssimpNodes ( CurrentAssimpNode->mChildren[index], Count );
		}
	}

static unsigned LoadAssimpNode ( struct MeshTreeNode *NodeTree, unsigned *CurrentNodeTreeSize, const int ParentIndex, const struct aiNode *CurrentAssimpNode )
	{
	unsigned NewIndex = ( *CurrentNodeTreeSize )++;
	NodeTree[NewIndex].NodeName = strdup ( CurrentAssimpNode->mName.data );
	NodeTree[NewIndex].ParentNode = ParentIndex;
	Translate_aiMatrix4x4 ( CurrentAssimpNode->mTransformation, &NodeTree[NewIndex].DefaultTransformationMatrix );
	NodeTree[NewIndex].MeshIndexCount = CurrentAssimpNode->mNumMeshes;
	if ( NodeTree[NewIndex].MeshIndexCount != 0 )
		{
		NodeTree[NewIndex].MeshIndices = calloc ( NodeTree[NewIndex].MeshIndexCount, sizeof ( uint32_t ) );
		assert ( NodeTree[NewIndex].MeshIndices != NULL );
		for ( unsigned MeshIndex = 0; MeshIndex < NodeTree[NewIndex].MeshIndexCount; ++MeshIndex )
			{
			NodeTree[NewIndex].MeshIndices[MeshIndex] = CurrentAssimpNode->mMeshes[MeshIndex];
			}
		}

	NodeTree[NewIndex].ChildNodeCount = CurrentAssimpNode->mNumChildren;
	if ( NodeTree[NewIndex].ChildNodeCount != 0 )
		{
		NodeTree[NewIndex].ChildNodes = calloc ( NodeTree[NewIndex].ChildNodeCount, sizeof ( int ) );
		assert ( NodeTree[NewIndex].ChildNodes != NULL );
		for ( unsigned ChildNodeIterator = 0; ChildNodeIterator < NodeTree[NewIndex].ChildNodeCount; ++ChildNodeIterator )
			{
			unsigned ChildNodeIndex = LoadAssimpNode ( NodeTree, CurrentNodeTreeSize, NewIndex, CurrentAssimpNode->mChildren[ChildNodeIterator] );
			NodeTree[NewIndex].ChildNodes[ChildNodeIterator] = ChildNodeIndex;
			}
		}

	return NewIndex;
	}

static bool LoadAssimpMaterial ( const struct aiMaterial *AssimpMaterial, struct MaterialDescriptor *Descriptor )
	{
	memset ( Descriptor, 0, sizeof ( struct MaterialDescriptor ) );

	struct aiColor4D AssimpColor4;
	float FloatValue;
	struct aiString AssimpString;
	if ( aiGetMaterialString ( AssimpMaterial, AI_MATKEY_NAME, &AssimpString ) == aiReturn_SUCCESS )
		Descriptor->MaterialName = strdup ( AssimpString.data );
	else
		{
		char GeneratedName[64];
		snprintf ( GeneratedName, sizeof ( GeneratedName ), "Material_%p", ( void * ) AssimpMaterial );
		Descriptor->MaterialName = strdup ( GeneratedName );
		LOG_WARNING ( "WARNING! Generated material name '%s'", GeneratedName );
		}

	if ( aiGetMaterialColor ( AssimpMaterial, AI_MATKEY_COLOR_DIFFUSE, &AssimpColor4 ) == AI_SUCCESS )
		memcpy ( &Descriptor->Diffuse, &AssimpColor4, sizeof ( float ) * 3 );
	if ( aiGetMaterialColor ( AssimpMaterial, AI_MATKEY_COLOR_SPECULAR, &AssimpColor4 ) == AI_SUCCESS )
		memcpy ( &Descriptor->Specular, &AssimpColor4, sizeof ( float ) * 3 );
	if ( aiGetMaterialColor ( AssimpMaterial, AI_MATKEY_COLOR_EMISSIVE, &AssimpColor4 ) == AI_SUCCESS )
		memcpy ( &Descriptor->Emissive, &AssimpColor4, sizeof ( float ) * 3 );
	if ( aiGetMaterialFloat ( AssimpMaterial, AI_MATKEY_SHININESS, &FloatValue ) == AI_SUCCESS )
		Descriptor->Shininess = FloatValue;

	for ( unsigned TextureTypeIterator = 0; TextureTypeIterator < ( unsigned ) MatTexType_Count; ++TextureTypeIterator )
		{
		enum aiTextureType AssimpType = TranslateMatTexTypeToAssimp ( TextureTypeIterator );
		if ( aiGetMaterialString ( AssimpMaterial, AI_MATKEY_TEXTURE ( AssimpType, 0 ), &AssimpString ) != AI_SUCCESS )
			{
			AssimpType = GetAssimpFallbackTextureType ( TextureTypeIterator );
			if ( ( AssimpType == aiTextureType_NONE ) || ( aiGetMaterialString ( AssimpMaterial, AI_MATKEY_TEXTURE ( AssimpType, 0 ), &AssimpString ) != AI_SUCCESS ) )
				continue;
			}
		Descriptor->Textures[TextureTypeIterator].TextureName = strdup ( AssimpString.data );
		int hwrap, vwrap;
		if ( ( aiGetMaterialInteger ( AssimpMaterial, AI_MATKEY_MAPPINGMODE_U ( AssimpType, 0 ), &hwrap ) == AI_SUCCESS ) &&
		        ( aiGetMaterialInteger ( AssimpMaterial, AI_MATKEY_MAPPINGMODE_V ( AssimpType, 0 ), &vwrap ) == AI_SUCCESS ) )
			{
			Descriptor->Textures[TextureTypeIterator].WrapSettings.Horizontal = Translate_aiTextureMapMode ( hwrap );
			Descriptor->Textures[TextureTypeIterator].WrapSettings.Vertical = Translate_aiTextureMapMode ( vwrap );
			}
		}

	LOG_DEBUG ( "Loaded material '%s'", Descriptor->MaterialName );
	return true;
	}

static bool LoadAssimpTexture ( const struct aiTexture *AssimpTexture, crTextureDescriptor *Descriptor )
	{
	memset ( Descriptor, 0, sizeof ( crTextureDescriptor ) );
	Descriptor->Type = crTextureType_Texture2D;
	if ( AssimpTexture->mFilename.length != 0 )
		Descriptor->TextureName = strdup ( AssimpTexture->mFilename.data );
	else
		{
		char GeneratedName[64];
		snprintf ( GeneratedName, sizeof ( GeneratedName ), "Embedded_%p", ( void * ) AssimpTexture );
		Descriptor->TextureName = strdup ( GeneratedName );
		LOG_WARNING ( "WARNING! Generated texture name '%s'", GeneratedName );
		}
	if ( Descriptor->TextureName == NULL )
		return false;

	if ( ( strcmp ( AssimpTexture->achFormatHint, "rgba8888" ) == 0 ) ||
	        ( strcmp ( AssimpTexture->achFormatHint, "argb8888" ) == 0 ) ||
	        ( strcmp ( AssimpTexture->achFormatHint, "rgba5650" ) == 0 ) )
		{
		Descriptor->Data = calloc ( AssimpTexture->mHeight * AssimpTexture->mWidth, sizeof ( struct aiTexel ) );
		if ( Descriptor->Data == NULL )
			return false;
		memcpy ( Descriptor->Data, AssimpTexture->pcData, AssimpTexture->mHeight * AssimpTexture->mWidth * sizeof ( struct aiTexel ) );
		Descriptor->Dimensions.x = AssimpTexture->mWidth;
		Descriptor->Dimensions.y = AssimpTexture->mHeight;

		if ( strcmp ( AssimpTexture->achFormatHint, "rgba8888" ) == 0 )
			Descriptor->Format = crPixelFormat_RedGreenBlueAlpha8888;
		else if ( strcmp ( AssimpTexture->achFormatHint, "argb8888" ) == 0 )
			Descriptor->Format = crPixelFormat_AlphaRedGreenBlue8888;
		else
			Descriptor->Format = crPixelFormat_RedGreenBlue565;
		}
	else
		{
		if ( crLoadTextureDescriptorFromMemory ( ( void * ) AssimpTexture->pcData, ( unsigned ) AssimpTexture->mWidth, true, Descriptor ) == false )
			{
			LOG_ERROR ( "Unable to load embedded texture '%s'", Descriptor->TextureName );
			return false;
			}
		}
	LOG_DEBUG ( "Loaded embedded texture '%s'", Descriptor->TextureName );
	return true;
	}

static bool LoadAssimpBone ( const struct aiBone *AssimpBone, struct MeshBone *Descriptor )
	{
	Descriptor->BoneName = strdup ( AssimpBone->mName.data );
	Descriptor->NodeIndex = -1;
	Translate_aiMatrix4x4 ( AssimpBone->mOffsetMatrix, &Descriptor->ModelToBoneMatrix );
	return true;
	}

static bool LoadAssimpMesh ( const struct aiMesh *AssimpMesh, struct MeshDescriptor *Descriptor )
	{
	memset ( Descriptor, 0, sizeof ( struct MeshDescriptor ) );

	//NewMeshDescriptor.Primitive = Translate((aiPrimitiveType)AssimpMesh->mPrimitiveTypes);

	if ( AssimpMesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE )
		{
		LOG_WARNING ( "Mesh '%s' has non-triangle primitives, ignoring", AssimpMesh->mName.data );
		return false;
		}
	Descriptor->Primitive = crTriangleList;
	Descriptor->MeshName = strdup ( AssimpMesh->mName.data );
	Descriptor->Positions = calloc ( AssimpMesh->mNumVertices, sizeof ( vec3 ) );
	ASSERT_FAIL ( Descriptor->Positions );
	memcpy ( Descriptor->Positions, AssimpMesh->mVertices, sizeof ( vec3 ) * AssimpMesh->mNumVertices );
	Descriptor->VertexCount = AssimpMesh->mNumVertices;

	if ( ( AssimpMesh->mBitangents != NULL ) && ( AssimpMesh->mTangents != NULL ) )
		{
		Descriptor->Bitangents = calloc ( AssimpMesh->mNumVertices, sizeof ( vec3 ) );
		assert ( Descriptor->Bitangents );
		memcpy ( Descriptor->Bitangents, AssimpMesh->mBitangents, sizeof ( vec3 ) * AssimpMesh->mNumVertices );
		Descriptor->Tangents = calloc ( AssimpMesh->mNumVertices, sizeof ( vec3 ) );
		assert ( Descriptor->Tangents );
		memcpy ( Descriptor->Tangents, AssimpMesh->mTangents, sizeof ( vec3 ) * AssimpMesh->mNumVertices );
		}
	if ( AssimpMesh->mNormals != NULL )
		{
		Descriptor->Normals = calloc ( AssimpMesh->mNumVertices, sizeof ( vec3 ) );
		assert ( Descriptor->Normals );
		memcpy ( Descriptor->Normals, AssimpMesh->mNormals, sizeof ( vec3 ) * AssimpMesh->mNumVertices );
		}
	if ( ( AssimpMesh->mTextureCoords != NULL ) && ( AssimpMesh->mTextureCoords[0] != NULL ) )
		{
		Descriptor->TexCoords = calloc ( AssimpMesh->mNumVertices, sizeof ( vec2 ) );
		assert ( Descriptor->TexCoords );
		for ( unsigned VertexIndex = 0; VertexIndex < AssimpMesh->mNumVertices; ++VertexIndex )
			{
			Descriptor->TexCoords[VertexIndex].x = AssimpMesh->mTextureCoords[0][VertexIndex].x;
			Descriptor->TexCoords[VertexIndex].y = AssimpMesh->mTextureCoords[0][VertexIndex].y;
			}
		}
	if ( ( AssimpMesh->mColors != NULL ) && ( AssimpMesh->mColors[0] != NULL ) )
		{
		Descriptor->Colors = calloc ( AssimpMesh->mNumVertices, sizeof ( vec4 ) );
		assert ( Descriptor->Colors );
		for ( unsigned VertexIndex = 0; VertexIndex < AssimpMesh->mNumVertices; ++VertexIndex )
			{
			Descriptor->Colors[VertexIndex].x = AssimpMesh->mColors[0][VertexIndex].r;
			Descriptor->Colors[VertexIndex].y = AssimpMesh->mColors[0][VertexIndex].g;
			Descriptor->Colors[VertexIndex].z = AssimpMesh->mColors[0][VertexIndex].b;
			Descriptor->Colors[VertexIndex].w = AssimpMesh->mColors[0][VertexIndex].a;
			}
		}

	Descriptor->Indices = calloc ( AssimpMesh->mNumFaces * 3, sizeof ( uint32_t ) );
	assert ( Descriptor->Indices );
	Descriptor->IndexCount = AssimpMesh->mNumFaces * 3;
	unsigned index = 0;
	for ( unsigned face_index = 0; face_index < AssimpMesh->mNumFaces; ++face_index )
		{
		assert ( AssimpMesh->mFaces[face_index].mNumIndices == 3 );
		Descriptor->Indices[index++] = AssimpMesh->mFaces[face_index].mIndices[0];
		Descriptor->Indices[index++] = AssimpMesh->mFaces[face_index].mIndices[1];
		Descriptor->Indices[index++] = AssimpMesh->mFaces[face_index].mIndices[2];
		}

	if ( AssimpMesh->mNumBones != 0 )
		{
		// Count maximum number of weights per vertex, and total weight. This is just a sanity verification, probably could skip this step...
		unsigned *BoneWeightCountPerVertex = calloc ( AssimpMesh->mNumVertices, sizeof ( unsigned ) );
		float *BoneWeightPerVertex = calloc ( AssimpMesh->mNumVertices, sizeof ( float ) );

		for ( unsigned BoneIndex = 0; BoneIndex < AssimpMesh->mNumBones; ++BoneIndex )
			{
			struct aiBone *AssimpBone = AssimpMesh->mBones[BoneIndex];
			for ( unsigned WeightIndex = 0; WeightIndex < AssimpBone->mNumWeights; ++WeightIndex )
				{
				BoneWeightCountPerVertex[AssimpBone->mWeights[WeightIndex].mVertexId]++;
				BoneWeightPerVertex[AssimpBone->mWeights[WeightIndex].mVertexId] += AssimpBone->mWeights[WeightIndex].mWeight;
				}
			}

		for ( unsigned vertex_index = 0; vertex_index < AssimpMesh->mNumVertices; ++vertex_index )
			{
			if ( BoneWeightCountPerVertex[vertex_index] > MAX_WEIGHTS_PER_VERTEX )
				LOG_ERROR ( "Max weight count per vertex exceeded at vertex %u (%u)", vertex_index, BoneWeightCountPerVertex[vertex_index] );
			if ( BoneWeightPerVertex[vertex_index] > 1.0f )
				LOG_ERROR ( "Max weight per vertex exceeded at vertex %u (%f)", vertex_index, BoneWeightPerVertex[vertex_index] );
			}
		SAFE_DEL_C ( BoneWeightCountPerVertex );
		SAFE_DEL_C ( BoneWeightPerVertex );

		// Start loading the Bones
		Descriptor->BoneCount = AssimpMesh->mNumBones;
		Descriptor->Bones = calloc ( Descriptor->BoneCount, sizeof ( struct MeshBone ) );
		Descriptor->BoneWeights = calloc ( AssimpMesh->mNumVertices * MAX_BONE_WEIGHTS_PER_VERTEX, sizeof ( float ) );
		Descriptor->BoneIndices = calloc ( AssimpMesh->mNumVertices * MAX_BONE_WEIGHTS_PER_VERTEX, sizeof ( uint32_t ) );
		unsigned *NextWeightSlot = calloc ( AssimpMesh->mNumVertices, sizeof ( unsigned ) );
		if ( ( Descriptor->Bones == NULL ) || ( Descriptor->BoneWeights == NULL ) || ( Descriptor->BoneIndices == NULL ) || ( NextWeightSlot == NULL ) )
			{
			SAFE_DEL_C ( NextWeightSlot );
			return false;
			}
		for ( unsigned BoneIndex = 0; BoneIndex < AssimpMesh->mNumBones; ++BoneIndex )
			{
			struct aiBone *AssimpBone = AssimpMesh->mBones[BoneIndex];
			if ( LoadAssimpBone ( AssimpBone, Descriptor->Bones + BoneIndex ) == false )
				{
				SAFE_DEL_C ( NextWeightSlot );
				return false;
				}

			for ( unsigned WeightIndex = 0; WeightIndex < AssimpBone->mNumWeights; ++WeightIndex )
				{
				const struct aiVertexWeight *CurrentAssimpVertexWeight = &AssimpBone->mWeights[WeightIndex];
				if ( CurrentAssimpVertexWeight->mVertexId >= Descriptor->VertexCount )
					{
					LOG_ERROR ( "Bone weight vertex id %u is out of range", CurrentAssimpVertexWeight->mVertexId );
					continue;
					}
				const unsigned Slot = NextWeightSlot[CurrentAssimpVertexWeight->mVertexId];
				if ( Slot >= MAX_BONE_WEIGHTS_PER_VERTEX )
					{
					LOG_ERROR ( "Max weight count per vertex exceeded at vertex %u", CurrentAssimpVertexWeight->mVertexId );
					continue;
					}
				Descriptor->BoneWeights[CurrentAssimpVertexWeight->mVertexId * MAX_BONE_WEIGHTS_PER_VERTEX + Slot] = CurrentAssimpVertexWeight->mWeight;
				Descriptor->BoneIndices[CurrentAssimpVertexWeight->mVertexId * MAX_BONE_WEIGHTS_PER_VERTEX + Slot] = BoneIndex;
				NextWeightSlot[CurrentAssimpVertexWeight->mVertexId]++;
				}
			}
		SAFE_DEL_C ( NextWeightSlot );
		}

	Descriptor->BoundingBox.Min.x = AssimpMesh->mAABB.mMin.x;
	Descriptor->BoundingBox.Min.y = AssimpMesh->mAABB.mMin.y;
	Descriptor->BoundingBox.Min.z = AssimpMesh->mAABB.mMin.z;

	Descriptor->BoundingBox.Max.x = AssimpMesh->mAABB.mMax.x;
	Descriptor->BoundingBox.Max.y = AssimpMesh->mAABB.mMax.y;
	Descriptor->BoundingBox.Max.z = AssimpMesh->mAABB.mMax.z;
	LOG_DEBUG ( "Loaded mesh '%s'", Descriptor->MeshName );
	return true;
	}

static bool LoadAssimpAnimation ( const struct aiAnimation *AssimpAnimation, struct AnimationDescriptor *Descriptor )
	{
	memset ( Descriptor, 0, sizeof ( struct AnimationDescriptor ) );

	Descriptor->AnimationName = strdup ( AssimpAnimation->mName.data );
	const double TicksPerSecond = AssimpAnimation->mTicksPerSecond != 0.0 ? AssimpAnimation->mTicksPerSecond : 30.0;
	Descriptor->Duration = AssimpAnimation->mDuration / TicksPerSecond;
	Descriptor->NodeTracks = calloc ( AssimpAnimation->mNumChannels, sizeof ( NodeTrack ) );
	Descriptor->NodeTrackCount = AssimpAnimation->mNumChannels;
	assert ( Descriptor->NodeTracks );
	for ( unsigned ChannelIndex = 0; ChannelIndex < AssimpAnimation->mNumChannels; ++ChannelIndex )
		{
		NodeTrack *CurrentTrack = Descriptor->NodeTracks + ChannelIndex;
		CurrentTrack->NodeName = strdup ( AssimpAnimation->mChannels[ChannelIndex]->mNodeName.data );

		CurrentTrack->Positions = calloc ( AssimpAnimation->mChannels[ChannelIndex]->mNumPositionKeys, sizeof ( NodeTrackKey ) );
		assert ( CurrentTrack->Positions );
		CurrentTrack->PositionCount = AssimpAnimation->mChannels[ChannelIndex]->mNumPositionKeys;
		for ( unsigned KeyIndex = 0; KeyIndex < AssimpAnimation->mChannels[ChannelIndex]->mNumPositionKeys; ++KeyIndex )
			{
			CurrentTrack->Positions[KeyIndex].Time = AssimpAnimation->mChannels[ChannelIndex]->mPositionKeys[KeyIndex].mTime / TicksPerSecond;
			Translate_aiVector3D ( AssimpAnimation->mChannels[ChannelIndex]->mPositionKeys[KeyIndex].mValue, &CurrentTrack->Positions[KeyIndex].Vec3Value );
			}

		CurrentTrack->Rotations = calloc ( AssimpAnimation->mChannels[ChannelIndex]->mNumRotationKeys, sizeof ( NodeTrackKey ) );
		assert ( CurrentTrack->Rotations );
		CurrentTrack->RotationCount = AssimpAnimation->mChannels[ChannelIndex]->mNumRotationKeys;
		for ( unsigned KeyIndex = 0; KeyIndex < AssimpAnimation->mChannels[ChannelIndex]->mNumRotationKeys; ++KeyIndex )
			{
			CurrentTrack->Rotations[KeyIndex].Time = AssimpAnimation->mChannels[ChannelIndex]->mRotationKeys[KeyIndex].mTime / TicksPerSecond;
			Translate_aiQuaternion ( AssimpAnimation->mChannels[ChannelIndex]->mRotationKeys[KeyIndex].mValue, &CurrentTrack->Rotations[KeyIndex].QuatValue );
			}

		CurrentTrack->Scales = calloc ( AssimpAnimation->mChannels[ChannelIndex]->mNumScalingKeys, sizeof ( NodeTrackKey ) );
		assert ( CurrentTrack->Scales );
		CurrentTrack->ScaleCount = AssimpAnimation->mChannels[ChannelIndex]->mNumScalingKeys;
		for ( unsigned KeyIndex = 0; KeyIndex < AssimpAnimation->mChannels[ChannelIndex]->mNumScalingKeys; ++KeyIndex )
			{
			CurrentTrack->Scales[KeyIndex].Time = AssimpAnimation->mChannels[ChannelIndex]->mScalingKeys[KeyIndex].mTime / TicksPerSecond;
			Translate_aiVector3D ( AssimpAnimation->mChannels[ChannelIndex]->mScalingKeys[KeyIndex].mValue, &CurrentTrack->Scales[KeyIndex].Vec3Value );
			}
		}
	LOG_DEBUG ( "Loaded animation '%s'", Descriptor->AnimationName );
	return true;
	}

bool LoadFileWithAssimp ( const char *Filename, struct DataFileContentDescriptor *FileData )
	{
	unsigned AssimpImportFlags = aiProcess_RemoveRedundantMaterials;
	AssimpImportFlags |= aiProcess_Triangulate;
	AssimpImportFlags |= aiProcess_LimitBoneWeights;// Limit to AI_CONFIG_PP_LBW_MAX_WEIGHTS weights per bone
	AssimpImportFlags |= aiProcess_SortByPType;
	AssimpImportFlags |= aiProcess_GenNormals;
	AssimpImportFlags |= aiProcess_CalcTangentSpace;
	AssimpImportFlags |= aiProcess_ImproveCacheLocality;
	AssimpImportFlags |= aiProcess_FindInstances;// Search for duplicate meshes
	AssimpImportFlags |= aiProcess_GenBoundingBoxes;
	//AssimpImportFlags |= aiProcess_FlipUVs;
	//AssimpImportFlags |= aiProcess_OptimizeMeshes;
	//AssimpImportFlags |= aiProcess_OptimizeGraph;// Remove empty nodes

	LOG_DEBUG ( "Loading '%s'", Filename );
	const struct aiScene *AssimpScene = aiImportFile ( Filename, AssimpImportFlags );
	if ( !AssimpScene )
		{
		LOG_ERROR ( "Error while loading %s - %s\n", Filename, aiGetErrorString() );
		return false;
		}

	FileData->NodeCount = 0;
	CountAssimpNodes ( AssimpScene->mRootNode, &FileData->NodeCount );
	FileData->NodeTree = calloc ( FileData->NodeCount, sizeof ( struct MeshTreeNode ) );
	unsigned StartIndex = 0;
	LoadAssimpNode ( FileData->NodeTree, &StartIndex, -1, AssimpScene->mRootNode );

	// Assimp 5.4.3 already bakes FBX and Collada axis and unit metadata into the root transform.
	// Scene metadata still describes the source file, so applying it again doubles the correction.
	LogMeshTreeNode ( FileData->NodeTree, 0, 0 );

	if ( AssimpScene->mNumTextures != 0 )
		{
		LOG_DEBUG ( "Loading %u textures", AssimpScene->mNumTextures );
		FileData->Textures = calloc ( AssimpScene->mNumTextures, sizeof ( crTextureDescriptor ) );
		assert ( FileData->Textures );
		FileData->TextureCount = AssimpScene->mNumTextures;

		for ( unsigned TextureIndex = 0; TextureIndex < AssimpScene->mNumTextures; ++TextureIndex )
			{
			crTextureDescriptor NewDescriptor;
			if ( LoadAssimpTexture ( AssimpScene->mTextures[TextureIndex], &NewDescriptor ) == false )
				continue;
			FileData->Textures[TextureIndex] = NewDescriptor;
			}
		}

	if ( AssimpScene->mNumMaterials != 0 )
		{
		LOG_DEBUG ( "Loading %u materials", AssimpScene->mNumMaterials );
		FileData->Materials = calloc ( AssimpScene->mNumMaterials, sizeof ( struct MaterialDescriptor ) );
		assert ( FileData->Materials );
		FileData->MaterialCount = AssimpScene->mNumMaterials;
		for ( unsigned MaterialIndex = 0; MaterialIndex < AssimpScene->mNumMaterials; ++MaterialIndex )
			{
			struct MaterialDescriptor NewDescriptor;
			if ( LoadAssimpMaterial ( AssimpScene->mMaterials[MaterialIndex], &NewDescriptor ) == false )
				continue;

			// Check for embedded textures
			for ( unsigned TextureTypeIterator = 0; TextureTypeIterator < ( unsigned ) MatTexType_Count; ++TextureTypeIterator )
				{
				if ( NewDescriptor.Textures[TextureTypeIterator].TextureName == NULL )
					continue;
				if ( ( strlen ( NewDescriptor.Textures[TextureTypeIterator].TextureName ) >= 2 ) && ( NewDescriptor.Textures[TextureTypeIterator].TextureName[0] == '*' ) ) // If it mentions an embedded texture...
					{
					int TextureIndex = atoi ( NewDescriptor.Textures[TextureTypeIterator].TextureName + 1 );
					free ( NewDescriptor.Textures[TextureTypeIterator].TextureName );
					NewDescriptor.Textures[TextureTypeIterator].TextureName = NULL;
					if ( ( TextureIndex >= 0 ) && ( ( unsigned ) TextureIndex < FileData->TextureCount ) && ( FileData->Textures[TextureIndex].TextureName != NULL ) )
						NewDescriptor.Textures[TextureTypeIterator].TextureName = strdup ( FileData->Textures[TextureIndex].TextureName );
					}
				}
			FileData->Materials[MaterialIndex] = NewDescriptor;
			}
		}

	if ( AssimpScene->mNumMeshes != 0 )
		{
		LOG_DEBUG ( "Loading %u meshes", AssimpScene->mNumMeshes );
		FileData->Meshes = calloc ( AssimpScene->mNumMeshes, sizeof ( struct MeshDescriptor ) );
		assert ( FileData->Meshes );
		FileData->MeshCount = AssimpScene->mNumMeshes;

		for ( unsigned MeshIndex = 0; MeshIndex < AssimpScene->mNumMeshes; ++MeshIndex )
			{
			if ( LoadAssimpMesh ( AssimpScene->mMeshes[MeshIndex], &FileData->Meshes[MeshIndex] ) == false )
				goto on_error;

			// Match mesh Bones to nodes
			for ( unsigned BoneIndex = 0; BoneIndex < FileData->Meshes[MeshIndex].BoneCount; ++BoneIndex )
				{
				for ( unsigned NodeIndex = 0; NodeIndex < FileData->NodeCount; ++NodeIndex )
					{
					if ( strcmp ( FileData->NodeTree[NodeIndex].NodeName, FileData->Meshes[MeshIndex].Bones[BoneIndex].BoneName ) == 0 )
						{
						FileData->Meshes[MeshIndex].Bones[BoneIndex].NodeIndex = NodeIndex;
						break;
						}
					}
				if ( FileData->Meshes[MeshIndex].Bones[BoneIndex].NodeIndex == -1 )
					{
					LOG_ERROR ( "Unable to find matching node for bone '%s'", FileData->Meshes[MeshIndex].Bones[BoneIndex].BoneName );
					continue;
					}
				}
			const unsigned MaterialIndex = AssimpScene->mMeshes[MeshIndex]->mMaterialIndex;
			if ( ( MaterialIndex < FileData->MaterialCount ) && ( FileData->Materials[MaterialIndex].MaterialName != NULL ) )
				FileData->Meshes[MeshIndex].MaterialName = strdup ( FileData->Materials[MaterialIndex].MaterialName );
			else
				FileData->Meshes[MeshIndex].MaterialName = NULL;
			}
		}

	if ( AssimpScene->mNumAnimations != 0 )
		{
		LOG_DEBUG ( "Loading %u animations", AssimpScene->mNumAnimations );
		FileData->Animations = calloc ( AssimpScene->mNumAnimations, sizeof ( struct AnimationDescriptor ) );
		assert ( FileData->Animations );
		FileData->AnimationCount = AssimpScene->mNumAnimations;

		for ( unsigned animation_index = 0; animation_index < AssimpScene->mNumAnimations; ++animation_index )
			{
			if ( LoadAssimpAnimation ( AssimpScene->mAnimations[animation_index], &FileData->Animations[animation_index] ) == false )
				goto on_error;
			}
		}

	LOG_DEBUG ( "Finished loading '%s'", Filename );
	aiReleaseImport ( AssimpScene );
	return true;

on_error:
	LOG_ERROR ( "Error while loading '%s'", Filename );
	DestroyDataFileContentDescriptor ( FileData );
	if ( AssimpScene != NULL )
		aiReleaseImport ( AssimpScene );
	return false;
	}

bool IsValidModelFile ( const char *Filename )
	{
	static const char **valid_model_extensions = NULL;
	static char *full_string_block = NULL;
	static unsigned valid_model_extension_count = 0;

	if ( full_string_block == NULL )
		{
		// First, count how many extensions there are.
		struct aiString valid_extensions = { 0 };
		aiGetExtensionList ( &valid_extensions );
		if ( valid_extensions.length == 0 )
			return false;

		valid_model_extension_count = 1;
		char *pointer = strchr ( valid_extensions.data, ';' );
		while ( pointer != NULL )
			{
			++valid_model_extension_count;
			pointer = strchr ( pointer + 1, ';' );
			}

		// Second, reserve space in our array for all of those pointers
		valid_model_extensions = calloc ( valid_model_extension_count, sizeof ( char * ) );
		assert ( valid_model_extensions );

		// Third, instead of a gazillion tiny allocations, make one big one for all the strings.
		full_string_block = calloc ( valid_extensions.length + 1, sizeof ( char ) );
		assert ( full_string_block );
		memcpy ( full_string_block, valid_extensions.data, valid_extensions.length );

		// Fourth, add all the pointers to the array, substituting ; for NULL
		pointer = full_string_block;
		unsigned index = 0;
		while ( pointer != NULL )
			{
			valid_model_extensions[index] = pointer;
			pointer = strchr ( pointer + 1, ';' );
			if ( pointer == NULL )
				break;
			*pointer = 0;
			++pointer;
			++index;
			}
		}

	const char *extension = strrchr ( Filename, '.' );
	if ( extension == NULL )
		return false;

	for ( unsigned index = 0; index < valid_model_extension_count; ++index )
		{
#if defined (PLATFORM_COMPILER_MSVC)
		if ( PathMatchSpecA ( Filename, ( const char * ) valid_model_extensions[index] ) )
			return true;
#else
		if ( fnmatch ( ( const char * ) valid_model_extensions[index], Filename, FNM_CASEFOLD ) == 0 )
			return true;
#endif
		}
	return false;
	}
