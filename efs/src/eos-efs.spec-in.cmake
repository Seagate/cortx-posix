%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@EOS_EFS_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: eos-efs
Version: @EOS_EFS_BASE_VERSION@
Release: %{dev_version}%{?dist}
Summary: EOS file system
License: Propietary
Group: Development/Libraries
Url: http://seagate.com/eos-efs
Source: %{sourcename}.tar.gz
BuildRequires: cmake gcc libini_config-devel
Requires: libini_config
Provides: %{name} = %{version}-%{release}

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
	-DLIBDSAL:PATH=@LIBDSAL@

make %{?_smp_mflags} || make %{?_smp_mflags} || make

%install

mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_libdir}/pkgconfig
mkdir -p %{buildroot}%{_includedir}/efs
mkdir -p %{buildroot}%{_sysconfdir}/efs.d
mkdir -p %{buildroot}/opt/seagate/eos/efs/bin
install -m 644 include/efs.h  %{buildroot}%{_includedir}/efs
install -m 644 include/efs_fh.h  %{buildroot}%{_includedir}/efs
install -m 744 libeos-efs.so %{buildroot}%{_libdir}
install -m 644 eos-efs.pc  %{buildroot}%{_libdir}/pkgconfig
install -m 755 efscli/client.py %{buildroot}/opt/seagate/eos/efs/bin/client.py
install -m 755 efscli/commands.py %{buildroot}/opt/seagate/eos/efs/bin/commands.py
install -m 755 efscli/efscli.py %{buildroot}/opt/seagate/eos/efs/bin/efscli


%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/libeos-efs.so*

%files devel
%defattr(-,root,root)
%{_libdir}/pkgconfig/eos-efs.pc
%{_includedir}/efs/efs.h
%{_includedir}/efs/efs_fh.h
/opt/seagate/eos/efs/bin/client.py
/opt/seagate/eos/efs/bin/commands.py
/opt/seagate/eos/efs/bin/efscli

%changelog
* Thu Feb 6 2020 Seagate 1.0.1
- Release candidate

