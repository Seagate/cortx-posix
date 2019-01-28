%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@LIBFSALKVSFS_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: libfsalkvsfs
Version: @LIBFSALKVSFS_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: NFS-Ganesha FSAL for KVSNS
License: Seagate Proprietary
URL: https://seagit.okla.seagate.com/seagate-eos/eos-fs
Requires: libkvsns
Source: %{sourcename}.tar.gz
Provides: %{name} = %{version}-%{release}

%description
NFS-Ganesha FSAL for KVSNS

%prep
%setup -q -n %{sourcename}
# Nothing to do here

%build

%install
cd %{_lib_path}
mkdir -p %{buildroot}/usr/lib64/ganesha/
install -m 644 libfsalkvsfs.so* %{buildroot}/usr/lib64/ganesha/

%post
/sbin/ldconfig

%files
# TODO - Verify permissions, user and groups for directory.
%defattr(-, root, root, -)
/usr/lib64/ganesha/libfsalkvsfs.so.4.2.0
/usr/lib64/ganesha/libfsalkvsfs.so.4
/usr/lib64/ganesha/libfsalkvsfs.so

%changelog
* Mon Jan 25 2019 Ashay Shirwadkar <ashay.shirwadkar@seagate.com> - 1.0.0
- Initial spec file
