cmake_minimum_required(VERSION 3.10)
project ( ShaderCreator )
set ( CMAKE_C_STANDARD 11 )
set ( CMAKE_C_STANDARD_REQUIRED ON )

file ( GLOB SHADER_CREATOR_SOURCES "*.c" "*.h")
add_library ( ShaderCreator ${SHADER_CREATOR_SOURCES} )
 