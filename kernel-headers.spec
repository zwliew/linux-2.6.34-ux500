Summary: Headers describing the kernel ABI
Name: kernel-headers
Group: System Environment/Kernel
License: GPLv2
URL: http://www.kernel.org/


%define kversion 2.6.34
Version: %{kversion}
Release: 1.1
BuildRoot: %{_tmppath}/kernel-%{kversion}-root
Provides: kernel-headers = %{kversion}

#
# A note about versions and patches.
# This package is supposed to provide the official, stable kernel ABI, as specified
# by the kernels released by Linus Torvalds. Release candidate kernels do not
# have a stable ABI yet, and should thus not be in this package.
#
# Likewise, if there are distro patches in the kernel package that would have the
# unfortunate side effect of extending the kernel ABI, these extensions are unofficial
# and applications should not depend on these extensions, and hence, these extensions
# should not be part of this package.
#
# Applications that want headers from the kernel that are not in this package need
# to realize that what they are using is not a stable ABI, and also need to include
# a provide a copy of the header they are interested in into their own package/source
# code.
#


Source0: linux-%{kversion}.tar.bz2

BuildRequires:  findutils,  make >= 3.78, diffutils, gawk


%description
The kernel-headers package contains the header files that describe
the kernel ABI. This package is mostly used by the C library and some
low level system software, and is only used indirectly by regular
applications.


%prep
%setup -q -n linux-%{kversion}


%build
make ARCH=arm allyesconfig

%install

make INSTALL_HDR_PATH=$RPM_BUILD_ROOT/usr headers_install


# glibc provides scsi headers for itself, for now
find  $RPM_BUILD_ROOT/usr/include -name ".install" | xargs rm -f
find  $RPM_BUILD_ROOT/usr/include -name "..install.cmd" | xargs rm -f
rm -rf $RPM_BUILD_ROOT/usr/include/scsi
rm -f $RPM_BUILD_ROOT/usr/include/asm*/atomic.h
rm -f $RPM_BUILD_ROOT/usr/include/asm*/io.h
rm -f $RPM_BUILD_ROOT/usr/include/asm*/irq.h

#
# Unfortunately we have a naming clash between the kernel ABI headers and
# the userland MESA headers, both occupy /usr/include/drm. We'll move the
# kernel out of the way and hope MESA doesn't do something stupid and
# uses an incompatible API/ABI
#
mv $RPM_BUILD_ROOT/usr/include/drm $RPM_BUILD_ROOT/usr/include/kerneldrm


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
/usr/include/*

%changelog
* Tue Jun 8 2010 Robert Rosengren <robert.rosengren@stericsson.com> - 2.6.34
- Updated to STE Kernel version 2.6.34.
* Wed May 19 2010 Robert Rosengren <robert.rosengren@stericsson.com> 
- Initial STE version for kernel 2.6.29. 
* Mon Mar  1 2010 Arjan van de Ven <arjan@linux.inte.ocm> - 2.6.33
- Update to 2.6.33
* Fri Jan  8 2010 Yong Wang <yong.y.wang@intel.com> 2.6.32
- Update to 2.6.32
* Fri Oct  2 2009 Anas Nashif <anas.nashif@intel.com> - 2.6.31
- Update to 2.6.31
* Wed Jul  8 2009 Arjan van de Ven <arjan@linux.intel.com> 2.6.30
- Update to 2.6.30
* Wed Jan  7 2009 Anas Nashif <anas.nashif@intel.com> 2.6.28
- Update to 2.6.28
