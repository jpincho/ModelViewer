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
#include <malloc.h>

#if defined (PLATFORM_COMPILER_GNU)
// For fnmatch definition of FNM_CASEFOLD
#define _GNU_SOURCE 
#include <fnmatch.h>
#elif defined (PLATFORM_COMPILER_MSVC)
#include <shlwapi.h>
#endif

const unsigned MAX_BONE_WEIGHTS_PER_VERTEX = 4;

static MatTexType translate_aiTextureType_to_MatTexType ( const enum aiTextureType assimp_texture_type )
	{
	switch ( assimp_texture_type )
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
			return 0;
		};
	}

static enum aiTextureType translate_MaxTexType_to_aiTextureType ( const MatTexType TextureType )
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

static void translate_aiMatrix4x4_to_mat4 ( const struct aiMatrix4x4 input, mat4 *output )
	{
	math_mat4_transpose ( output, * ( ( mat4 * ) &input.a1 ) );
	}

static void translate_aiVector3D_to_vec3 ( const struct aiVector3D input, vec3 *output )
	{
	output->x = input.x;
	output->y = input.y;
	output->z = input.z;
	}

static void translate_aiQuaternion_to_quat ( const struct aiQuaternion input, vec4 *output )
	{
	output->x = input.x;
	output->y = input.y;
	output->z = input.z;
	output->w = input.w;
	}

static void count_assimp_nodes ( const struct aiNode *CurrentAssimpNode, unsigned *count )
	{
	if ( CurrentAssimpNode == NULL )
		return;

	++ ( *count );
	for ( unsigned index = 0; index < CurrentAssimpNode->mNumChildren; ++index )
		{
		count_assimp_nodes ( CurrentAssimpNode->mChildren[index], count );
		}
	}

