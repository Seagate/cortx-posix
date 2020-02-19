Name:		libevhtp
Version:	1.2.18
Release:	2%{dist}
Summary:	Create extremely-fast and secure embedded HTTP servers with ease.

Group:		System Environment/Libraries
License:	BSD

URL:		https://github.com/criticalstack/libevhtp
Source0:	%{name}-1.2.18.tar.gz

BuildRequires:	gcc, make, redhat-rpm-config
BuildRequires:	doxygen openssl-devel libevent

%define _evhtp_include_install_dir	local/include/
%define _evhtp_lib_install_dir		local/lib/

%description
Create extremely-fast and secure embedded HTTP servers with ease.

%package	devel
Summary:	Header files, libraries for %{name}
Group:		Development/Libraries
Requires:	%{name} = %{version}-%{release}

%description	devel
This package contains the header files, shared libraries for %{name}.
If you like to develop programs using %{name}, you will need to
install %{name}-devel.

%prep
%setup -q -n %{name}-%{version}

%build
cd build
cmake3 -DBUILD_SHARED_LIBS=ON ..
make %{?_smp_mflags} || make %{?_smp_mflags} || make

%install
rm -rf $RPM_BUILD_ROOT
cd build
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0755)
%{_exec_prefix}/%{_evhtp_lib_install_dir}/libevhtp*.so.*

%files devel
%defattr(-,root,root,0755)
%{_exec_prefix}/%{_evhtp_include_install_dir}/evhtp.h
%dir %{_exec_prefix}/%{_evhtp_include_install_dir}/evhtp
%{_exec_prefix}/%{_evhtp_include_install_dir}/evhtp/*.h
%dir %{_exec_prefix}/%{_evhtp_include_install_dir}/evhtp/sys
%{_exec_prefix}/%{_evhtp_include_install_dir}/evhtp/sys/*.h
%{_exec_prefix}/%{_evhtp_lib_install_dir}/libevhtp*.so.*
%{_exec_prefix}/%{_evhtp_lib_install_dir}/pkgconfig/evhtp.pc
%{_exec_prefix}/%{_evhtp_lib_install_dir}/libevhtp.so
%dir %{_exec_prefix}/%{_evhtp_lib_install_dir}/cmake/%{name}
%{_exec_prefix}/%{_evhtp_lib_install_dir}/cmake/%{name}/*.cmake

%changelog
