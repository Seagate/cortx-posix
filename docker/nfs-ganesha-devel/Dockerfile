ARG  CENTOS_RELEASE=7
FROM registry.gitlab.mero.colo.seagate.com/mero/mero:${CENTOS_RELEASE}

RUN git clone --recursive http://gitlab.mero.colo.seagate.com/eos/fs/nfs-ganesha.git
RUN git clone http://gitlab.mero.colo.seagate.com/eos/fs/eos-fs.git

RUN yum install -y \
        bison \
        cmake \
        flex \
        krb5-devel \
        krb5-libs

RUN cd eos-fs && ./scripts/build-nfs-ganesha.sh config
RUN yum-builddep -y nfs-ganesha/src/libntirpc/libntirpc.spec
RUN yum-builddep -y nfs-ganesha/src/nfs-ganesha.spec
RUN cd eos-fs && ./scripts/build-nfs-ganesha.sh rpm-gen

RUN mv $HOME/rpmbuild/SRPMS/nfs-ganesha*.rpm $HOME \
    && mv $HOME/rpmbuild/RPMS/x86_64/* $HOME \
    && rm -rf $HOME/rpmbuild \
    && rpmdev-setuptree \
    && rm -rf eos-fs
