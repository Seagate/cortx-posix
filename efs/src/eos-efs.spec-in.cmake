%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@EOS_EFS_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: eos-efs
Version: @EOS_EFS_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: EOS file system
License: Seagate
Group: Development/Libraries
Url: http://seagate.com/eos-efs
Source: %{sourcename}.tar.gz
BuildRequires: cmake gcc libini_config-devel
Requires: libini_config
Provides: %{name} = %{version}-%{release}

%define on_off_switch() %%{?with_%1:ON}%%{!?with_%1:OFF}

@BCOND_ENABLE_DASSERT@ enable_dassert
%global enable_dassert %{on_off_switch enable_dassert}

# EOS NSAL library paths
%define _efs_dir		@INSTALL_DIR_ROOT@/@PROJECT_NAME_BASE@/efs
%define _efs_lib_dir		%{_efs_dir}/lib
%define _efs_bin_dir		%{_efs_dir}/bin
%define _efs_conf_dir		%{_efs_dir}/conf
%define _efs_log_dir		/var/log/@PROJECT_NAME_BASE@/efs
%define _efs_include_dir	%{_includedir}/efs

%description
The eos-efs is EOS file system.

%package devel
Summary: Development file for the library eos-efs
Group: Development/Libraries
Requires: %{name} = %{version}-%{release} pkgconfig
Provides: %{name}-devel = %{version}-%{release}

%description devel
The eos-efs EOS file system.
This package contains tools for eos-efs.

%prep
%setup -q -n %{sourcename}

%build
cmake . -DEOSUTILSINC:PATH=@EOSUTILSINC@         \
	-DLIBEOSUTILS:PATH=@LIBEOSUTILS@	 \
	-DNSALINC:PATH=@NSALINC@            	 \
	-DLIBNSAL:PATH=@LIBNSAL@		 \
	-DDSALINC:PATH=@DSALINC@		 \
	-DLIBDSAL:PATH=@LIBDSAL@		\
	-DENABLE_DASSERT=%{enable_dassert}

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
mkdir -p %{buildroot}%{_sysconfdir}/efs
mkdir -p %{buildroot}%{_libdir}/pkgconfig
install -m 644 include/*.h  %{buildroot}%{_efs_include_dir}
install -m 755 libeos-efs.so %{buildroot}%{_efs_lib_dir}
install -m 644 efs.conf %{buildroot}%{_efs_conf_dir}
install -m 755 efscli/efscli.py %{buildroot}%{_efs_bin_dir}/efscli
install -m 644 eos-efs.pc  %{buildroot}%{_libdir}/pkgconfig
ln -s %{_efs_lib_dir}/libeos-efs.so %{buildroot}%{_libdir}/libeos-efs.so
ln -s %{_efs_conf_dir}/efs.conf %{buildroot}%{_sysconfdir}/efs/efs.conf
ln -s %{_efs_bin_dir}/efscli %{buildroot}%{_bindir}/efscli

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/libeos-efs.so*
%{_efs_lib_dir}/libeos-efs.so*
%{_efs_conf_dir}/efs.conf
%config(noreplace) %{_sysconfdir}/efs/efs.conf
%{_efs_bin_dir}/efscli
%{_bindir}/efscli

%files devel
%defattr(-,root,root)
%{_libdir}/pkgconfig/eos-efs.pc
%{_efs_include_dir}/*.h

%changelog
* Thu Feb 6 2020 Seagate 1.0.1
- Release candidate

