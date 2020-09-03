%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@CORTX_EFS_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: @PROJECT_NAME@
Version: @CORTX_EFS_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: @PROJECT_NAME_BASE@ file system
License: Seagate
Group: Development/Libraries
Url: GHS://@PROJECT_NAME@
Source: %{sourcename}.tar.gz
BuildRequires: cmake gcc libini_config-devel
Requires: libini_config
Provides: %{name} = %{version}-%{release}

%define on_off_switch() %%{?with_%1:ON}%%{!?with_%1:OFF}

@BCOND_ENABLE_DASSERT@ enable_dassert
%global enable_dassert %{on_off_switch enable_dassert}

# CORTX NSAL library paths
%define	_efs_lib		@PROJECT_NAME@
%define _efs_dir		@INSTALL_DIR_ROOT@/@PROJECT_NAME_BASE@/fs
%define _efs_lib_dir		%{_efs_dir}/lib
%define _efs_bin_dir		%{_efs_dir}/bin
%define _efs_conf_dir		%{_efs_dir}/conf
%define _efs_log_dir		/var/log/@PROJECT_NAME_BASE@/fs
%define _efs_include_dir	%{_includedir}/fs

%description
The @PROJECT_NAME@ is @PROJECT_NAME_BASE@ file system.

%package devel
Summary: Development file for the library cortx-efs
Group: Development/Libraries
Requires: %{name} = %{version}-%{release} pkgconfig
Provides: %{name}-devel = %{version}-%{release}

%description devel
The @PROJECT_NAME@ is @PROJECT_NAME_BASE@ file system.
This package contains tools for @PROJECT_NAME@.

%prep
%setup -q -n %{sourcename}

%build
cmake . -DCORTXUTILSINC:PATH=@CORTXUTILSINC@         \
	-DLIBCORTXUTILS:PATH=@LIBCORTXUTILS@	 \
	-DNSALINC:PATH=@NSALINC@            	 \
	-DLIBNSAL:PATH=@LIBNSAL@		 \
	-DDSALINC:PATH=@DSALINC@		 \
	-DLIBDSAL:PATH=@LIBDSAL@		\
	-DENABLE_DASSERT=%{enable_dassert}	\
	-DPROJECT_NAME_BASE=@PROJECT_NAME_BASE@

make %{?_smp_mflags} || make %{?_smp_mflags} || make

%install

mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_sbindir}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_efs_dir}
mkdir -p %{buildroot}%{_efs_lib_dir}
mkdir -p %{buildroot}%{_efs_bin_dir}
mkdir -p %{buildroot}%{_efs_conf_dir}
mkdir -p %{buildroot}%{_efs_log_dir}
mkdir -p %{buildroot}%{_efs_include_dir}/
mkdir -p %{buildroot}%{_sysconfdir}/cortx
mkdir -p %{buildroot}%{_libdir}/pkgconfig
install -m 644 include/*.h  %{buildroot}%{_efs_include_dir}
install -m 755 lib%{_efs_lib}.so %{buildroot}%{_efs_lib_dir}
install -m 644 cortxfs.conf %{buildroot}%{_efs_conf_dir}
install -m 755 efscli/efscli.py %{buildroot}%{_efs_bin_dir}/efscli
install -m 644 %{_efs_lib}.pc  %{buildroot}%{_libdir}/pkgconfig
ln -s %{_efs_lib_dir}/lib%{_efs_lib}.so %{buildroot}%{_libdir}/lib%{_efs_lib}.so
ln -s %{_efs_conf_dir}/cortxfs.conf %{buildroot}%{_sysconfdir}/cortx/cortxfs.conf
ln -s %{_efs_bin_dir}/efscli %{buildroot}%{_bindir}/efscli

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/lib%{_efs_lib}.so*
%{_efs_lib_dir}/lib%{_efs_lib}.so*
%{_efs_conf_dir}/cortxfs.conf
%config(noreplace) %{_sysconfdir}/cortx/cortxfs.conf
%{_efs_bin_dir}/efscli
%{_bindir}/efscli
%{_efs_log_dir}

%files devel
%defattr(-,root,root)
%{_libdir}/pkgconfig/%{_efs_lib}.pc
%{_efs_include_dir}/*.h

%changelog
* Thu Feb 6 2020 Seagate 1.0.1
- Release candidate

