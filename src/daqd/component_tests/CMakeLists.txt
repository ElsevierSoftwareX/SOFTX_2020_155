add_executable(generate_crc32_table ../checksum_crc32.cc)
target_compile_definitions(generate_crc32_table PRIVATE CRC32_MAIN)
target_compile_definitions(generate_crc32_table PRIVATE CRC32_AUTO_GENERATE_TABLE)
target_include_directories(generate_crc32_table PUBLIC ..)

add_executable(test_crc32 ../checksum_crc32.cc test_checksum_crc32.cc)
target_include_directories(test_crc32 PUBLIC ..)
target_compile_definitions(test_crc32 PRIVATE CRC32_AUTO_GENERATE_TABLE)

add_executable(test_crc32_table_lookup ../checksum_crc32.cc test_checksum_crc32)
target_include_directories(test_crc32_table_lookup PUBLIC ..)
