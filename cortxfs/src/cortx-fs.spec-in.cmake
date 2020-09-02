%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@CORTXFS_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: @PROJECT_NAME@
Version: @CORTXFS_BASE_VERSION@
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
%define	_cortxfs_lib		@PROJECT_NAME@
%define _cortxfs_dir		@INSTALL_DIR_ROOT@/@PROJECT_NAME_BASE@/fs
%define _cortxfs_lib_dir		%{_cortxfs_dir}/lib
%define _cortxfs_bin_dir		%{_cortxfs_dir}/bin
%define _cortxfs_conf_dir		%{_cortxfs_dir}/conf
%define _cortxfs_log_dir		/var/log/@PROJECT_NAME_BASE@/fs
%define _cortxfs_include_dir	%{_includedir}/fs

%description
The @PROJECT_NAME@ is @PROJECT_NAME_BASE@ file system.

%package devel
Summary: Development file for the library cortxfs
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
mkdir -p %{buildroot}%{_cortxfs_dir}
mkdir -p %{buildroot}%{_cortxfs_lib_dir}
mkdir -p %{buildroot}%{_cortxfs_bin_dir}
mkdir -p %{buildroot}%{_cortxfs_conf_dir}
mkdir -p %{buildroot}%{_cortxfs_log_dir}
mkdir -p %{buildroot}%{_cortxfs_include_dir}/
mkdir -p %{buildroot}%{_sysconfdir}/cortx
mkdir -p %{buildroot}%{_libdir}/pkgconfig
install -m 644 include/*.h  %{buildroot}%{_cortxfs_include_dir}
install -m 755 lib%{_cortxfs_lib}.so %{buildroot}%{_cortxfs_lib_dir}
install -m 644 cortxfs.conf %{buildroot}%{_cortxfs_conf_dir}
install -m 755 cortxfscli/cortxfscli.py %{buildroot}%{_cortxfs_bin_dir}/cortxfscli
install -m 644 %{_cortxfs_lib}.pc  %{buildroot}%{_libdir}/pkgconfig
ln -s %{_cortxfs_lib_dir}/lib%{_cortxfs_lib}.so %{buildroot}%{_libdir}/lib%{_cortxfs_lib}.so
ln -s %{_cortxfs_conf_dir}/cortxfs.conf %{buildroot}%{_sysconfdir}/cortx/cortxfs.conf
ln -s %{_cortxfs_bin_dir}/cortxfscli %{buildroot}%{_bindir}/cortxfscli

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/lib%{_cortxfs_lib}.so*
%{_cortxfs_lib_dir}/lib%{_cortxfs_lib}.so*
%{_cortxfs_conf_dir}/cortxfs.conf
%config(noreplace) %{_sysconfdir}/cortx/cortxfs.conf
%{_cortxfs_bin_dir}/cortxfscli
%{_bindir}/cortxfscli
%{_cortxfs_log_dir}

%files devel
%defattr(-,root,root)
%{_libdir}/pkgconfig/%{_cortxfs_lib}.pc
%{_cortxfs_include_dir}/*.h

%changelog
* Thu Feb 6 2020 Seagate 1.0.1
- Release candidate

