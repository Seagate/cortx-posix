cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

add_executable(kvsns_test kvsns_test.c)
target_link_libraries(kvsns_test kvsns ${STORE_LIBRARY}
		      ${KVSAL_LIBRARY} )

# NOTE: Delete-on-close test has been removed because
# kvsns does not support open states.

add_executable(kvsns_file_test_rw kvsns_file_test_rw.c)
target_link_libraries(kvsns_file_test_rw kvsns ${STORE_LIBRARY}
		      ${KVSAL_LIBRARY})

add_executable(kvsns_set_char kvsns_set_char.c)
target_link_libraries(kvsns_set_char kvsns ${STORE_LIBRARY}
		      ${KVSAL_LIBRARY})

add_executable(kvsns_get_char kvsns_get_char.c)
target_link_libraries(kvsns_get_char kvsns ${STORE_LIBRARY}
		      ${KVSAL_LIBRARY})

add_executable(kvsns_del_char kvsns_del_char.c)
target_link_libraries(kvsns_del_char kvsns ${STORE_LIBRARY}
		      ${KVSAL_LIBRARY})

add_executable(kvsns_get_list kvsns_get_list.c)
target_link_libraries(kvsns_get_list kvsns ${STORE_LIBRARY}
		      ${KVSAL_LIBRARY})


add_executable(kvsns_ino_incr kvsns_ino_incr.c)
target_link_libraries(kvsns_ino_incr kvsns ${STORE_LIBRARY}
		      ${KVSAL_LIBRARY})
