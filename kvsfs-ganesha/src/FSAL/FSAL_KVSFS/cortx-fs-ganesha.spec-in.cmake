%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@KVSFS_GANESHA_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: @PROJECT_NAME@
Version: @KVSFS_GANESHA_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: NFS-Ganesha FSAL for @PROJECT_NAME_BASE@-fs
License: Seagate
URL: https://seagit.okla.seagate.com/seagate-eos/eos-fs
Requires: @PROJECT_NAME_BASE@-fs
Source: %{sourcename}.tar.gz
Provides: %{name} = %{version}-%{release}

# EOS EFS-GANESHA library paths
%define	_fs_lib			@PROJECT_NAME_BASE@-fs
%define	_fsal_ganesha_lib	@PROJECT_NAME@
%define _fsal_ganesha_dir	@INSTALL_DIR_ROOT@/@PROJECT_NAME_BASE@/%{_fsal_ganesha_lib}
%define _fsal_ganesha_lib_dir	%{_fsal_ganesha_dir}/lib
%define _fsal_ganesha_bin_dir	%{_fsal_ganesha_dir}/bin

%description
NFS-Ganesha FSAL for @PROJECT_NAME_BASE@-fs

%prep
%setup -q -n %{sourcename}
# Nothing to do here

%build

%install

mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_sbindir}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_fsal_ganesha_dir}
mkdir -p %{buildroot}%{_fsal_ganesha_lib_dir}
mkdir -p %{buildroot}%{_fsal_ganesha_bin_dir}
mkdir -p %{buildroot}%{_libdir}/ganesha/
cd %{_lib_path}
install -m 755 lib%{_fsal_ganesha_lib}.so* %{buildroot}%{_fsal_ganesha_lib_dir}
install -m 755 %{_nfs_setup_dir}/nfs_setup.sh %{buildroot}%{_fsal_ganesha_bin_dir}
ln -s %{_fsal_ganesha_lib_dir}/lib%{_fsal_ganesha_lib}.so.4.2.0 %{buildroot}%{_libdir}/ganesha/libfsal%{_fs_lib}.so.4.2.0
ln -s %{_fsal_ganesha_lib_dir}/lib%{_fsal_ganesha_lib}.so.4 %{buildroot}%{_libdir}/ganesha/libfsal%{_fs_lib}.so.4
ln -s %{_fsal_ganesha_lib_dir}/lib%{_fsal_ganesha_lib}.so %{buildroot}%{_libdir}/ganesha/libfsal%{_fs_lib}.so
ln -s %{_fsal_ganesha_bin_dir}/nfs_setup.sh %{buildroot}%{_sbindir}/nfs_setup

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%files
# TODO - Verify permissions, user and groups for directory.
%defattr(-, root, root, -)
%{_libdir}/ganesha/libfsal%{_fs_lib}.so*
%{_fsal_ganesha_lib_dir}/lib%{_fsal_ganesha_lib}.so*
%{_fsal_ganesha_bin_dir}/nfs_setup.sh
%{_sbindir}/nfs_setup

%changelog
* Mon Jan 25 2019 Ashay Shirwadkar <ashay.shirwadkar@seagate.com> - 1.0.0
- Initial spec file
