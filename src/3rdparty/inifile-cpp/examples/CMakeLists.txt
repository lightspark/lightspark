# CMakeLists.txt
#
#     Author: Fabian Meyer
# Created On: 14 Nov 2020

add_executable(custom_type_conversion "custom_type_conversion.cpp")
target_link_libraries(custom_type_conversion inicpp::inicpp)

add_executable(decode_ini_file "decode_ini_file.cpp")
target_link_libraries(decode_ini_file inicpp::inicpp)

add_executable(encode_ini_file "encode_ini_file.cpp")
target_link_libraries(encode_ini_file inicpp::inicpp)

add_executable(load_ini_file "load_ini_file.cpp")
target_link_libraries(load_ini_file inicpp::inicpp)

add_executable(save_ini_file "save_ini_file.cpp")
target_link_libraries(save_ini_file inicpp::inicpp)
