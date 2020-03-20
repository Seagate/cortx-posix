#!/bin/bash

Package=libevhtp
Tag=1.2.18
Repo=https://github.com/criticalstack/$Package/archive/$Tag.tar.gz

_download()
{
    local source="$1"
    local target="$2"
    rm -rf "$target"
    wget "$source" -O "$target"
}

install_builddep()
{
    sudo yum-builddep -y "$(dirname $0)/libevhtp.spec"
}

_build_rpms()
{
    local spec_file="$1"
    rpmbuild -ba "$spec_file"
}

rebuild_rpms()
{
    local rpmb_tgz="$HOME/rpmbuild/SOURCES/$Package-$Tag.tar.gz"
    local rpmb_spec="$HOME/rpmbuild/SPECS/libevhtp.spec"
    local rc=0;

    if [ ! -d "$HOME/rpmbuild/SOURCES" ]; then
        mkdir -p "$HOME/rpmbuild/SOURCES"
    fi

    if [ ! -d "$HOME/rpmbuild/SPECS" ]; then
        mkdir -p "$HOME/rpmbuild/SPECS"
    fi


    _download $Repo $rpmb_tgz
    rc=$?
    if ((rc != 0)); then
        echo "Failed to download sources ($rc, '$Repo' => '$rpmb_tgz')."
        return rc
    fi

    cp -a $(dirname $0)/libevhtp.spec $rpmb_spec
    rc=$?
    if ((rc != 0)); then
        echo "Failed to copy the spec file ($rc)."
        return rc
    fi

    _build_rpms $rpmb_spec
    rc=$?
    if ((rc != 0)); then
        echo "Failed to build RPMs ($rc, $rpmb_spec)."
        return rc
    fi
}

install_rpms()
{
    local lib_rpm=""
    local lib_devel_rpm=""
    local lib_debuginfo_rpm=""

    lib_rpm=$(find "$HOME/rpmbuild/RPMS/" -name "$Package-$Tag*.rpm")
    lib_devel_rpm=$(find "$HOME/rpmbuild/RPMS/" -name "$Package-devel-$Tag*.rpm")
    lib_debuginfo_rpm=$(find "$HOME/rpmbuild/RPMS/" -name "$Package-debuginfo-$Tag*.rpm")

    if [ "x$lib_rpm" == "x" ]; then
        echo "Cannot find $Package-$Tag RPM."
        return 1
    fi

    if [ "x$lib_devel_rpm" == "x" ]; then
        echo "Cannot find $Package-devel-$Tag- RPM."
        return 1
    fi

    if [ "x$lib_debuginfo_rpm" == "x" ]; then
        echo "Cannot find $Package-debuginfo-$Tag RPM."
        return 1
    fi

    echo -en "Installing RPMs:\n" "\t$lib_rpm\n" "\t$lib_devel_rpm\n" "\t$lib_debuginfo_rpm\n"

    sudo yum install -y "$lib_rpm" "$lib_devel_rpm" "$lib_debuginfo_rpm"

    echo "Installed."
}

uninstall_rpms()
{
    sudo yum remove -y libevhtp libevhtp-devel libevhtp-debuginfo
}


do_auto()
{
    install_builddep &&
        rebuild_rpms &&
        install_rpms &&
        echo "Done."
}

print_usage()
{
    echo -e "
Build script for libevhtp RPMs.

Usage:
    $0 <action>

Where action is one of the following:
    help      - Print usage.
    builddep  - Install build dependencies.
    rpms      - Build libevhtp RPMs (builddep is required).
    install   - Install built RPMs (rpms is required).
    uninstall - Uninstalls libevhtp RPMs.
    auto      - Automaticaly build and install
               libevhtp and its dependencies.
    "
}

# Main
case $1 in
    builddep)
        install_builddep ;;
    rpms)
        rebuild_rpms ;;
    install)
        install_rpms ;;
    uninstall)
        uninstall_rpms ;;
    auto)
        do_auto ;;
    *)
        print_usage ;;
esac
