#include "SkeletonRenderData.h"
#include "Utils.h"
#include <Platform/Logger.h>
#include <string.h>
#include <ShaderGraph/ShaderGraph.h>
#include <ShaderGraph/GLSLGenerator/GLSLGenerator.h>

static crShaderHandle BuildSkeletonShader ( void )
	{
	crShaderHandle ShaderHandle = crShaderHandle_Invalid;
	crShaderCode ShaderCode = {0};

	sgGraphHandle Graph = sgCreate();
	if ( Graph == sgGraphHandle_Invalid )
		{
		LOG_ERROR ( "Failed to create graph" );
		goto End;
		}

	sgNodeHandle PositionAttribute = sgAddAttribute ( Graph, Stringify_ShaderAttributeType ( scAttribute_Position ), sgValueType_Vec3 );
	sgNodeHandle Model = sgAddUniform ( Graph, "ModelMatrix", sgValueType_Mat4 );
	sgNodeHandle View = sgAddUniform ( Graph, "ViewMatrix", sgValueType_Mat4 );
	sgNodeHandle Projection = sgAddUniform ( Graph, "ProjectionMatrix", sgValueType_Mat4 );
	sgNodeHandle Position = sgAddVertexPosition ( Graph );
	sgNodeHandle Vec3ToVec4Node = sgAddVec3ToVec4 ( Graph );
	sgNodeHandle WConstant = sgAddConstantFloat ( Graph, 1.0f );
	if ( ( PositionAttribute == sgNodeHandle_Invalid ) ||
	        ( Model == sgNodeHandle_Invalid ) ||
	        ( View == sgNodeHandle_Invalid ) ||
	        ( Projection == sgNodeHandle_Invalid ) ||
	        ( Position == sgNodeHandle_Invalid ) ||
	        ( Vec3ToVec4Node == sgNodeHandle_Invalid ) ||
	        ( WConstant == sgNodeHandle_Invalid ) )
		{
		LOG_ERROR ( "Failed to add nodes" );
		goto End;
		}

	// Vec3ToVec4
	if ( ( sgAddConnection ( Graph, PositionAttribute, 0, Vec3ToVec4Node, 0 ) == false ) ||
	        ( sgAddConnection ( Graph, WConstant, 0, Vec3ToVec4Node, 1 ) == false ) )
		{
		LOG_ERROR ( "Failed to add connection" );
		goto End;
		}

	// Form MVP
	sgNodeHandle MVPHandle = sgAddFormMVP ( Graph, Model, View, Projection );
	sgNodeHandle Mult1 = sgAddMultiplicationTyped ( Graph, sgValueType_Mat4, sgValueType_Mat4 );
	sgNodeHandle Mult2 = sgAddMultiplicationTyped ( Graph, sgValueType_Mat4, sgValueType_Mat4 );
	sgNodeHandle Mult3 = sgAddMultiplicationTyped ( Graph, sgValueType_Mat4, sgValueType_Vec4 );
	sgNodeHandle Mult = sgAddMultiplication ( Graph );
	if ( ( Mult1 == sgNodeHandle_Invalid ) || ( Mult2 == sgNodeHandle_Invalid ) || ( Mult3 == sgNodeHandle_Invalid ) )
		{
		LOG_ERROR ( "Failed to add multiplication nodes" );
		goto End;
		}

	if ( ( sgAddConnection ( Graph, Vec3ToVec4Node, 0, Mult3, 1 ) == false ) ||
	        ( sgAddConnection ( Graph, MVPHandle, 0, Mult3, 0 ) == false ) ||
	        ( sgAddConnection ( Graph, Mult3, 0, Position, 0 ) == false ) )
		{
		LOG_ERROR ( "Failed to add connection" );
		goto End;
		}

	sgNodeHandle FragmentColorHandle = sgAddFragmentColor ( Graph );
	sgNodeHandle ConstantColorHandle = sgAddConstantVec4 ( Graph, 0.0f, 1.0f, 0.0f, 1.0f );
	if ( ( FragmentColorHandle == sgNodeHandle_Invalid ) || ( ConstantColorHandle == sgNodeHandle_Invalid ) )
		{
		LOG_ERROR ( "Failed to add nodes" );
		goto End;
		}
	if ( sgAddConnection ( Graph, ConstantColorHandle, 0, FragmentColorHandle, 0 ) == false )
		{
		LOG_ERROR ( "Failed to add connection" );
		goto End;
		}

	if ( sgValidateGraph ( Graph ) == false )
		{
		LOG_ERROR ( "Failed to validate graph" );
		goto End;
		}

	if ( sgGenerateGLSL ( Graph, sgGetDefaultGLSLGeneratorOptions(), &ShaderCode.VertexShader, &ShaderCode.FragmentShader ) == false )
		{
		LOG_ERROR ( "Failed to compile graph to shader code" );
		goto End;
		}
	ShaderHandle = crCreateShader ( ShaderCode );
	if ( ShaderHandle == crShaderHandle_Invalid )
		{
		LOG_ERROR ( "Failed to create shader" );
		goto End;
		}
End:
	SAFE_DEL_C ( ShaderCode.VertexShader );
	SAFE_DEL_C ( ShaderCode.FragmentShader );
	if ( Graph != sgGraphHandle_Invalid )
		sgDestroy ( Graph );
	return ShaderHandle;
	}

