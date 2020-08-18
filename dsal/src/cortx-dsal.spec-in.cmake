%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@CORTX_DSAL_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: @PROJECT_NAME@
Version: @CORTX_DSAL_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: Data abstraction layer library
License: Seagate
Group: Development/Libraries
Url: GHS://corts-dsal
Source: %{sourcename}.tar.gz
BuildRequires: cmake gcc libini_config-devel
BuildRequires: @RPM_DEVEL_REQUIRES@
Requires: libini_config @RPM_REQUIRES@
Provides: %{name} = %{version}-%{release}

# CORTX DSAL library paths
%define _dsal_lib		@PROJECT_NAME@
%define _dsal_dir		@INSTALL_DIR_ROOT@/@PROJECT_NAME_BASE@/dsal
%define _dsal_lib_dir		%{_dsal_dir}/lib
%define _dsal_include_dir	%{_includedir}/dsal

# Conditionally enable object stores
#
# 1. rpmbuild accepts these options (gpfs as example):
#    --with cortx
#    --without redis

%define on_off_switch() %%{?with_%1:ON}%%{!?with_%1:OFF}

# A few explanation about %bcond_with and %bcond_without
# /!\ be careful: this syntax can be quite messy
# %bcond_with means you add a "--with" option, default = without this feature
# %bcond_without adds a"--without" so the feature is enabled by default

@BCOND_POSIX_STORE@ posix_store
%global use_posix_store %{on_off_switch posix_store}

@BCOND_POSIX_OBJ@ posix_obj
%global use_posix_obj %{on_off_switch posix_obj}

@BCOND_CORTX_STORE@ cortx_store
%global use_cortx_store %{on_off_switch cortx_store}

@BCOND_ENABLE_DASSERT@ enable_dassert
%global enable_dassert %{on_off_switch enable_dassert}

%description
The @PROJECT_NAME@ is Data Store Abstraction Layer library.

%package devel
Summary: Development file for the library @PROJECT_NAME@
Group: Development/Libraries
Requires: %{name} = %{version}-%{release} pkgconfig
Requires: @RPM_DEVEL_REQUIRES@
Provides: %{name}-devel = %{version}-%{release}

%description devel
The @PROJECT_NAME@ is Data Store Abstraction Layer library.

%prep
%setup -q -n %{sourcename}

%build
cmake . -DUSE_POSIX_STORE=%{use_posix_store}     \
	-DUSE_POSIX_OBJ=%{use_posix_obj}         \
	-DUSE_CORTX_STORE=%{use_cortx_store}       \
	-DCORTXUTILSINC:PATH=@CORTXUTILSINC@         \
	-DLIBCORTXUTILS:PATH=@LIBCORTXUTILS@	\
	-DENABLE_DASSERT=%{enable_dassert}	\
	-DPROJECT_NAME_BASE=@PROJECT_NAME_BASE@

make %{?_smp_mflags} || make %{?_smp_mflags} || make

%install

mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_dsal_dir}
mkdir -p %{buildroot}%{_dsal_lib_dir}
mkdir -p %{buildroot}%{_dsal_include_dir}/
mkdir -p %{buildroot}%{_sysconfdir}/dsal.d
mkdir -p %{buildroot}%{_libdir}/pkgconfig
install -m 644 include/*.h  %{buildroot}%{_dsal_include_dir}
install -m 755 lib%{_dsal_lib}.so %{buildroot}%{_dsal_lib_dir}
install -m 644 %{_dsal_lib}.pc  %{buildroot}%{_libdir}/pkgconfig
ln -s %{_dsal_lib_dir}/lib%{_dsal_lib}.so %{buildroot}%{_libdir}/lib%{_dsal_lib}.so

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/lib%{_dsal_lib}.so*
%{_dsal_lib_dir}/lib%{_dsal_lib}.so*

%files devel
%defattr(-,root,root)
%{_libdir}/pkgconfig/%{_dsal_lib}.pc
%{_dsal_include_dir}/*.h

%changelog
* Wed Jan 22 2020 Seagate 1.0.1
- Release candidate

