set(MERO_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(MERO_CFLAGS "${MERO_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(MERO_CFLAGS "${MERO_CFLAGS} -I/usr/include/mero ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MERO_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MERO_CFLAGS}")

if(USE_POSIX_STORE)
	add_subdirectory(posix_store)
endif(USE_POSIX_STORE)

if(USE_POSIX_OBJ)
	add_subdirectory(posix_obj)
endif(USE_POSIX_OBJ)

if(USE_MERO_STORE)
	add_subdirectory(mero)
endif(USE_MERO_STORE)

add_executable(extstore_cp_to extstore_cp_to.c)
target_link_libraries(extstore_cp_to extstore)

add_executable(extstore_cp_from extstore_cp_from.c)
target_link_libraries(extstore_cp_from extstore)