bool BuildSkeletonRenderData ( struct SkeletonRenderData *RenderData, const struct ModelRenderData *InputModelRenderData )
	{
	DestroySkeletonRenderData ( RenderData );

	// I need at least 2 nodes to make a bone
	if ( InputModelRenderData->OriginalModelData->NodeCount < 2 )
		return true;
	crShaderHandle SkeletonShader = BuildSkeletonShader();
	if ( SkeletonShader == crShaderHandle_Invalid )
		{
		LOG_ERROR ( "Unable to build skeleton shader" );
		return false;
		}

	const unsigned VertexCount = ( InputModelRenderData->OriginalModelData->NodeCount ) * 2;
	crShaderBufferDescriptor BufferDescriptor = { 0 };
	BufferDescriptor.AccessType = crShaderBufferAccessType_Dynamic;
	BufferDescriptor.BufferType = crShaderBufferType_Array;
	BufferDescriptor.Data = NULL;
	BufferDescriptor.DataSize = sizeof ( vec3 ) * VertexCount;
	BufferDescriptor.Capacity = BufferDescriptor.DataSize;
	crShaderBufferHandle Positions = crCreateShaderBuffer ( BufferDescriptor );
	if ( Positions == crShaderBufferHandle_Invalid )
		{
		LOG_ERROR ( "Unable to create skeleton vertex buffer" );
		crDeleteShader ( SkeletonShader );
		return false;
		}

	crSetRenderCommandShader ( &RenderData->SkeletonCommand, SkeletonShader );
	crShaderBufferDataStream PositionStream = { 0 };
	PositionStream.BufferHandle = Positions;
	PositionStream.ComponentsPerElement = 3;
	PositionStream.ComponentType = crShaderBufferComponentType_Float;
	PositionStream.NormalizeData = false;
	PositionStream.PerInstance = false;
	PositionStream.StartOffset = 0;
	PositionStream.Stride = sizeof ( vec3 );
	if ( crSetRenderCommandShaderBufferBinding ( &RenderData->SkeletonCommand, crGetShaderAttributeHandle ( SkeletonShader, "a_Position" ), PositionStream ) == false )
		{
		LOG_ERROR ( "Unable to bind skeleton positions" );
		crDeleteShader ( SkeletonShader );
		SAFE_DELETE_SHADER_BUFFER ( Positions );
		crDestroyRenderCommand ( &RenderData->SkeletonCommand );
		return false;
		}

	RenderData->SkeletonCommand.Primitive = crLineList;
	RenderData->SkeletonCommand.StartVertex = 0;
	RenderData->SkeletonCommand.VertexCount = VertexCount;
	RenderData->SkeletonCommand.InstanceCount = 1;
	crSetRenderStateToDefault ( &RenderData->SkeletonCommand.State );
	RenderData->SkeletonCommand.State.Blending.Enabled = false;
	RenderData->SkeletonCommand.State.DepthTest.Enabled = false;
	RenderData->SkeletonCommand.State.Culling.Enabled = false;
	RenderData->SkeletonPositions = Positions;
	RenderData->ModelRenderData = InputModelRenderData;
	return true;

OnError:
	DestroySkeletonRenderData ( RenderData );
	return false;
	}

