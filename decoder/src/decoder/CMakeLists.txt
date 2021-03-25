include_directories(include)
include_directories("/usr/include/motr")

set(MOTR_CFLAGS "-g -D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -I/usr/include/motr")

set_source_files_properties(m0log_dumper.c PROPERTIES COMPILE_FLAGS  "${CMAKE_C_FLAGS} ${MOTR_CFLAGS}")

add_executable(m0log_dumper m0log_dumper.c)

target_link_libraries(m0log_dumper
	${PROJECT_NAME_BASE}-fs
	${PROJECT_NAME_BASE}-nsal
	${PROJECT_NAME_BASE}-dsal
	motr
	motr-helpers
        ${STORE_LIBRARY}
)

