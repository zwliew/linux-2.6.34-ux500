#
# Spec file originally created for Fedora, modified for Moblin Linux
#

Summary: The Linux kernel (the core of the Linux operating system)


# For a stable, released kernel, released_kernel should be 1. For rawhide
# and/or a kernel built from an rc snapshot, released_kernel should
# be 0.
%define released_kernel 1

# Versions of various parts

# base_sublevel is the kernel version we're starting with and patching
# on top of -- for example, 2.6.22-rc7 starts with a 2.6.21 base,
# which yields a base_sublevel of 21.

%define base_sublevel 34


## If this is a released kernel ##
%if 0%{?released_kernel}
# Do we have a 2.6.21.y update to apply?
%define stable_update 0
# Set rpm version accordingly
%if 0%{?stable_update}
%define stablerev .%{stable_update}
%endif
%define rpmversion 2.6.%{base_sublevel}%{?stablerev}

## The not-released-kernel case ##
%else
# The next upstream release sublevel (base_sublevel+1)
%define upstream_sublevel %(expr %{base_sublevel} + 1)
# The rc snapshot level

%define rcrev 8


%if 0%{?rcrev}
%define rctag ~rc%rcrev
%endif

%if !0%{?rcrev}
%define rctag ~rc0
%endif

# Set rpm version accordingly
%define rpmversion 2.6.%{upstream_sublevel}%{?rctag}
%endif

# The kernel tarball/base version
%define kversion 2.6.%{base_sublevel}

%define make_target bzImage

%define KVERREL %{version}-%{release}
%define hdrarch %_target_cpu

%define all_x86 i386 i586 i686 %{ix86}

%define all_arm arm %{arm}

# Overrides for generic default options
%define all_arch_configs kernel-*.config

# Per-arch tweaks

%ifarch %{all_x86}
%define all_arch_configs kernel-*.config
%define image_install_path boot
%define hdrarch i386
%define kernel_image arch/x86/boot/bzImage
%endif

%ifarch x86_64
%define all_arch_configs kernel-%{version}-x86_64*.config
%define image_install_path boot
%define kernel_image arch/x86/boot/bzImage
%endif

%ifarch %{all_arm}
%define all_arch_configs kernel-*.config
%define image_install_path boot
%define kernel_image arch/arm/boot/zImage
%define make_target uImage
%endif

%define oldconfig_target nonint_oldconfig

#
# Packages that need to be installed before the kernel is, because the %post
# scripts use them.
#
%define kernel_prereq  fileutils, module-init-tools, mkinitrd >= 6.0.39-1

#
# Disable debug info
#
%define debug_package %{nil}

#
# This macro does requires, provides, conflicts, obsoletes for a kernel package.
#	%%kernel_reqprovconf <subpackage>
# It uses any kernel_<subpackage>_conflicts and kernel_<subpackage>_obsoletes
# macros defined above.
#
%define kernel_reqprovconf \
Provides: kernel = %{rpmversion}-%{release}\
Provides: kernel-drm = 4.3.0\
Provides: kernel-uname-r = %{KVERREL}%{?1:-%{1}}\
Requires(pre): %{kernel_prereq}\
%{?1:%{expand:%%{?kernel_%{1}_conflicts:Conflicts: %%{kernel_%{1}_conflicts}}}}\
%{?1:%{expand:%%{?kernel_%{1}_obsoletes:Obsoletes: %%{kernel_%{1}_obsoletes}}}}\
%{?1:%{expand:%%{?kernel_%{1}_provides:Provides: %%{kernel_%{1}_provides}}}}\
# We can't let RPM do the dependencies automatic because it'll then pick up\
# a correct but undesirable perl dependency from the module headers which\
# isn't required for the kernel proper to function\
AutoReq: no\
AutoProv: yes\
%{nil}

Name: kernel%{?variant}
Group: System/Kernel
License: GPLv2
URL: http://www.kernel.org/
Version: %{rpmversion}
Release: 1

%kernel_reqprovconf

#
# List the packages used during the kernel build
#
BuildRequires: module-init-tools, bash >= 2.03, sh-utils
BuildRequires:  findutils,  make >= 3.78
#BuildRequires: linux-firmware
BuildRequires: elfutils-libelf-devel binutils-devel
BuildRequires: u-boot-tools
BuildRequires:  fdupes

Source0: linux-%{kversion}.tar.gz
Source100: kernel-rpmlintrc


#
# Reminder of the patch filename format:
# linux-<version it is supposed to be upstream>-<description-separated-with-dashes>.patch
#


