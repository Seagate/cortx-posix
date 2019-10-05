cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

add_executable(test_mkdir test_mkdir.c test_ns_common.c)
target_link_libraries(test_mkdir kvsns ${STORE_LIBRARY}
                      ${KVSAL_LIBRARY} )

add_executable(test_rmdir test_rmdir.c test_ns_common.c)
target_link_libraries(test_rmdir kvsns ${STORE_LIBRARY}
                      ${KVSAL_LIBRARY} )
