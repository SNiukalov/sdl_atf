project(RemoteTestingAdapterServer)
cmake_minimum_required(VERSION 3.11)
set (CMAKE_CXX_STANDARD 11)
include(FetchContent)

# Enable PIC as static libraries may be linked to shared objects
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

if (UNIX)
  if (QNXNTO)
    set(LINUX FALSE)
  else(QNXNTO)
    set(LINUX TRUE)
  endif(QNXNTO)
endif()

FetchContent_Declare(
  rpclib
  GIT_REPOSITORY https://github.com/rpclib/rpclib.git
  GIT_TAG        v2.2.1
)

FetchContent_GetProperties(rpclib)
if(NOT rpclib_POPULATED)
  FetchContent_Populate(rpclib)
  add_subdirectory("${rpclib_SOURCE_DIR}" "${rpclib_BINARY_DIR}")
  find_package( Threads REQUIRED )
  add_library(rpc::rpclib ALIAS rpc)
endif()

set(RPCLIB_DEPENDENCIES "${rpclib_SOURCE_DIR}/dependencies")
file(GLOB_RECURSE DEP_HEADERS
  ${RPCLIB_DEPENDENCIES}/include/*.h
  ${RPCLIB_DEP_LIBRARIDEPENDENCIES}/include/*.hpp)
set(DEP_SOURCES
  ${RPCLIB_DEPENDENCIES}/src/format.cc
  ${RPCLIB_DEPENDENCIES}/src/posix.cc)

find_package( Threads REQUIRED )
add_executable(${PROJECT_NAME}
                main.cc                
                utils_manager.cc
		            tcp_message_broker.cpp
                ${DEP_SOURCES})


include(ExternalProject) 

set(Boost_NO_SYSTEM_PATHS TRUE)
set(BOOST_ROOT ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost)
set(BOOST_INSTALL ${BOOST_ROOT})
set(BOOST_INCLUDE_DIRS ${BOOST_INSTALL}/include)
set(BOOST_LIBRARY_DIRS ${BOOST_INSTALL}/lib)

if(QNXNTO)
  set(CMAKE_FIND_ROOT_PATH "${CMAKE_FIND_ROOT_PATH}" "${BOOST_ROOT}")
endif()

find_package(Boost 1.68.0 COMPONENTS system) 

if (NOT ${Boost_FOUND})
  message(STATUS "Did not find boost. Downloading and installing boost 1.68")
  if(${LINUX})
  
    set(BOOST_QNX_PROJECT_CONFIG_JAM "")
    set(BOOST_GCC_JAM "")
    set(BOOST_FILESYSTEM_OPERATION "")
    set(BOOTSTRAP
      ./bootstrap.sh --with-libraries=system --prefix=${BOOST_INSTALL})
    set(BOOST_BUILD_COMMAND ./b2 --prefix=${BOOST_INSTALL})
  
  else()
    
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES ".*aarch64")
      set(ADDRESS_MODEL "64")
    else()  
      set(ADDRESS_MODEL "32")
    endif ()
    
    set(
      BOOST_QNX_PROJECT_CONFIG_JAM
      "using gcc : nto${CMAKE_SYSTEM_PROCESSOR} : ${QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-g++${HOST_EXECUTABLE_SUFFIX} : -L${QNX_HOST}/usr/lib -I${QNX_HOST}/usr/include" $<SEMICOLON>)
    set(
      BOOST_GCC_JAM
      #sed -Ei "s/case darwin/case *qnx*/g" ./tools/build/src/tools/gcc.jam) # boost 1.66
      sed -Ei "s/ : rt/ /g" ./tools/build/src/tools/gcc.jam)
    set(
      BOOST_FILESYSTEM_OPERATION
      sed -Ei "s/__SUNPRO_CC/__QNX__/g" ./libs/filesystem/src/operations.cpp)
    set(
      BOOTSTRAP
      ./bootstrap.sh --with-toolset=gcc --with-libraries=system --prefix=${BOOST_INSTALL})    
    set(BOOST_BUILD_COMMAND ./b2 address-model=${ADDRESS_MODEL} -std=gnu++11 target-os=qnxnto toolset=gcc-nto${CMAKE_SYSTEM_PROCESSOR} --prefix=${BOOST_INSTALL})
  
  endif()
  
  set(BOOST_INSTALL_COMMAND ${BOOST_BUILD_COMMAND} install)  
  
  ExternalProject_Add(
    Boost
    URL https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz
    DOWNLOAD_DIR ${BOOST_LIB_SOURCE_DIRECTORY}
    SOURCE_DIR ${BOOST_LIB_SOURCE_DIRECTORY}  
    CONFIGURE_COMMAND  ${BOOST_GCC_JAM} COMMAND ${BOOST_FILESYSTEM_OPERATION} COMMAND ${BOOTSTRAP}
    BUILD_COMMAND echo ${BOOST_QNX_PROJECT_CONFIG_JAM} >> ./project-config.jam COMMAND ${BOOST_BUILD_COMMAND}
    INSTALL_COMMAND ${BOOST_INSTALL_COMMAND}
    INSTALL_DIR ${BOOST_INSTALL}
    BUILD_IN_SOURCE true
  )
endif()

set(BOOST_LIBRARIES
    "${BOOST_LIBRARY_DIRS}/libboost_system.a")

include_directories("${BOOST_INCLUDE_DIRS}" )
add_dependencies(${PROJECT_NAME} Boost)

target_link_libraries(${PROJECT_NAME}
                       rpc::rpclib
                       Threads::Threads
                       $<$<BOOL:${LINUX}>:rt>
                       $<$<BOOL:${QNXNTO}>:mq>
                       $<$<BOOL:${QNXNTO}>:socket>
                       $<$<BOOL:${QNXNTO}>:backtrace>
                       $<$<BOOL:${QNXNTO}>:stdc++>
                       ${BOOST_LIBRARIES}
                      )

target_include_directories( ${PROJECT_NAME}  PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/..
   PRIVATE ${RPCLIB_DEPENDENCIES}/include
)

target_compile_definitions(${PROJECT_NAME} 
  PRIVATE    
    "ASIO_STANDALONE"
    "RPCLIB_ASIO=clmdep_asio"
    "RPCLIB_FMT=clmdep_fmt"
  PUBLIC
    "RPCLIB_MSGPACK=clmdep_msgpack"
  )

if(RPCLIB_ENABLE_LOGGING)
  target_compile_definitions(${PROJECT_NAME} PRIVATE "RPCLIB_ENABLE_LOGGING")
endif()

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION "${CMAKE_INSTALL_PREFIX}/RemoteTestingAdapterServer"
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
  COMPONENT sdl_atf
)
