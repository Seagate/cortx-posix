[![Codacy Badge](https://app.codacy.com/project/badge/Grade/7e6ffd004e794ecf945f076988a9185a)](https://www.codacy.com?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Seagate/cortx-posix&amp;utm_campaign=Badge_Grade)

# CORTX-POSIX
CORTX-POSIX is top level code repository, which helps in building various sub-components (like CORTXFS, NSAL, DSAL etc) to support different file access protocols (like SAMBA, NFS etc.) to Seagate CORTX. This code base consists of scripts which will facilitate in fetching the sub-components repos, and build the code.
Note that currently only NFS protocol is supported. The supported NFS server is user-space implementation (a.k.a. NFS Ganesha).

### Disclaimer
The code for file access protocol (like NFS) for CORTX is distributed across multiple repos. The code is still under active development.  We welcome anyone and everyone who is interested to join our community and track our progress and even participate in development or testing if you are so inclined!  Just please be advised that this code is not ready for production usage and is only provided to allow the external community to watch and participate in the development.

## Pre-requisites
CORTX-POSIX assumes that MOTR code is installed on the system. For details on CORTX MOTR please refer to [Cortx-Motr-Quick-Start-Guide](https://github.com/Seagate/cortx-motr/blob/dev/doc/Quick-Start-Guide.rst)

## Get CORTX-POSIX ready! 

Refer to the [CORTX-POSIX Quickstart Guide](https://github.com/Seagate/cortx-posix/blob/dev/doc/Quick_Start_Guide.md) to build and test the CORTX-POSIX Component.

## Contribute to CORTX-POSIX

We welcome all Source Code and Documentation contributions to the CORTX-POSIX component repository.
Refer to the [Contributing to CORTX-POSIX](https://github.com/Seagate/cortx-posix/blob/dev/doc/ContributingToCortxPosix.md) document to submit your contributions.

### CORTX Community

We are excited about your interest in CORTX and hope you will join us. We take our community very seriously, and we are committed to creating a community built on respectful interactions and inclusivity, as documented in our [Code of conduct](https://github.com/Seagate/cortx/blob/main/CODE_OF_CONDUCT.md).

Refer to the CORTX Community Guide that hosts all information about community values, code of conduct, how to contribute code and documentation, community and code style guide, and how to reach out to us.

## Thank You!

We thank you for stopping by to check out the CORTX Community. Seagate is fully dedicated to its mission to build open source technologies that help the world save unlimited data and solve challenging data problems. Join our mission to help reinvent a data-driven world.
