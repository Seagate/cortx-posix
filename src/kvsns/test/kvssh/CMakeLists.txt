add_executable(kvssh kvssh.c)
target_link_libraries( kvssh
	kvsns ${KVSNS_FRAMEWORK}
	${KVSNS_OP} ${KVSNS_UTILS}
	${KVSAL_LIBRARY} ${STORE_LIBRARY}
)
