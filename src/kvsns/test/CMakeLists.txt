include_directories(include)

add_subdirectory(framework)
set(KVSNS_FRAMEWORK kvsns_framework)

add_subdirectory(utils)
set(KVSNS_UTILS kvsns_utils)

add_subdirectory(op)
set(KVSNS_OP kvsns_op)

add_subdirectory(kvssh)
