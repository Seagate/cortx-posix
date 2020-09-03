%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@KVSFS_GANESHA_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: @PROJECT_NAME@
Version: @KVSFS_GANESHA_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: Test-Tools for KVSFS
License: Seagate
URL: https://github.com/Seagate/cortx-fs-ganesha
Requires: @PROJECT_NAME_BASE@-fs
Source: %{sourcename}.tar.gz
Provides: %{name} = %{version}-%{release}

# CORTX EFS-TEST library paths
%define _fs_test_dir		@INSTALL_DIR_ROOT@/@PROJECT_NAME_BASE@/fs-ganesha/test

%description
Installs test scripts for KVSFS-Ganesha

%prep
%setup -q -n %{sourcename}
# Nothing to do here

%install
mkdir -p %{buildroot}/%{_fs_test_dir}
mkdir -p %{buildroot}/%{_fs_test_dir}/filebench
mkdir -p %{buildroot}/%{_fs_test_dir}/concurrency
mkdir -p %{buildroot}/%{_fs_test_dir}/iozone
mkdir -p %{buildroot}/%{_fs_test_dir}/fio
mkdir -p %{buildroot}/%{_fs_test_dir}/fio/fio_workloads
install -m 755 run_* %{buildroot}/%{_fs_test_dir}/
install -m 755 filebench/*  %{buildroot}/%{_fs_test_dir}/filebench/
install -m 755 concurrency/*  %{buildroot}/%{_fs_test_dir}/concurrency
install -m 755 iozone/*  %{buildroot}/%{_fs_test_dir}/iozone/
install -m 755 fio/fio_params %{buildroot}/%{_fs_test_dir}/fio/
install -m 755 fio/fio_workloads/* %{buildroot}/%{_fs_test_dir}/fio/fio_workloads

#%post

%files
%defattr(-, root, root, -)
%{_fs_test_dir}/run_*
%{_fs_test_dir}/filebench/*
%{_fs_test_dir}/concurrency/*
%{_fs_test_dir}/iozone/*
%{_fs_test_dir}/fio/*

%changelog
* Tue Nov 26 2019 Shreya Karmakar <shreya.karmakar@seagate.com> - 1.0.0
- Initial spec file
