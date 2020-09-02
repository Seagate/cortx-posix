[![Codacy Badge](https://app.codacy.com/project/badge/Grade/62e8043f34f642c397ab84bfbe5cba4d)](https://www.codacy.com?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Seagate/efs-ganesha&amp;utm_campaign=Badge_Grade)

# KVSFS-FSAL

NFS Ganesha backend for CORTXFS implementation.

The FSAL (Filesystem Abstraction Layer) implements
NFS Ganesha FSAL API based on CORTXFS as an underlying filesystem.

See also:

* CORTXFS repo: https://github.com/Seagate/cortx-fs.git

* NFS Ganesha repo: https://github.com/Seagate/nfs-ganesha.git

* KVSFS repo: http://github.com/Seagate/cortx-fs-ganesha.git


### Disclaimer
Please refer to the shared disclaimer (https://github.com/Seagate/cortx-posix#disclaimer) about this code repository.

# Quick start

1. Install CORTXFS packages (`libcortxfs{-devel}`).

2. Download NFS Ganesha into `$PWD/../../nfs-ganesha`.

3. Build & Install NFS Ganesha.

4. Build & Install KVSFS-FSAL:

```sh
./scripts/build.sh reconf
./scripts/build.sh make -j
./scripts/build.sh reinstall
```

4. Run NFS Ganesha.
