
# CORTX-POSIX

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/9b53934395164e83b24e9caed221c4ae)](https://app.codacy.com/gh/Seagate/cortx-posix?utm_source=github.com&utm_medium=referral&utm_content=Seagate/cortx-posix&utm_campaign=Badge_Grade) [![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://github.com/Seagate/cortx-posix/blob/main/LICENSE) [![Slack](https://img.shields.io/badge/chat-on%20Slack-blue")](https://cortx.link/join-slack) [![YouTube](https://img.shields.io/badge/Video-YouTube-red)](https://cortx.link/videos) 

CORTX-POSIX is top level code repository, which helps in building various sub-components (like CORTXFS, NSAL, DSAL etc) to support different file access protocols (like SAMBA, NFS etc.) to Seagate CORTX. This code base consists of scripts which will facilitate in fetching the sub-components repos, and build the code.
Note that currently only NFS protocol is supported. The supported NFS server is user-space implementation (a.k.a. NFS Ganesha).

### Disclaimer
The code for file access protocol (like NFS) for CORTX is distributed across multiple repos. The code is no longer under active development. We welcome anyone and everyone who is interested to please attempt to revive the development of this code. Just please be advised that this code is not ready for production usage and is only provided as a starting point for anyone interested in an NFS layer to CORTX and who is willing to drive its development.

## Pre-requisites
CORTX-POSIX assumes that MOTR code is installed on the system. For details on CORTX MOTR please refer to [Cortx-Motr-Quick-Start-Guide](https://github.com/Seagate/cortx-motr/blob/main/doc/Quick-Start-Guide.rst).

## Get CORTX-POSIX ready! 

Refer to the [CORTX-POSIX Quickstart Guide](https://github.com/Seagate/cortx-posix/blob/main/doc/Quick_Start_Guide.md) to build and test the CORTX-POSIX Component.

## Contribute to CORTX-POSIX

We are excited about your interest in CORTX and hope you will join us. We take our community very seriously, and we are committed to creating a community built on respectful interactions and inclusivity, as documented in our [Code of conduct](CODE_OF_CONDUCT.md).

Refer to the [CORTX Contribution Guide](CONTRIBUTING.md) that hosts all information about community values, code of conduct, how to contribute code and documentation, community and code style guide, and how to reach out to us.

## Reach Out To Us

- Please refer to our [Support](SUPPORT.md) section for support or to discuss things with your fellow CORTX Community members.
- Click here to [view existing issues or to create new issues](https://github.com/Seagate/cortx-posix/issues).

## Thank You!

We thank you for stopping by to check out the CORTX Community. Seagate is fully dedicated to its mission to build open source technologies that help the world save unlimited data and solve challenging data problems. Join our mission to help reinvent a data-driven world.
