%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@KVSFS_GANESHA_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: kvsfs-ganesha
Version: @KVSFS_GANESHA_BASE_VERSION@
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
mkdir -p %{buildroot}/usr/share/libfsalkvsfs/
mkdir -p %{buildroot}/opt/seagate/nfs/setup/
install -m 755 %{_nfs_setup_dir}/nfs_setup.sh %{buildroot}/usr/share/libfsalkvsfs/
ln -s /usr/share/libfsalkvsfs/nfs_setup.sh %{buildroot}/opt/seagate/nfs/setup/eos_nfs_setup

%post
/sbin/ldconfig

%files
# TODO - Verify permissions, user and groups for directory.
%defattr(-, root, root, -)
/usr/lib64/ganesha/libfsalkvsfs.so.4.2.0
/usr/lib64/ganesha/libfsalkvsfs.so.4
/usr/lib64/ganesha/libfsalkvsfs.so
/usr/share/libfsalkvsfs/nfs_setup.sh
/opt/seagate/nfs/setup/eos_nfs_setup

%changelog
* Mon Jan 25 2019 Ashay Shirwadkar <ashay.shirwadkar@seagate.com> - 1.0.0
- Initial spec file
