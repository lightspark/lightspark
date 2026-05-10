# CMakeLists.txt
#
#     Author: Fabian Meyer
# Created On: 12 Jul 2019

include_directories(
    ${CATCH2_INCLUDE_DIR}
)

add_executable(unit_tests
    "main.cpp"
    "test_inifile.cpp"
)
target_link_libraries(unit_tests inicpp::inicpp)

add_test(NAME unit_tests COMMAND unit_tests)
