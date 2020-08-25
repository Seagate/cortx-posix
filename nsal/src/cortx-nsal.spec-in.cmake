%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@CORTX_NSAL_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: @PROJECT_NAME@
Version: @CORTX_NSAL_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: Namespace abstraction layer library
License: Seagate
Group: Development/Libraries
Url: GHS://@PROJECT_NAME@
Source: %{sourcename}.tar.gz
BuildRequires: cmake gcc libini_config-devel
BuildRequires: @RPM_DEVEL_REQUIRES@
Requires: libini_config @RPM_REQUIRES@
Provides: %{name} = %{version}-%{release}

# CORTX NSAL library paths
%define _nsal_lib		@PROJECT_NAME@
%define _nsal_dir		@INSTALL_DIR_ROOT@/@PROJECT_NAME_BASE@/nsal
%define _nsal_lib_dir		%{_nsal_dir}/lib
%define _nsal_include_dir	%{_includedir}/nsal

# Conditionally enable KVS and object stores
#
# 1. rpmbuild accepts these options:
#    --with cortx
#    --without redis

%define on_off_switch() %%{?with_%1:ON}%%{!?with_%1:OFF}

# A few explanation about %bcond_with and %bcond_without
# /!\ be careful: this syntax can be quite messy
# %bcond_with means you add a "--with" option, default = without this feature
# %bcond_without adds a"--without" so the feature is enabled by default

@BCOND_KVS_REDIS@ kvs_redis
%global use_kvs_redis %{on_off_switch kvs_redis}

@BCOND_KVS_CORTX@ kvs_cortx
%global use_kvs_cortx %{on_off_switch kvs_cortx}

@BCOND_ENABLE_DASSERT@ enable_dassert
%global enable_dassert %{on_off_switch enable_dassert}

%description
The @PROJECT_NAME@ is Namespace Abstraction Layer library.

%package devel
Summary: Development file for the library @PROJECT_NAME@
Group: Development/Libraries
Requires: %{name} = %{version}-%{release} pkgconfig
Requires: @RPM_DEVEL_REQUIRES@
Provides: %{name}-devel = %{version}-%{release}

%description devel
The @PROJECT_NAME@ is Namespace Abstraction Layer library.

%prep
%setup -q -n %{sourcename}

%build
cmake . -DUSE_KVS_REDIS=%{use_kvs_redis}     	\
	-DUSE_KVS_CORTX=%{use_kvs_cortx}       	\
	-DCORTXUTILSINC:PATH=@CORTXUTILSINC@     	\
	-DLIBCORTXUTILS:PATH=@LIBCORTXUTILS@	\
	-DENABLE_DASSERT=%{enable_dassert}	\
	-DPROJECT_NAME_BASE=@PROJECT_NAME_BASE@ \

make %{?_smp_mflags} || make %{?_smp_mflags} || make

%install

mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_nsal_dir}
mkdir -p %{buildroot}%{_nsal_lib_dir}
mkdir -p %{buildroot}%{_nsal_include_dir}/
mkdir -p %{buildroot}%{_sysconfdir}/nsal.d
mkdir -p %{buildroot}%{_libdir}/pkgconfig
install -m 644 include/*.h  %{buildroot}%{_nsal_include_dir}
install -m 755 lib%{_nsal_lib}.so %{buildroot}%{_nsal_lib_dir}
install -m 644 %{_nsal_lib}.pc  %{buildroot}%{_libdir}/pkgconfig
ln -s %{_nsal_lib_dir}/lib%{_nsal_lib}.so %{buildroot}%{_libdir}/lib%{_nsal_lib}.so

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/lib%{_nsal_lib}.so*
%{_nsal_lib_dir}/lib%{_nsal_lib}.so*

%files devel
%defattr(-,root,root)
%{_libdir}/pkgconfig/%{_nsal_lib}.pc
%{_nsal_include_dir}/*.h

%changelog
* Tue Dec 24 2019 Seagate 1.0.1
- Release candidate

