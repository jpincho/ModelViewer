cmake_minimum_required(VERSION 3.10)
project ( ModelViewer )
set ( CMAKE_C_STANDARD 11 )
set ( CMAKE_C_STANDARD_REQUIRED ON )

list ( APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/ExternalModules/CMakeUtils )
include ( target_warning_options )

include_directories (ExternalModules)
add_subdirectory ( ExternalModules/Platform )
add_subdirectory ( ExternalModules/Math )
add_subdirectory ( ExternalModules/C_CrossRenderer )
add_subdirectory ( ShaderCreator )

find_package ( assimp REQUIRED )

file ( GLOB SOURCE_FILES "*.c" "*.h" "ResourceLoading/*.h" "ResourceLoading/*.c" )
add_executable ( ModelViewer ${SOURCE_FILES} )
target_link_libraries ( ModelViewer PUBLIC Platform C_CrossRenderer Math Platform ShaderCreator )
target_link_libraries ( ModelViewer PRIVATE ${ASSIMP_LIBRARIES} )
target_include_directories ( ModelViewer PRIVATE ${ASSIMP_INCLUDE_DIRS})
target_compile_definitions ( ModelViewer PRIVATE DATA_PATH="${CMAKE_CURRENT_LIST_DIR}/Data/")
 