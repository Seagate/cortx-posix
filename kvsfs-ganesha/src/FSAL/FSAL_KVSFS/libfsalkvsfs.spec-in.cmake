%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@KVSFS_GANESHA_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: kvsfs-ganesha
Version: @KVSFS_GANESHA_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: NFS-Ganesha FSAL for EFS
License: Seagate
URL: https://seagit.okla.seagate.com/seagate-eos/eos-fs
Requires: eos-efs
Source: %{sourcename}.tar.gz
Provides: %{name} = %{version}-%{release}

# EOS EFS-GANESHA library paths
%define _efsganesha_dir		@INSTALL_DIR_ROOT@/@PROJECT_NAME_BASE@/efsganesha
%define _efsganesha_lib_dir	%{_efsganesha_dir}/lib
%define _efsganesha_bin_dir	%{_efsganesha_dir}/bin

%description
NFS-Ganesha FSAL for EFS

%prep
%setup -q -n %{sourcename}
# Nothing to do here

%build

%install

mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_sbindir}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_efsganesha_dir}
mkdir -p %{buildroot}%{_efsganesha_lib_dir}
mkdir -p %{buildroot}%{_efsganesha_bin_dir}
mkdir -p %{buildroot}%{_libdir}/ganesha/
cd %{_lib_path}
install -m 755 libfsalkvsfs.so* %{buildroot}%{_efsganesha_lib_dir}
install -m 755 %{_nfs_setup_dir}/nfs_setup.sh %{buildroot}%{_efsganesha_bin_dir}
ln -s %{_efsganesha_lib_dir}/libfsalkvsfs.so.4.2.0 %{buildroot}%{_libdir}/ganesha/libfsalkvsfs.so.4.2.0
ln -s %{_efsganesha_lib_dir}/libfsalkvsfs.so.4 %{buildroot}%{_libdir}/ganesha/libfsalkvsfs.so.4
ln -s %{_efsganesha_lib_dir}/libfsalkvsfs.so %{buildroot}%{_libdir}/ganesha/libfsalkvsfs.so
ln -s %{_efsganesha_bin_dir}/nfs_setup.sh %{buildroot}%{_sbindir}/nfs_setup

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%files
# TODO - Verify permissions, user and groups for directory.
%defattr(-, root, root, -)
%{_libdir}/ganesha/libfsalkvsfs.so*
%{_efsganesha_lib_dir}/libfsalkvsfs.so*
%{_efsganesha_bin_dir}/nfs_setup.sh
%{_sbindir}/nfs_setup

%changelog
* Mon Jan 25 2019 Ashay Shirwadkar <ashay.shirwadkar@seagate.com> - 1.0.0
- Initial spec file