#
# Stable patch - critical bugfixes
#
#Patch: patch-2.6.33.2.bz2

#
# STE: scripts for automatically listing and applying patches
#
%define ste_patch_archive ste-patches.tar.gz
%define ste_patches \
%( \
if [ -e %{_sourcedir}/%{ste_patch_archive} ] \
then \
tar -C %{_sourcedir} -xzf %{_sourcedir}/%{ste_patch_archive} \
for i in `cd %{_sourcedir} && ls -1 *.patch.gz` \
do \
  num=`echo $i | cut -d- -f1 | sed 's|^0*||g'` \
  num=$(($num-1)) \
  name=`echo $i` \
  echo 'Patch'$num':\ '$name \
done \
fi \
)

%define ste_apply_patches \
%( \
if [ -e %{_sourcedir}/%{ste_patch_archive} ] \
then \
for i in `cd %{_sourcedir} && ls -1 *.patch.gz` \
do \
  num=`echo $i | cut -d- -f1 | sed 's|^0*||g'` \
  num=$(($num-1)) \
  echo '%patch'$num' -p1'\
#  echo 'patch -i '%{_sourcedir}/$i' -p1 --verbose'\
done \
fi \
)

%ste_patches


BuildRoot: %{_tmppath}/kernel-%{KVERREL}-root

%description
The kernel package contains the Linux kernel (vmlinuz), the core of any
Linux operating system.  The kernel handles the basic functions
of the operating system: memory allocation, process allocation, device
input and output, etc.


#
# This macro creates a kernel-<subpackage>-devel package.
#	%%kernel_devel_package <subpackage> <pretty-name>
#
%define kernel_devel_package() \
%package %{?1:%{1}-}devel\
Summary: Development package for building kernel modules to match the %{?2:%{2} }kernel\
Group: System/Kernel\
Provides: kernel%{?1:-%{1}}-devel = %{version}-%{release}\
Provides: kernel-devel = %{version}-%{release}%{?1:-%{1}}\
Provides: kernel-devel-uname-r = %{KVERREL}%{?1:-%{1}}\
Requires: kernel%{?1:-%{1}} = %{version}-%{release}\
AutoReqProv: no\
Requires(pre): /usr/bin/find\
%description -n kernel%{?variant}%{?1:-%{1}}-devel\
This package provides kernel headers and makefiles sufficient to build modules\
against the %{?2:%{2} }kernel package.\
%{nil}

#
# This macro creates a kernel-<subpackage> and its -devel too.
#	%%define variant_summary The Linux kernel compiled for <configuration>
#	%%kernel_variant_package [-n <pretty-name>] <subpackage>
#
%define kernel_variant_package(n:) \
%package %1\
Summary: %{variant_summary}\
Group: System/Kernel\
%kernel_reqprovconf\
%{expand:%%kernel_devel_package %1 %{!?-n:%1}%{?-n:%{-n*}}}\
%{nil}


# First the auxiliary packages of the main kernel package.
%kernel_devel_package

%package -n perf
Summary: The 'perf' performance counter tool
Group: System/Performance
Obsoletes: oprofile <= 0.9.5
%description -n perf
This package provides the "perf" tool that can be used to monitor performance counter events
as well as various kernel internal events.




# Now, each variant package.

%define variant_summary Kernel for the ste u8500
%kernel_variant_package u8500
%description u8500
This package contains the kernel optimized for the Nokia N900 device


%prep
# First we unpack the kernel tarball.
# If this isn't the first make prep, we use links to the existing clean tarball
# which speeds things up quite a bit.

# Update to latest upstream.
%if 0%{?released_kernel}
%define vanillaversion 2.6.%{base_sublevel}
# released_kernel with stable_update available case
%if 0%{?stable_update}
%define vanillaversion 2.6.%{base_sublevel}.%{stable_update}
%endif
# non-released_kernel case
%else
%if 0%{?rcrev}
%define vanillaversion 2.6.%{upstream_sublevel}-rc%{rcrev}
%endif
%else
# pre-{base_sublevel+1}-rc1 case
%endif

if [ ! -d kernel-%{kversion}/vanilla-%{vanillaversion} ]; then
  # Ok, first time we do a make prep.
  rm -f pax_global_header
%setup -q -n kernel-%{kversion} -c
  mv linux-%{kversion} vanilla-%{vanillaversion}
  cd vanilla-%{vanillaversion}


%if 0%{?rcrev}
# patch-2.6.%{upstream_sublevel}-rc%{rcrev}.bz2
#%patch00 -p1
%endif

