cmake_minimum_required(VERSION 3.7)
project(owlmodem-tests)

set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_definitions(-DBUILD_FOR_TEST)

set(MODEM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/BreakoutSDK/modem)

set(MODEM_SOURCES
	${MODEM_DIR}/enums.cpp
	${MODEM_DIR}/../utils/str.c
	${MODEM_DIR}/OwlModemAT.cpp
	)

add_executable( test_owlmodemat
  test_owlmodemat.cpp
  test_platform.cpp
  ${MODEM_SOURCES}
  )
