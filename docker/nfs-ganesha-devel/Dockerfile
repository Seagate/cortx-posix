ARG  CENTOS_RELEASE=7
FROM registry.gitlab.mero.colo.seagate.com/mero/mero:${CENTOS_RELEASE}

RUN git clone --recursive http://gitlab.mero.colo.seagate.com/cortx/fs/nfs-ganesha.git \
    --branch V2.8-stable nfs-ganesha-cortx
RUN git clone http://gitlab.mero.colo.seagate.com/cortx/fs/efs.git

RUN yum install -y \
        bison \
        cmake \
        flex \
        krb5-devel \
        krb5-libs \
        userspace-rcu-devel \
        python-devel

RUN cd efs && ./scripts/build-nfs-ganesha.sh config
RUN yum-builddep -y nfs-ganesha-cortx/src/libntirpc/libntirpc.spec
RUN yum-builddep -y nfs-ganesha-cortx/src/nfs-ganesha.spec
RUN cd efs && ./scripts/build-nfs-ganesha.sh rpm-gen

RUN mv $HOME/rpmbuild/SRPMS/nfs-ganesha*.rpm $HOME \
    && mv $HOME/rpmbuild/RPMS/x86_64/* $HOME \
    && rm -rf $HOME/rpmbuild \
    && rpmdev-setuptree \
    && rm -rf efs