static unsigned LoadAssimpNode ( MeshTreeNode *NodeTree, unsigned *CurrentNodeTreeSize, const int ParentIndex, const struct aiNode *CurrentAssimpNode )
	{
	unsigned NewIndex = ( *CurrentNodeTreeSize )++;
	NodeTree[NewIndex].NodeName = strdup ( CurrentAssimpNode->mName.data );
	NodeTree[NewIndex].ParentNode = ParentIndex;
	translate_aiMatrix4x4_to_mat4 ( CurrentAssimpNode->mTransformation, &NodeTree[NewIndex].DefaultTransformationMatrix );
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

static bool LoadAssimpMaterial ( const struct aiMaterial *AssimpMaterial, MaterialDescriptor *Descriptor )
	{
	memset ( Descriptor, 0, sizeof ( MaterialDescriptor ) );

	struct aiColor4D AssimpColor4;
	float FloatValue;
	struct aiString AssimpString;
	if ( aiGetMaterialString ( AssimpMaterial, AI_MATKEY_NAME, &AssimpString ) != aiReturn_SUCCESS )
		return false;

	Descriptor->MaterialName = strdup ( AssimpString.data );

	if ( aiGetMaterialColor ( AssimpMaterial, AI_MATKEY_COLOR_DIFFUSE, &AssimpColor4 ) == AI_SUCCESS )
		memcpy ( &Descriptor->Diffuse, &AssimpColor4, sizeof ( float ) * 3 );
	if ( aiGetMaterialColor ( AssimpMaterial, AI_MATKEY_COLOR_SPECULAR, &AssimpColor4 ) == AI_SUCCESS )
		memcpy ( &Descriptor->Specular, &AssimpColor4, sizeof ( float ) * 3 );
	if ( aiGetMaterialColor ( AssimpMaterial, AI_MATKEY_COLOR_EMISSIVE, &AssimpColor4 ) == AI_SUCCESS )
		memcpy ( &Descriptor->Emissive, &AssimpColor4, sizeof ( float ) * 3 );
	if ( aiGetMaterialFloat ( AssimpMaterial, AI_MATKEY_SHININESS, &FloatValue ) == AI_SUCCESS )
		Descriptor->Shininess = MIN ( 1.0f, FloatValue );

	for ( unsigned TextureTypeIterator = 0; TextureTypeIterator < ( unsigned ) MatTexType_Count; ++TextureTypeIterator )
		{
		enum aiTextureType AssimpType = translate_MaxTexType_to_aiTextureType ( TextureTypeIterator );
		if ( aiGetMaterialString ( AssimpMaterial, AI_MATKEY_TEXTURE ( AssimpType, 0 ), &AssimpString ) != AI_SUCCESS )
			continue;
		Descriptor->Textures[TextureTypeIterator].TextureName = strdup ( AssimpString.data );
		int hwrap, vwrap;
		if ( ( aiGetMaterialInteger ( AssimpMaterial, AI_MATKEY_MAPPINGMODE_U ( AssimpType, 0 ), &hwrap ) == AI_SUCCESS ) &&
		        ( aiGetMaterialInteger ( AssimpMaterial, AI_MATKEY_MAPPINGMODE_V ( AssimpType, 0 ), &vwrap ) == AI_SUCCESS ) )
			{
			Descriptor->Textures[TextureTypeIterator].WrapSettings.Horizontal = hwrap;
			Descriptor->Textures[TextureTypeIterator].WrapSettings.Vertical = vwrap;
			}
		}

	LOG_DEBUG ( "Loaded material '%s'", Descriptor->MaterialName );
	return true;
	}

static bool LoadAssimpTexture ( const struct aiTexture *AssimpTexture, crTextureDescriptor *Descriptor )
	{
	Descriptor->Type = crTextureType_Texture2D;
	if ( AssimpTexture->mFilename.length != 0 )
		Descriptor->TextureName = strdup ( AssimpTexture->mFilename.data );
	ASSERT_FAIL ( Descriptor->TextureName != NULL );

	if ( ( strcmp ( AssimpTexture->achFormatHint, "rgba8888" ) == 0 ) ||
	        ( strcmp ( AssimpTexture->achFormatHint, "argb8888" ) == 0 ) ||
	        ( strcmp ( AssimpTexture->achFormatHint, "rgba5650" ) == 0 ) ||
	        ( strcmp ( AssimpTexture->achFormatHint, "rgba0010" ) == 0 ) )
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
		else if ( strcmp ( AssimpTexture->achFormatHint, "rgba5650" ) == 0 )
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

static bool LoadAssimpBone ( const struct aiBone *AssimpBone, MeshBone *Descriptor )
	{
	Descriptor->BoneName = strdup ( AssimpBone->mName.data );
	Descriptor->NodeIndex = -1;
	translate_aiMatrix4x4_to_mat4 ( AssimpBone->mOffsetMatrix, &Descriptor->ModelToBoneMatrix );
	return true;
	}

static bool LoadAssimpMesh ( const struct aiMesh *AssimpMesh, MeshDescriptor *Descriptor )
	{
	memset ( Descriptor, 0, sizeof ( MeshDescriptor ) );

	//NewMeshDescriptor.Primitive = Translate((aiPrimitiveType)AssimpMesh->mPrimitiveTypes);

	Descriptor->Primitive = crTriangleList;
	Descriptor->MeshName = strdup ( AssimpMesh->mName.data );
	Descriptor->Positions = calloc ( sizeof ( vec3 ), AssimpMesh->mNumVertices );
	ASSERT_FAIL ( Descriptor->Positions );
	memcpy ( Descriptor->Positions, AssimpMesh->mVertices, sizeof ( vec3 ) * AssimpMesh->mNumVertices );
	Descriptor->VertexCount = AssimpMesh->mNumVertices;

	if ( ( AssimpMesh->mBitangents != NULL ) && ( AssimpMesh->mTangents != NULL ) )
		{
		Descriptor->Bitangents = calloc ( sizeof ( vec3 ), AssimpMesh->mNumVertices );
		assert ( Descriptor->Bitangents );
		memcpy ( Descriptor->Bitangents, AssimpMesh->mBitangents, sizeof ( vec3 ) * AssimpMesh->mNumVertices );
		Descriptor->Tangents = calloc ( sizeof ( vec3 ), AssimpMesh->mNumVertices );
		assert ( Descriptor->Tangents );
		memcpy ( Descriptor->Tangents, AssimpMesh->mTangents, sizeof ( vec3 ) * AssimpMesh->mNumVertices );
		}
	if ( AssimpMesh->mNormals != NULL )
		{
		Descriptor->Normals = calloc ( sizeof ( vec3 ), AssimpMesh->mNumVertices );
		assert ( Descriptor->Normals );
		memcpy ( Descriptor->Normals, AssimpMesh->mNormals, sizeof ( vec3 ) * AssimpMesh->mNumVertices );
		}
	if ( ( AssimpMesh->mTextureCoords != NULL ) && ( AssimpMesh->mTextureCoords[0] != NULL ) )
		{
		Descriptor->TexCoords = calloc ( sizeof ( vec2 ), AssimpMesh->mNumVertices );
		assert ( Descriptor->TexCoords );
		for ( unsigned vertex_index = 0; vertex_index < AssimpMesh->mNumVertices; ++vertex_index )
			{
			Descriptor->TexCoords[vertex_index].x = AssimpMesh->mTextureCoords[0][vertex_index].x;
			Descriptor->TexCoords[vertex_index].y = AssimpMesh->mTextureCoords[0][vertex_index].y;
			}
		}

	Descriptor->Indices = calloc ( sizeof ( uint32_t ), AssimpMesh->mNumFaces * 3 );
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
		Descriptor->Bones = calloc ( Descriptor->BoneCount, sizeof ( MeshBone ) );
		Descriptor->BoneWeights = calloc ( AssimpMesh->mNumVertices * MAX_BONE_WEIGHTS_PER_VERTEX, sizeof ( float ) );
		Descriptor->BoneIndices = calloc ( AssimpMesh->mNumVertices * MAX_BONE_WEIGHTS_PER_VERTEX, sizeof ( uint32_t ) );
		for ( unsigned BoneIndex = 0; BoneIndex < AssimpMesh->mNumBones; ++BoneIndex )
			{
			struct aiBone *AssimpBone = AssimpMesh->mBones[BoneIndex];
			if ( LoadAssimpBone ( AssimpBone, Descriptor->Bones + BoneIndex ) == false )
				return false;

			for ( unsigned WeightIndex = 0; WeightIndex < AssimpBone->mNumWeights; ++WeightIndex )
				{
				const struct aiVertexWeight *CurrentAssimpVertexWeight = &AssimpBone->mWeights[WeightIndex];
				assert ( CurrentAssimpVertexWeight->mVertexId <= Descriptor->VertexCount );
				bool Found = false;

				for ( unsigned VertexWeightIndex = 0; VertexWeightIndex < MAX_BONE_WEIGHTS_PER_VERTEX; ++VertexWeightIndex )
					{
					if ( ( Descriptor->BoneWeights[CurrentAssimpVertexWeight->mVertexId * MAX_BONE_WEIGHTS_PER_VERTEX + VertexWeightIndex] == 0.0f ) &&
					        ( Descriptor->BoneIndices[CurrentAssimpVertexWeight->mVertexId * MAX_BONE_WEIGHTS_PER_VERTEX + VertexWeightIndex] == 0 ) )
						{
						Found = true;
						Descriptor->BoneWeights[CurrentAssimpVertexWeight->mVertexId * MAX_BONE_WEIGHTS_PER_VERTEX + VertexWeightIndex] = AssimpBone->mWeights[WeightIndex].mWeight;
						Descriptor->BoneIndices[CurrentAssimpVertexWeight->mVertexId * MAX_BONE_WEIGHTS_PER_VERTEX + VertexWeightIndex] = BoneIndex;
						break;
						}
					}
				assert ( Found == true );
				}

			// TODO make sure the weights all add up to 1.0
			}
		}
	LOG_DEBUG ( "Loaded mesh '%s'", Descriptor->MeshName );
	return true;
	}

static bool LoadAssimpAnimation ( const struct aiAnimation *AssimpAnimation, AnimationDescriptor *Descriptor )
	{
	memset ( Descriptor, 0, sizeof ( AnimationDescriptor ) );

	Descriptor->AnimationName = strdup ( AssimpAnimation->mName.data );
	Descriptor->Duration = AssimpAnimation->mDuration / ( AssimpAnimation->mTicksPerSecond != 0.0 ? AssimpAnimation->mTicksPerSecond : 30.0 );
	Descriptor->NodeTracks = calloc ( AssimpAnimation->mNumChannels, sizeof ( NodeTrack ) );
	Descriptor->NodeTrackCount = AssimpAnimation->mNumChannels;
	assert ( Descriptor->NodeTracks );
	for ( unsigned channel_index = 0; channel_index < AssimpAnimation->mNumChannels; ++channel_index )
		{
		NodeTrack *CurrentTrack = Descriptor->NodeTracks + channel_index;
		CurrentTrack->NodeName = strdup ( AssimpAnimation->mChannels[channel_index]->mNodeName.data );

		CurrentTrack->Positions = calloc ( AssimpAnimation->mChannels[channel_index]->mNumPositionKeys, sizeof ( NodeTrackKey ) );
		assert ( CurrentTrack->Positions );
		CurrentTrack->PositionCount = AssimpAnimation->mChannels[channel_index]->mNumPositionKeys;
		for ( unsigned KeyIndex = 0; KeyIndex < AssimpAnimation->mChannels[channel_index]->mNumPositionKeys; ++KeyIndex )
			{
			CurrentTrack->Positions[KeyIndex].time = AssimpAnimation->mChannels[channel_index]->mPositionKeys[KeyIndex].mTime / AssimpAnimation->mTicksPerSecond;
			translate_aiVector3D_to_vec3 ( AssimpAnimation->mChannels[channel_index]->mPositionKeys[KeyIndex].mValue, &CurrentTrack->Positions[KeyIndex].Vec3Value );
			}

		CurrentTrack->Rotations = calloc ( AssimpAnimation->mChannels[channel_index]->mNumRotationKeys, sizeof ( NodeTrackKey ) );
		assert ( CurrentTrack->Rotations );
		CurrentTrack->RotationCount = AssimpAnimation->mChannels[channel_index]->mNumRotationKeys;
		for ( unsigned KeyIndex = 0; KeyIndex < AssimpAnimation->mChannels[channel_index]->mNumRotationKeys; ++KeyIndex )
			{
			CurrentTrack->Rotations[KeyIndex].time = AssimpAnimation->mChannels[channel_index]->mRotationKeys[KeyIndex].mTime / AssimpAnimation->mTicksPerSecond;
			translate_aiQuaternion_to_quat ( AssimpAnimation->mChannels[channel_index]->mRotationKeys[KeyIndex].mValue, &CurrentTrack->Rotations[KeyIndex].QuatValue );
			}

		CurrentTrack->Scales = calloc ( AssimpAnimation->mChannels[channel_index]->mNumScalingKeys, sizeof ( NodeTrackKey ) );
		assert ( CurrentTrack->Scales );
		CurrentTrack->ScaleCount = AssimpAnimation->mChannels[channel_index]->mNumScalingKeys;
		for ( unsigned KeyIndex = 0; KeyIndex < AssimpAnimation->mChannels[channel_index]->mNumScalingKeys; ++KeyIndex )
			{
			CurrentTrack->Scales[KeyIndex].time = AssimpAnimation->mChannels[channel_index]->mScalingKeys[KeyIndex].mTime / AssimpAnimation->mTicksPerSecond;
			translate_aiVector3D_to_vec3 ( AssimpAnimation->mChannels[channel_index]->mScalingKeys[KeyIndex].mValue, &CurrentTrack->Scales[KeyIndex].Vec3Value );
			}
		}
	LOG_DEBUG ( "Loaded animation '%s'", Descriptor->AnimationName );
	return true;
	}

bool LoadFileWithAssimp ( const char *Filename, DataFileContentDescriptor *FileData )
	{
	unsigned AssimpImportFlags = aiProcess_RemoveRedundantMaterials;
	AssimpImportFlags |= aiProcess_Triangulate;
	AssimpImportFlags |= aiProcess_LimitBoneWeights;// Limit to AI_CONFIG_PP_LBW_MAX_WEIGHTS weights per bone
	//AssimpImportFlags |= aiProcess_SortByPType;
	AssimpImportFlags |= aiProcess_GenNormals;
	AssimpImportFlags |= aiProcess_CalcTangentSpace;
	AssimpImportFlags |= aiProcess_ImproveCacheLocality;
	AssimpImportFlags |= aiProcess_FindInstances;// Search for duplicate meshes
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
	count_assimp_nodes ( AssimpScene->mRootNode, &FileData->NodeCount );
	FileData->NodeTree = calloc ( FileData->NodeCount, sizeof ( MeshTreeNode ) );
	unsigned StartIndex = 0;
	LoadAssimpNode ( FileData->NodeTree, &StartIndex, -1, AssimpScene->mRootNode );

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
		FileData->Materials = calloc ( AssimpScene->mNumMaterials, sizeof ( MaterialDescriptor ) );
		assert ( FileData->Materials );
		FileData->MaterialCount = AssimpScene->mNumMaterials;
		for ( unsigned MaterialIndex = 0; MaterialIndex < AssimpScene->mNumMaterials; ++MaterialIndex )
			{
			MaterialDescriptor NewDescriptor;
			if ( LoadAssimpMaterial ( AssimpScene->mMaterials[MaterialIndex], &NewDescriptor ) == false )
				continue;

			if ( NewDescriptor.MaterialName == NULL )
				{
				//StringUtils::Format(NewMaterialDescriptor.Name, "__Material%03u__", MatIndex);
				//LOG_WARNING("No material name provided. Assigning '%s'", NewMaterialDescriptor.Name.c_str());
				}

			// Check for embedded textures
			for ( unsigned TextureTypeIterator = 0; TextureTypeIterator < ( unsigned ) MatTexType_Count; ++TextureTypeIterator )
				{
				if ( NewDescriptor.Textures[TextureTypeIterator].TextureName == NULL )
					continue;
				if ( ( strlen ( NewDescriptor.Textures[TextureTypeIterator].TextureName ) >= 2 ) && ( NewDescriptor.Textures[TextureTypeIterator].TextureName[0] == '*' ) ) // If it mentions an embedded texture...
					{
					int TextureIndex = atoi ( NewDescriptor.Textures[TextureTypeIterator].TextureName + 1 );
					free ( NewDescriptor.Textures[TextureTypeIterator].TextureName );
					NewDescriptor.Textures[TextureTypeIterator].TextureName = strdup ( FileData->Textures[TextureIndex].TextureName );
					}
				}
			FileData->Materials[MaterialIndex] = NewDescriptor;
			}
		}

	if ( AssimpScene->mNumMeshes != 0 )
		{
		LOG_DEBUG ( "Loading %u meshes", AssimpScene->mNumMeshes );
		FileData->Meshes = calloc ( AssimpScene->mNumMeshes, sizeof ( MeshDescriptor ) );
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
			FileData->Meshes[MeshIndex].MaterialName = strdup ( FileData->Materials[AssimpScene->mMeshes[MeshIndex]->mMaterialIndex].MaterialName );
			}
		}

	if ( AssimpScene->mNumAnimations != 0 )
		{
		LOG_DEBUG ( "Loading %u animations", AssimpScene->mNumAnimations );
		FileData->Animations = calloc ( AssimpScene->mNumAnimations, sizeof ( AnimationDescriptor ) );
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
