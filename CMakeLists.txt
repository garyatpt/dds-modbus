cmake_minimum_required(VERSION 2.8.9)
project(dds-modbus C)

#SET(CMAKE_VERBOSE_MAKEFILE ON)

SET(MASTER_NAME master)
SET(BRIDGE_NAME bridge)

# libdds location
SET(DDS_LIB_DIR /opt/PrismTech/Vortex_v2/Device/VortexLite/2.0.4/lib/linux_gcc_x86/libdds.so)

# dds include files
SET(DDS_HEADER_INCLUDE_DIR /opt/PrismTech/Vortex_v2/Device/VortexLite/2.0.4/include)

# modbus include files
SET(MB_HEADER_INCLUDE_DIR /usr/include/modbus)

# source codes
SET(MASTER_SOURCES master.c mb.c mb.h ${DDS_HEADER_INCLUDE_DIR}/dds.h ${MB_HEADER_INCLUDE_DIR}/modbus.h)
SET(BRIDGE_SOURCES bridge.c mb.c mb.h ${DDS_HEADER_INCLUDE_DIR}/dds.h)

# cflag
SET(EXT_C_FLAGS "-m64 -pipe -Wall -fno-strict-aliasing -O3 -std=c99 -Wstrict-prototypes -Wmissing-prototypes -Dos_linux_gcc_x86 -DLITE=1 -DNDEBUG -D_GNU_SOURCE")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXT_C_FLAGS}")

INCLUDE_DIRECTORIES(${DDS_HEADER_INCLUDE_DIR} ${MB_HEADER_INCLUDE_DIR})

# set install prefix
SET(CMAKE_INSTALL_PREFIX /)

# master
ADD_EXECUTABLE(${MASTER_NAME} ${MASTER_SOURCES})
TARGET_LINK_LIBRARIES(${MASTER_NAME} LINK_PUBLIC ${DDS_LIB_DIR} rt pthread modbus)

# bridge
ADD_EXECUTABLE(${BRIDGE_NAME} ${BRIDGE_SOURCES})
TARGET_LINK_LIBRARIES(${BRIDGE_NAME} LINK_PUBLIC ${DDS_LIB_DIR} rt pthread mosquitto)

# install binary
install(TARGETS ${MASTER_NAME} DESTINATION /usr/bin COMPONENT binaries)
install(TARGETS ${BRIDGE_NAME} DESTINATION /usr/bin COMPONENT binaries)