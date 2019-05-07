cmake_minimum_required(VERSION 3.0)
project( hdf2dat VERSION 1.0.1 DESCRIPTION "GUI for displaying and processing HDF5 data")

# Compiler stuff
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Debug)
# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )


# ----------- wxwidgets ---------
set( wxWidgets_CONFIG_OPTIONS --toolkit=gtk2 )
find_package( wxWidgets REQUIRED std aui adv core base )
if( wxWidgets_FOUND )
	set( wxWidgets_USE_STATIC ON )
	set( wxWidgets_USE_UNICODE ON )
	set( wxWidgets_CXX_FLAGS )
	include( ${wxWidgets_USE_FILE} )
endif()

# ----------- gsl --------------
# find_package( GSL REQUIRED )
# if ( GSL_FOUND )
# 	include_directories( ${GLS_INCLUDE_DIRS} )
# endif()
# ----------- boost -------------
find_package( Boost 1.36.0 REQUIRED filesystem )
if( Boost_FOUND )
	include_directories( ${Boost_INCLUDE_DIRS} )
endif()

# ---------- HDF5 -------------
# where the HDF5 installation lives
set ( HDF5_USE_STATIC_LIBRARIES OFF )
find_package( HDF5 REQUIRED CXX )
if ( HDF5_FOUND )
	include_directories( ${HDF5_CXX_INCLUDE_DIRS} )
endif()

# project headers
include_directories(include)

link_directories(build /usr/local/hdf5/lib)
# project sources
set( SOURCES src/hdfexport.cpp )

add_executable( hdf2dat ${SOURCES} )
target_link_libraries( hdf2dat ${HDF5_CXX_LIBRARIES} ${wxWidgets_LIBRARIES} ${Boost_LIBRARIES})
