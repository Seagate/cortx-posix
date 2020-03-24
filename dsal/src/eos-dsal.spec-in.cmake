%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@EOS_DSAL_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: eos-dsal
Version: @EOS_DSAL_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: Data abstraction layer library
License: Propietary
Group: Development/Libraries
Url: http://seagate.com/eos-dsal
Source: %{sourcename}.tar.gz
BuildRequires: cmake gcc libini_config-devel
BuildRequires: @RPM_DEVEL_REQUIRES@
Requires: libini_config @RPM_REQUIRES@
Provides: %{name} = %{version}-%{release}

# Conditionally enable object stores
#
# 1. rpmbuild accepts these options (gpfs as example):
#    --with eos
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

@BCOND_EOS_STORE@ eos_store
%global use_eos_store %{on_off_switch eos_store}

%description
The eos-dsal is Data abstraction layer library.

%package devel
Summary: Development file for the library eos-dsal
Group: Development/Libraries
Requires: %{name} = %{version}-%{release} pkgconfig
Requires: @RPM_DEVEL_REQUIRES@
Provides: %{name}-devel = %{version}-%{release}


%description devel
The eos-dsal is Data abstraction layer library.
This package contains tools for eos-dsal.

%prep
%setup -q -n %{sourcename}

%build
cmake . -DUSE_POSIX_STORE=%{use_posix_store}     \
	-DUSE_POSIX_OBJ=%{use_posix_obj}         \
	-DUSE_EOS_STORE=%{use_eos_store}       \
	-DEOSUTILSINC:PATH=@EOSUTILSINC@         \
	-DLIBEOSUTILS:PATH=@LIBEOSUTILS@

make %{?_smp_mflags} || make %{?_smp_mflags} || make

%install

mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_libdir}/pkgconfig
mkdir -p %{buildroot}%{_includedir}/dsal
mkdir -p %{buildroot}%{_includedir}/dsal/eos
mkdir -p %{buildroot}%{_sysconfdir}/dsal.d
install -m 644 include/dstore.h  %{buildroot}%{_includedir}/dsal
install -m 644 include/dsal.h  %{buildroot}%{_includedir}/dsal
install -m 644 include/eos/eos_dstore.h  %{buildroot}%{_includedir}/dsal/eos
install -m 744 libeos-dsal.so %{buildroot}%{_libdir}
install -m 644 eos-dsal.pc  %{buildroot}%{_libdir}/pkgconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/libeos-dsal.so*

%files devel
%defattr(-,root,root)
%{_libdir}/pkgconfig/eos-dsal.pc
%{_includedir}/dsal/dstore.h
%{_includedir}/dsal/dsal.h
%{_includedir}/dsal/eos/eos_dstore.h

%changelog
* Wed Jan 22 2020 Seagate 1.0.1
- Release candidate

