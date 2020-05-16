%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@KVSFS_GANESHA_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: kvsfs-ganesha-test
Version: @KVSFS_GANESHA_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: Test-Tools for KVSFS
License: Seagate
URL: https://seagit.okla.seagate.com/seagate-eos/eos-fs
Requires: eos-efs
Source: %{sourcename}.tar.gz
Provides: %{name} = %{version}-%{release}

# EOS EFS-TEST library paths
%define _efstest_dir		@INSTALL_DIR_ROOT@/@PROJECT_NAME_BASE@/efs/test

%description
Installs test scripts for KVSFS-Ganesha

%prep
%setup -q -n %{sourcename}
# Nothing to do here

%install
mkdir -p %{buildroot}/%{_efstest_dir}
mkdir -p %{buildroot}/%{_efstest_dir}/filebench
mkdir -p %{buildroot}/%{_efstest_dir}/concurrency
mkdir -p %{buildroot}/%{_efstest_dir}/iozone
mkdir -p %{buildroot}/%{_efstest_dir}/fio
mkdir -p %{buildroot}/%{_efstest_dir}/fio/fio_workloads
install -m 755 run_* %{buildroot}/%{_efstest_dir}/
install -m 755 filebench/*  %{buildroot}/%{_efstest_dir}/filebench/
install -m 755 concurrency/*  %{buildroot}/%{_efstest_dir}/concurrency
install -m 755 iozone/*  %{buildroot}/%{_efstest_dir}/iozone/
install -m 755 fio/fio_params %{buildroot}/%{_efstest_dir}/fio/
install -m 755 fio/fio_workloads/* %{buildroot}/%{_efstest_dir}/fio/fio_workloads

#%post

%files
%defattr(-, root, root, -)
%{_efstest_dir}/run_*
%{_efstest_dir}/filebench/*
%{_efstest_dir}/concurrency/*
%{_efstest_dir}/iozone/*
%{_efstest_dir}/fio/*
%{_efstest_dir}/fio/fio_workloads/*

%changelog
* Tue Nov 26 2019 Shreya Karmakar <shreya.karmakar@seagate.com> - 1.0.0
- Initial spec file
