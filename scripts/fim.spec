Summary:FIM
Name:FIM
Version:%{_hitvversion}
Release:%{_hitvrelease}
Source: FIM.tar.gz
License: GPL
#Requires: 
Packager: YYY
Group: Application
AutoReq: no
URL: http://hitv.hisense.com
Buildroot:/usr/src/packages/BUILDROOT/%{name}-%{version}-%{release}.%{_arch}
Vendor: 
Distribution: 

%description 

FIM

Author:
-------
	YYY

%prep
%setup -c
%build
cd src
./autogen.sh
./configure
make
cd ..
%install
mkdir -p ${RPM_BUILD_ROOT}/usr/local/bin/
mkdir -p ${RPM_BUILD_ROOT}/usr/local/lib/
install -m 555 src/zzhelper ${RPM_BUILD_ROOT}/usr/local/bin/
install -m 555 src/lib/libmoncfg.a ${RPM_BUILD_ROOT}/usr/local/lib/


%clean
rm -rf ${RPM_BUILD_ROOT}
rm -rf ${RPM_BUILD_DIR}/*
%files
/usr/local/bin/zzhelper
/usr/local/lib/libmoncfg.a

%changelog