void UpdateSkeletonRenderDataWithCamera ( struct SkeletonRenderData *RenderData, const struct CameraData *Camera )
	{
	if ( ( RenderData == NULL ) || ( Camera == NULL ) || ( RenderData->SkeletonCommand.VertexCount == 0 ) )
		return;

	crSetRenderCommandUniformMatrix4Value ( &RenderData->SkeletonCommand, crGetShaderUniformHandle ( RenderData->SkeletonCommand.Shader, "u_ViewMatrix" ), Camera->CachedValues.ViewMatrix );
	crSetRenderCommandUniformMatrix4Value ( &RenderData->SkeletonCommand, crGetShaderUniformHandle ( RenderData->SkeletonCommand.Shader, "u_ProjectionMatrix" ), Camera->CachedValues.ProjectionMatrix );
	}

void UpdateSkeletonRenderData ( struct SkeletonRenderData *RenderData )
	{
	if ( ( RenderData == NULL ) || ( RenderData->ModelRenderData == NULL ) || ( RenderData->SkeletonPositions == crShaderBufferHandle_Invalid ) )
		return;

	if ( ( RenderData->ModelRenderData->OriginalModelData == NULL ) || ( RenderData->ModelRenderData->OriginalModelData->NodeCount == 0 ) || ( RenderData->ModelRenderData->OriginalModelData->NodeTree == NULL ) )
		return;

	mat4 *WorldMatrices = NULL;
	const bool UseAnimatedNodes = ( RenderData->ModelRenderData->Animator.OutputData.NodeMatrices != NULL ) &&
	                              ( RenderData->ModelRenderData->Animator.InputData.NodeCount == RenderData->ModelRenderData->OriginalModelData->NodeCount );
	if ( UseAnimatedNodes )
		{
		WorldMatrices = RenderData->ModelRenderData->Animator.OutputData.NodeMatrices;
		}
	else
		{
		WorldMatrices = RenderData->ModelRenderData->OriginalModelData->MeshNodeTreeWorldMatrices;
		}

	vec3 *Positions = crMapShaderBuffer ( RenderData->SkeletonPositions, crShaderBufferMapAccessType_WriteOnly );
	if ( Positions == NULL )
		{
		LOG_ERROR ( "Failed to map skeleton vertex buffer" );
		return;
		}

	// Node animation poses this hierarchy. A node with no parent is a root, and there can be more than one.
	unsigned Vertex = 0;
	const unsigned VertexCapacity = RenderData->ModelRenderData->OriginalModelData->NodeCount * 2;
	for ( unsigned NodeIndex = 0; NodeIndex < RenderData->ModelRenderData->OriginalModelData->NodeCount; ++NodeIndex )
		{
		const int Parent = RenderData->ModelRenderData->OriginalModelData->NodeTree[NodeIndex].ParentNode;
		if ( ( Parent < 0 ) || ( ( unsigned ) Parent >= RenderData->ModelRenderData->OriginalModelData->NodeCount ) )
			continue;
		if ( ( Vertex + 2 ) > VertexCapacity )
			break;

		math_vec4_to_vec3 ( WorldMatrices[Parent].vectors[3], &Positions[Vertex] );
		++Vertex;
		math_vec4_to_vec3 ( WorldMatrices[NodeIndex].vectors[3], &Positions[Vertex] );
		++Vertex;
		}
	crUnmapShaderBuffer ( RenderData->SkeletonPositions );

	RenderData->SkeletonCommand.StartVertex = 0;
	RenderData->SkeletonCommand.VertexCount = Vertex;
	crSetRenderCommandUniformMatrix4Value ( &RenderData->SkeletonCommand, crGetShaderUniformHandle ( RenderData->SkeletonCommand.Shader, "u_ModelMatrix" ), RenderData->ModelRenderData->InstanceMatrix );
	}

void RenderSkeleton ( const struct SkeletonRenderData *RenderData )
	{
	if ( RenderData->SkeletonCommand.VertexCount != 0 )
		crRunCommand ( RenderData->SkeletonCommand );
	}

void DestroySkeletonRenderData ( struct SkeletonRenderData *RenderData )
	{
	SAFE_DELETE_SHADER_BUFFER ( RenderData->SkeletonPositions );
	SAFE_DELETE_SHADER ( RenderData->SkeletonCommand.Shader );
	crDestroyRenderCommand ( &RenderData->SkeletonCommand );
	memset ( RenderData, 0, sizeof ( struct SkeletonRenderData ) );
	}
