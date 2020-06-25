[![Codacy Badge](https://app.codacy.com/project/badge/Grade/62e8043f34f642c397ab84bfbe5cba4d)](https://www.codacy.com?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Seagate/cortx-fs-ganesha&amp;utm_campaign=Badge_Grade)

# KVSFS-FSAL

NFS Ganesha backend for KVSNS implementation.

The FSAL (Filesystem Abstraction Layer) implements
NFS Ganesha FSAL API based on KVSNS as an underlying filesystem.

See also:

* EOS-FS repo: http://gitlab.mero.colo.seagate.com/eos/fs/eos-fs

* NFS Ganesha repo: http://gitlab.mero.colo.seagate.com/eos/fs/nfs-ganesha

* KVSNS repo: http://gitlab.mero.colo.seagate.com/eos/fs/kvsns


# Quick start

1. Install KVSNS packages (`libkvsns{-devel}`).

2. Download NFS Ganesha into `$PWD/../../nfs-ganesha`.

3. Build & Install NFS Ganesha.

4. Build & Install KVSFS-FSAL:

```sh
./scripts/build.sh reconf
./scripts/build.sh make -j
./scripts/build.sh reinstall
```

4. Run NFS Ganesha.
