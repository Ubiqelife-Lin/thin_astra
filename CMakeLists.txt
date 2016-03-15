cmake_minimum_required(VERSION 2.8.3)
project(thin_astra)

find_path(_astra_openni_found OpenNI-Linux-x64-2.2 PATHS ~/src)
if(NOT _astra_openni_found)
    message(WARNING "No ORBBEC OPENNI2 directory found in ~/src, please install ORBBEC OPENNI2 in this location if you want to compile thin_astra")
    return()
endif()

SET(ROS_BUILD_TYPE Release)
SET(CMAKE_C_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS "-O3")
find_package(catkin REQUIRED COMPONENTS
  roscpp
  std_msgs
  sensor_msgs
)

include_directories(~/src/OpenNI-Linux-x64-2.2/Include)
include_directories(~/src/OpenNI-Linux-x64-2.2/Samples/Common/)
link_directories(~/src/OpenNI-Linux-x64-2.2/Redist/)

catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES xtionberry
#  CATKIN_DEPENDS roscpp sensor_msgs std_msgs
#  DEPENDS system_lib
)

include_directories(
  ${catkin_INCLUDE_DIRS}
)

add_executable(thin_astra_node src/thin_astra_node.cpp)
target_link_libraries(thin_astra_node
   ${catkin_LIBRARIES}
   rt 
   OpenNI2
 )

