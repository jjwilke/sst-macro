cmake_minimum_required (VERSION 3.9)

project (SSTMacroUnitTests)

set (CMAKE_CXX_STANDARD 11)

set(SSTMAC_DIR "" CACHE PATH "The path to SST/macro installation")

find_path(SSTMAC_INC sstmac/sstmacro.h PATHS ${SSTMAC_DIR}/include)
find_library(SSTMAC_LIB sstmac ${SSTMAC_DIR}/lib)
find_library(SPKT_LIB sprockit ${SSTMAC_DIR}/lib)
find_program(SSTMAC_EXE sstmac ${SSTMAC_DIR}/bin)



set (SSTMacroUnitTests_VERSION_MAJOR 8)
set (SSTMacroUnitTests_VERSION_MINOR 1)

enable_testing()
find_package(GTest REQUIRED)

function(add_sstmac_test exec source)
add_executable(${exec} ${source})
target_include_directories(${exec} PUBLIC ${SSTMAC_INC})
target_link_libraries(${exec} ${SSTMAC_LIB})
target_link_libraries(${exec} ${SPKT_LIB})
target_link_libraries(${exec} GTest::GTest)
add_test(NAME ${exec} COMMAND ${exec} -f test.ini)
endfunction(add_sstmac_test)

function(add_check_test config)
add_test(NAME ${config} COMMAND ${SSTMAC_EXE} --help)
endfunction(add_check_test)

add_sstmac_test(routing test/routing.cc)
add_check_test(hello)