#
# Reminder of the patch filename format:
# linux-<version it is supposed to be upstream>-<description-separated-with-dashes>.patch
#


#
# Stable patch - critical bugfixes
#

#
# STE: Applying all patches generated from script
#
%ste_apply_patches
# STE: Workaround chmod 644 on all headers, since patch dont like chmod'ing (?)
find . -name "*.h" -exec chmod 644 {} \; 
# STE: Workaround end

cd ..

else
  # We already have a vanilla dir.
  cd kernel-%{kversion}
  if [ -d linux-%{kversion} ]; then
     # Just in case we ctrl-c'd a prep already
     rm -rf deleteme
     # Move away the stale away, and delete in background.
     mv linux-%{kversion} deleteme
     rm -rf deleteme &
  fi
fi

cp -rl vanilla-%{vanillaversion} linux-%{kversion}

cd linux-%{kversion}

# Any further pre-build tree manipulations happen here.

# only deal with configs if we are going to build for the arch

# get rid of unwanted files resulting from patch fuzz
find . \( -name "*.orig" -o -name "*~" \) -exec rm -f {} \; >/dev/null

cd ..

###
### build
###
%build


cp_vmlinux()
{
  eu-strip --remove-comment -o "$2" "$1"
}

BuildKernel() {
    MakeTarget=$1
    KernelImage=$2
    Flavour=$3
    InstallName=${4:-vmlinuz}

    # Pick the right config file for the kernel we're building
    Config=kernel${Flavour:+-${Flavour}}.config
    DevelDir=/usr/src/kernels/%{KVERREL}${Flavour:+-${Flavour}}

    # When the bootable image is just the ELF kernel, strip it.
    # We already copy the unstripped file into the debuginfo package.
    if [ "$KernelImage" = vmlinux ]; then
      CopyKernel=cp_vmlinux
    else
      CopyKernel=cp
    fi

    KernelVer=%{version}-%{release}${Flavour:+-${Flavour}}
    echo BUILDING A KERNEL FOR ${Flavour} %{_target_cpu}...

    # make sure EXTRAVERSION says what we want it to say
    perl -p -i -e "s/^EXTRAVERSION.*/EXTRAVERSION = %{?stablerev}%{?rctag}-%{release}/" Makefile
#    perl -p -i -e "s/^EXTRAVERSION.*/EXTRAVERSION = %{?stablerev}%{?rctag}-%{release}${Flavour:+-${Flavour}}/" Makefile

    # and now to start the build process

    make -s mrproper

    # Set config
    make %{?_smp_mflags} mop500$(DEBUG)_power_defconfig
    # STE: Disable multibuffer and multitouch. 
    scripts/config --file .config --enable DISPLAY_GENERIC_DSI_PRIMARY_AUTO_SYNC --enable DISPLAY_GENERIC_DSI_SECONDARY_AUTO_SYNC 
    # Does not exist in the 34 kernel yet
    #scripts/config --file .config --disable U8500_TSC_MULTITOUCH

    # Enable pmem
    scripts/config --file .config --enable CONFIG_ANDROID_PMEM

    # Enable hwmem
    scripts/config --file .config --enable CONFIG_HWMEM

    # STE: Enable conf for external sd-cards. 
    scripts/config --file .config --enable LEVELSHIFTER_HREF_V1_PLUS
    # STE: Enable g_multi USB gadget with RNDIS, CDC Serial and Storage configuration.
    scripts/config --file .config --module CONFIG_USB_G_MULTI --enable CONFIG_USB_G_MULTI_RNDIS --disable USB_G_MULTI_CDC
    # STE: Enable CONFIG_MCDE_FB_AVOID_REALLOC to avoid reallocations.
    scripts/config --file .config --enable CONFIG_MCDE_FB_AVOID_REALLOC
    scripts/config --file .config --enable CONFIG_DISPLAY_GENERIC_DSI_PRIMARY_AUTO_SYNC

    Arch="x86"
%ifarch %{all_arm}
    Arch="arm"
%endif
    echo USING ARCH=$Arch

    # Build    
    make %{?_smp_mflags} ARCH=$Arch $MakeTarget
    make %{?_smp_mflags} ARCH=$Arch modules
    
    # STE: uImage placed for now in boot.
    cp -f arch/arm/boot/uImage $RPM_BUILD_ROOT/boot/uImage-$KernelVer
    chmod 755 $RPM_BUILD_ROOT/boot/uImage-$KernelVer


    # Start installing the results
    install -m 644 .config $RPM_BUILD_ROOT/boot/config-$KernelVer
    install -m 644 System.map $RPM_BUILD_ROOT/boot/System.map-$KernelVer
    touch $RPM_BUILD_ROOT/boot/initrd-$KernelVer.img
    if [ -f arch/$Arch/boot/zImage.stub ]; then
      cp arch/$Arch/boot/zImage.stub $RPM_BUILD_ROOT/boot/zImage.stub-$KernelVer || :
    fi
    $CopyKernel $KernelImage \
    		$RPM_BUILD_ROOT/boot/$InstallName-$KernelVer
    chmod 755 $RPM_BUILD_ROOT/boot/$InstallName-$KernelVer

    mkdir -p $RPM_BUILD_ROOT/lib/modules/$KernelVer
    make ARCH=$Arch INSTALL_MOD_PATH=$RPM_BUILD_ROOT modules_install KERNELRELEASE=$KernelVer
%ifnarch %{all_arm}
    make -s ARCH=$Arch INSTALL_MOD_PATH=$RPM_BUILD_ROOT vdso_install KERNELRELEASE=$KernelVer
%endif

    # And save the headers/makefiles etc for building modules against
    #
    # This all looks scary, but the end result is supposed to be:
    # * all arch relevant include/ files
    # * all Makefile/Kconfig files
    # * all script/ files

    rm -f $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    rm -f $RPM_BUILD_ROOT/lib/modules/$KernelVer/source
    mkdir -p $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    (cd $RPM_BUILD_ROOT/lib/modules/$KernelVer ; ln -s build source)
    # dirs for additional modules per module-init-tools, kbuild/modules.txt
    # first copy everything
    cp --parents `find  -type f -name "Makefile*" -o -name "Kconfig*"` $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    cp Module.symvers $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    cp System.map $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    if [ -s Module.markers ]; then
      cp Module.markers $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    fi
    # then drop all but the needed Makefiles/Kconfig files
    rm -rf $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/Documentation
    rm -rf $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/scripts
    rm -rf $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
    cp .config $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    cp -a scripts $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    if [ -d arch/%{_arch}/scripts ]; then
      cp -a arch/%{_arch}/scripts $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/arch/%{_arch} || :
    fi
    if [ -f arch/%{_arch}/*lds ]; then
      cp -a arch/%{_arch}/*lds $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/arch/%{_arch}/ || :
    fi
    rm -f $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/scripts/*.o
    rm -f $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/scripts/*/*.o
    cp -a --parents arch/%{_arch}/include $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    if [ "$Flavour" = u8500 ]; then
      # U8500 specific include files
      cp -a --parents arch/arm/mach-ux500/include $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
      cp -a --parents arch/arm/plat-nomadik/include $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    fi
    mkdir -p $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
    cd include
    cp -a acpi asm-generic config crypto drm generated keys linux math-emu media mtd net pcmcia rdma rxrpc scsi sound video trace $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
    #cp -a `readlink asm` $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
    # While arch/powerpc/include/asm is still a symlink to the old
    # include/asm-ppc{64,} directory, include that in kernel-devel too.
    if [ "$Arch" = "powerpc" -a -r ../arch/powerpc/include/asm ]; then
      cp -a `readlink ../arch/powerpc/include/asm` $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
      mkdir -p $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/arch/$Arch/include
      pushd $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/arch/$Arch/include
      ln -sf ../../../include/asm-ppc* asm
      popd
    fi

    # Find and remove .gitignore
    find $RPM_BUILD_ROOT/lib/modules/$KernelVer/build -name .gitignore | xargs --no-run-if-empty rm -f

    # Make sure the Makefile and version.h have a matching timestamp so that
    # external modules can be built
    touch -r $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/Makefile $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include/linux/version.h
    touch -r $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/.config $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include/generated/autoconf.h
    # Copy .config to include/config/auto.conf so "make prepare" is unnecessary.
    cp $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/.config $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include/config/auto.conf
    cd ..

    #
    # save the vmlinux file for kernel debugging into the kernel-debuginfo rpm
    #
    # cp vmlinux $RPM_BUILD_ROOT/lib/modules/$KernelVer

    find $RPM_BUILD_ROOT/lib/modules/$KernelVer -name "*.ko" -type f >modnames

    # mark modules executable so that strip-to-file can strip them
    xargs --no-run-if-empty chmod u+x < modnames 
    
    # Generate a list of modules for block and networking.

    fgrep /drivers/ modnames | xargs --no-run-if-empty nm -upA |
    sed -n 's,^.*/\([^/]*\.ko\):  *U \(.*\)$,\1 \2,p' > drivers.undef

    collect_modules_list()
    {
      sed -r -n -e "s/^([^ ]+) \\.?($2)\$/\\1/p" drivers.undef |
      LC_ALL=C sort -u > $RPM_BUILD_ROOT/lib/modules/$KernelVer/modules.$1
    }

    collect_modules_list networking \
    			 'register_netdev|ieee80211_register_hw|usbnet_probe'
    collect_modules_list block \
    			 'ata_scsi_ioctl|scsi_add_host|blk_init_queue|register_mtd_blktrans'

    # remove files that will be auto generated by depmod at rpm -i time
    for i in alias ccwmap dep ieee1394map inputmap isapnpmap ofmap pcimap seriomap symbols usbmap
    do
      rm -f $RPM_BUILD_ROOT/lib/modules/$KernelVer/modules.$i
    done

    # Move the devel headers out of the root file system
    mkdir -p $RPM_BUILD_ROOT/usr/src/kernels
    mv $RPM_BUILD_ROOT/lib/modules/$KernelVer/build $RPM_BUILD_ROOT/$DevelDir
    ln -sf ../../..$DevelDir $RPM_BUILD_ROOT/lib/modules/$KernelVer/build

    # Last but not least, remove few kernel dupes..
    %fdupes $RPM_BUILD_ROOT/$DevelDir
}


###
# DO it...
###

# prepare directories
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/boot

cd linux-%{kversion}
BuildKernel %make_target %kernel_image u8500

###
### install
###

%define install  %{?_enable_debug_packages:%{?buildsubdir:%{debug_package}}}\
%%install

%install

# NOTE - Linux Kernel already installed in build section. Now only installing perf tool.
cd linux-%{kversion}


cd tools/perf
make DESTDIR=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT/usr/bin/
mkdir -p $RPM_BUILD_ROOT/usr/libexec/
mv $RPM_BUILD_ROOT/bin/* $RPM_BUILD_ROOT/usr/bin/
mv $RPM_BUILD_ROOT/libexec/* $RPM_BUILD_ROOT/usr/libexec/

rm -rf $RPM_BUILD_ROOT/lib/firmware

%fdupes $RPM_BUILD_ROOT/usr/libexec/



###
### clean
###

%clean
rm -rf $RPM_BUILD_ROOT

###
### scripts
###

#
# This macro defines a %%post script for a kernel*-devel package.
#	%%kernel_devel_post <subpackage>
#
%define kernel_devel_post() \
%{expand:%%post %{?1:%{1}-}devel}\
if [ -f /etc/sysconfig/kernel ]\
then\
    . /etc/sysconfig/kernel || exit $?\
fi\
if [ "$HARDLINK" != "no" -a -x /usr/sbin/hardlink ]\
then\
    (cd /usr/src/kernels/%{KVERREL}%{?1:-%{1}} &&\
     /usr/bin/find . -type f | while read f; do\
       hardlink -c /usr/src/kernels/*.fc*.*/$f $f\
     done)\
