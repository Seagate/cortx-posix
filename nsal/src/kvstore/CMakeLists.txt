if(USE_KVS_REDIS)
    add_subdirectory(plugins/redis)
endif(USE_KVS_REDIS)

if(USE_KVS_MERO)
    add_subdirectory(plugins/eos)
endif(USE_KVS_MERO)
