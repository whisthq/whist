set(BINARY FractalClient_test)

#file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp)

#set(SOURCES ${TEST_SOURCES})
enable_testing()
add_executable(${BINARY} main.cpp)

add_test(NAME ${BINARY} COMMAND ${BINARY})

#target_link_libraries(${BINARY} PUBLIC FRACTAL_CLIENT_LIBRARY gtest)

target_link_libraries(${BINARY} PUBLIC  gtest)

include(GoogleTest)
gtest_discover_tests(${BINARY})