fi\
%{nil}

# This macro defines a %%posttrans script for a kernel package.
#	%%kernel_variant_posttrans [-v <subpackage>] [-s <s> -r <r>] <mkinitrd-args>
# More text can follow to go at the end of this variant's %%post.
#
%define kernel_variant_posttrans(s:r:v:) \
%{expand:%%posttrans %{?-v*}}\
/sbin/new-kernel-pkg --package kernel%{?-v:-%{-v*}} --rpmposttrans %{KVERREL}%{?-v:-%{-v*}} || exit $?\
%{nil}

#
# This macro defines a %%post script for a kernel package and its devel package.
#	%%kernel_variant_post [-v <subpackage>] [-s <s> -r <r>] <mkinitrd-args>
# More text can follow to go at the end of this variant's %%post.
#
%define kernel_variant_post(s:r:v:) \
%{expand:%%kernel_devel_post %{?-v*}}\
%{expand:%%kernel_variant_posttrans %{?-v*}}\
%{expand:%%post %{?-v*}}\
%{-s:\
if [ `uname -i` == "x86_64" -o `uname -i` == "i386" ] &&\
   [ -f /etc/sysconfig/kernel ]; then\
  /bin/sed -i -e 's/^DEFAULTKERNEL=%{-s*}$/DEFAULTKERNEL=%{-r*}/' /etc/sysconfig/kernel || exit $?\
fi}\
# Added workaround for kernel modules built in a chroot environment\
# where modprobe fails \
if [ ! -d /lib/modules/`uname -r`/kernel ]; then \
  ln -sf /lib/modules/%{KVERREL}%{?-v:-%{-v*}} /lib/modules/`uname -r`\
