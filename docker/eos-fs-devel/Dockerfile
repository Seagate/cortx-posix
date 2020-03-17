ARG  CENTOS_RELEASE=7
FROM registry.gitlab.mero.colo.seagate.com/eos/fs/nfs-ganesha:${CENTOS_RELEASE}

RUN yum install -y \
        jemalloc \
        jemalloc-devel \
        libcmocka \
        libcmocka-devel \
        libini_config-devel \
        json-c \
        json-c-devel \
        libevent \
        libevent-devel \
        openssl \
        openssl-devel

# TODO: Use libevhtp from the right place.
# See http://gitlab.mero.colo.seagate.com/eos/fs/eos-fs/issues/2
# for details.
RUN yum install -y \
	http://ci-storage.mero.colo.seagate.com/releases/eos/uploads/nfs_uploads/libevhtp{,-devel}-1.2.18-2.el7.x86_64.rpm
