ARG  CENTOS_RELEASE=7
FROM registry.gitlab.mero.colo.seagate.com/eos/fs/nfs-ganesha:${CENTOS_RELEASE}

RUN yum install -y \
        jemalloc \
        jemalloc-devel \
        libini_config-devel