fi \
/sbin/new-kernel-pkg --package kernel%{?-v:-%{-v*}} --mkinitrd --depmod --install %{*} %{KVERREL}%{?-v:-%{-v*}} || exit $?\
%{nil}

#
# This macro defines a %%preun script for a kernel package.
#	%%kernel_variant_preun <subpackage>
#
%define kernel_variant_preun() \
%{expand:%%preun %{?1}} \
/sbin/new-kernel-pkg --rminitrd --rmmoddep --remove %{KVERREL}%{?1:-%{1}} || exit $? \ 
%{nil}


%kernel_variant_preun u8500
%kernel_variant_post -v u8500

if [ -x /sbin/ldconfig ]
then
    /sbin/ldconfig -X || exit $?
fi

###
### file lists
###



# This is %{image_install_path} on an arch where that includes ELF files,
# or empty otherwise.
%define elf_image_install_path %{?kernel_image_elf:%{image_install_path}}

#
# This macro defines the %%files sections for a kernel package
# and its devel packages.
#	%%kernel_variant_files [-k vmlinux] [-a <extra-files-glob>] [-e <extra-nonbinary>] <condition> <subpackage>
#
%define kernel_variant_files(a:e:k:) \
%if %{1}\
%{expand:%%files %{?2}}\
%defattr(-,root,root)\
/boot/%{?-k:%{-k*}}%{!?-k:vmlinuz}-%{KVERREL}%{?2:-%{2}}\
/boot/uImage-%{KVERREL}%{?2:-%{2}}\
/boot/System.map-%{KVERREL}%{?2:-%{2}}\
#/boot/symvers-%{KVERREL}%{?2:-%{2}}.gz\
/boot/config-%{KVERREL}%{?2:-%{2}}\
%{?-a:%{-a*}}\
%dir /lib/modules/%{KVERREL}%{?2:-%{2}}\
/lib/modules/%{KVERREL}%{?2:-%{2}}/kernel\
/lib/modules/%{KVERREL}%{?2:-%{2}}/build\
/lib/modules/%{KVERREL}%{?2:-%{2}}/source\
%ifnarch %{all_arm}\
/lib/modules/%{KVERREL}%{?2:-%{2}}/vdso\
%endif\
/lib/modules/%{KVERREL}%{?2:-%{2}}/modules.block\
/lib/modules/%{KVERREL}%{?2:-%{2}}/modules.dep.bin\
/lib/modules/%{KVERREL}%{?2:-%{2}}/modules.alias.bin\
/lib/modules/%{KVERREL}%{?2:-%{2}}/modules.symbols.bin\
/lib/modules/%{KVERREL}%{?2:-%{2}}/modules.networking\
/lib/modules/%{KVERREL}%{?2:-%{2}}/modules.order\
/lib/modules/%{KVERREL}%{?2:-%{2}}/modules.builtin* \
%ghost /boot/initrd-%{KVERREL}%{?2:-%{2}}.img\
%{?-e:%{-e*}}\
%{expand:%%files %{?2:%{2}-}devel}\
%defattr(-,root,root)\
%verify(not mtime) /usr/src/kernels/%{KVERREL}%{?2:-%{2}}\
%endif\
%{nil}


%files -n perf
%defattr(-,root,root)
/usr/bin/perf
/usr/libexec/perf-core/


%kernel_variant_files 1 u8500


%changelog
* Mon Jul 5 2010 Olle Tränk <olle.trank@stericsson.com> - 2.6.34
- Added USB Gadget Multi (g_multi.ko) support.
* Fri Jul 2 2010 Robert Rosengren <robert.rosengren@stericsson.com> - 2.6.34
- Solved rpmlint errors and few warnings. 
* Tue Jun 29 2010 Robert Rosengren <robert.rosengren@stericsson.com> - 2.6.34
- Turned STE changes into patches on vanilla kernel instead of one package. 
* Tue Jun 8 2010 Robert Rosengren <robert.rosengren@stericsson.com> - 2.6.34
- Upgraded to STE Kernel version 2.6.34.
* Mon May 17 2010 Robert Rosengren <robert.rosengren@stericsson.com> - 2.6.29
- Initial STE version of 2.6.29 kernel, spec file based on MeeGo. 
