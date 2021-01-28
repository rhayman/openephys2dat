cmake_minimum_required(VERSION 3.0)
project( hdf2dat VERSION 1.0.1 DESCRIPTION "CLI for processing HDF5 data")

# Compiler stuff
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Debug)
# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )

# ---------- HDF5 -------------
# where the HDF5 installation lives
set ( HDF5_USE_STATIC_LIBRARIES OFF )
find_package( HDF5 REQUIRED CXX )
if ( HDF5_FOUND )
	include_directories( ${HDF5_CXX_INCLUDE_DIRS} ${HDF5_C_INCLUDE_DIRS})
endif()

# project headers
include_directories(include  include/include/include/)

link_directories(build /usr/include/hdf5/serial /usr/local/include)

set( SOURCES src/main.cpp )

add_executable( hdf2dat ${SOURCES} )
target_link_libraries( hdf2dat 
						${HDF5_CXX_LIBRARIES}
						${HDF5_C_LIBRARIES}  
						/usr/local/lib/libcnpy.so
						/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_cpp.so
						/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.so)
