%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@KVSFS_GANESHA_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: kvsfs-ganesha-test
Version: @KVSFS_GANESHA_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: Test-Tools for KVSFS
License: Seagate Proprietary
URL: https://seagit.okla.seagate.com/seagate-eos/eos-fs
Requires: libkvsns
Source: %{sourcename}.tar.gz
Provides: %{name} = %{version}-%{release}

%description
Installs test scripts for KVSFS-Ganesha

%prep
%setup -q -n %{sourcename}
# Nothing to do here

%install
mkdir -p %{buildroot}/opt/seagate/eos/efs/test/
install -m 755 run_* %{buildroot}/opt/seagate/eos/efs/test/
mkdir -p %{buildroot}/opt/seagate/eos/efs/test/filebench
install -m 755 filebench/*  %{buildroot}/opt/seagate/eos/efs/test/filebench/
mkdir -p %{buildroot}/opt/seagate/eos/efs/test/concurrency
install -m 755 concurrency/*  %{buildroot}/opt/seagate/eos/efs/test/concurrency/
mkdir -p %{buildroot}/opt/seagate/eos/efs/test/iozone
install -m 755 iozone/*  %{buildroot}/opt/seagate/eos/efs/test/iozone/

#%post

%files
%defattr(-, root, root, -)
/opt/seagate/eos/efs/test/

%changelog
* Tue Nov 26 2019 Shreya Karmakar <shreya.karmakar@seagate.com> - 1.0.0
- Initial spec file
