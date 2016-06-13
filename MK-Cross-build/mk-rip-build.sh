#!/bin/bash

# script to install machinekit on a clean linux (debian) installation so
# it's easy to start developing for Machinekit and try it out
# future scripts should add the possibility to add a Xenomai kernel and/or
# the QtQuickVCP development prerequisits
# there are no build prerequisits installed
#
# the script repo list history goes back as follows:
#
# https://github.com/the-snowwhite/scripts
# mksoc-jessie-mk-install.sh

# https://github.com/luminize/scripts
# bareinstall.sh

# https://github.com/mhaberler/scripts
# testbranch.sh
#
#
#for more info see:
# https://github.com/machinekit/machinekit/issues/229
# [Installing (development) Machinekit from pristine Debian image #229](https://github.com/machinekit/machinekit/issues/229#issue-36072102)
#
# #765
# https://github.com/machinekit/machinekit/issues/765
# [Info on Creating Vagrant VMs for Machinekit missing](https://github.com/machinekit/machinekit/issues/765)
# <a href="https://github.com/machinekit/machinekit/issues/765" class="title-link"> Info on Creating Vagrant VMs for Machinekit missing <span class="issue-num">#765</span> </a>
# <a href="https://github.com/machinekit/machinekit/issues/765" class="issue-link js-issue-link" data-id="110147438" data-error-text="Failed to load issue title" data-permission-text="Issue title is private" title="Info on Creating Vagrant VMs for Machinekit missing">#765</a>
#
# [machinekit not ready for Xenomai3, jessie, beagleboard X15 #598] (https://github.com/machinekit/machinekit/issues/598)
# https://github.com/machinekit/machinekit/issues/598#issuecomment-134667637


# ? first download and install czmq from:
# https://packages.debian.org/source/sid/czmq
#
# sudo dpkg -i libczmq3_3.0.2-1_armhf.deb  libczmq-dev_3.0.2-1_armhf.deb
# sudo apt-get install -f

# ? then libwebsockets 1.3

# before running this script, make sure that the user running this script has
# sudo privileges. For instance, if the user is "machinekit" then do:
# $ su
# $ [password]
# visudo
# now add beneath "%root ALL=(ALL:ALL) ALL" the line:
#                 "%machinekit ALL=(ALL:ALL) ALL"
#                       ^---------------------------with correct user name

# ----->   The defaults below are valid for the Debian Wheezy distro
# ----->   please review and edit for other platforms

# the repository and branch settings are for the machinekit master branch.
# please adapt as needed.

## ----- begin configurable options ---
set -x
#set -e
# this is where the test build will go.
SCRATCH=${HOME}/machinekit

MK_SOURCEFILE_NAME=machinekit-src.tar.bz2
MK_BUILTFILE_NAME=machinekit-built-src.tar.bz2

# git repository to pull from
REPO=git://github.com/machinekit/machinekit.git

# the origin to use
ORIGIN=github-machinekit

# the branch to build
BRANCH=master

# the configure command line.

# for the Beaglebone, use:
#CONFIG_ARGS=" --with-xenomai --with-posix --with-platform=beaglebone"

# for the raspberry, use this:
#CONFIG_ARGS=" --with-xenomai --with-posix --with-platform=raspberry"

# for the development install use:
#CONFIG_ARGS=" --with-posix"

# for the cyclone v soc install use:
#CONFIG_ARGS=" --with-rt-preempt --with-posix --with-extra-kernel-sources=/home/mib/Documents/Altera/WS2/test/rocketboards"
#CONFIG_ARGS=" --with-rt-preempt --with-posix --with-platform-socfpga"
CONFIG_ARGS=" --with-rt-preempt --with-platform-socfpga"

# echo commands during execution - very verbose
# comment out once you trust this
set -x

# comment out these lines before running this script:
#echo "please review and edit this script before use."
#exit 1
install_clone_deps(){
# prerequisits for cloning Machinekit
# git
echo machinekit | sudo -S apt-get -y install git dpkg-dev
}

mk_clone() {
# refuse to clone into an existing directory.
if [ -d "$SCRATCH" ]; then
    echo the target directory $SCRATCH already exists.
    echo cleaning repo
    cd $SCRATCH
    git clean -d -f -x
    cd ..
#    echo please remove or rename this directory and run again.
#    exit 1
else
# $SCRATCH does not exist. Make a shallow git clone into it.
# make sure you have around 200MB free space.

    git clone -b "$BRANCH" -o "$ORIGIN" --depth 1 "$REPO" "$SCRATCH"
fi
}

# ----------- end configurable options --------

install_mk_fresh_deps() {
# prerequisits for building from a fresh debian distro
# dependencies + more from https://github.com/mhaberler/asciidoc-sandbox/wiki/Machinekit-Build-for-Multiple-RT-Operating-Systems#installation

sudo apt -y update
sudo apt -y upgrade
#
sudo apt -y install libudev-dev libmodbus-dev libboost-python-dev libusb-1.0-0-dev autoconf pkg-config glib-2.0 gtk+-2.0 tcllib tcl-dev tk-dev bwidget libxaw7-dev libreadline6-dev python-tk libqt4-opengl libqt4-opengl-dev libtk-img python-opengl glade python-xlib python-gtkglext1 python-configobj python-vte libglade2-dev python-glade2 python-gtksourceview2 libncurses-dev libreadline-dev libboost-serialization-dev libboost-thread-dev libjansson-dev lsb-release git dpkg-dev rsyslog automake uuid-runtime ccache  avahi-daemon avahi-discover libnss-mdns bc cython netcat
#
sudo apt -y install python-zmq libjansson-dev python-pyftpdlib libzmq3-dev libprotobuf-dev python-protobuf protobuf-compiler liburiparser-dev libssl-dev  libavahi-client-dev
#

sudo apt -y install libavahi-client3

sudo apt -y install python-zmq libjansson-dev python-pyftpdlib
sudo apt -y install libavahi-common-dev libprotobuf-lite9 libprotobuf9 libprotoc-dev libprotoc9 liburiparser1 psmisc python-pkg-resources python-setuptools python-simplejson uuid-dev


sudo sh -c \
"echo 'Package: *
Pin: release a=stable
Pin-Priority: 900

Package: *
Pin: release o=Debian
Pin-Priority: -10' > \
/etc/apt/preferences.d/stretch;"

sudo sh -c 'cat <<EOT >> /etc/apt/sources.list
deb http://ftp.dk.debian.org/debian stretch  main
deb-src http://ftp.dk.debian.org/debian stretch main
EOT'

sudo apt -y update

sudo apt -y install libavahi-client3 libgcc1 libprotobuf9 libstdc++6 libudev1 libuuid1 libzmq3

sudo apt-key adv --keyserver keyserver.ubuntu.com --recv 43DDF224
sudo sh -c \
  "echo 'deb http://deb.machinekit.io/debian jessie main' > \
  /etc/apt/sources.list.d/machinekit.list"

sudo apt -y update

sudo apt -f install
#sudo apt-get -y autoremove
sudo apt -y update
sudo apt -y upgrade

sudo apt -y install libwebsockets3 libwebsockets-dev

}

replace_mk_source(){
cd ${HOME}
if [ -d machinekit ]; then
    echo "the target directory machinekit already exists ... replacing"
    sudo rm -Rf machinekit
fi
echo "extracting machinekit"
tar -jxf ${MK_SOURCEFILE_NAME}
}

replace_mk_built_source(){
cd ${HOME}
if [ -d machinekit ]; then
    echo "the target directory machinekit already exists ... replacing"
    sudo rm -Rf machinekit
fi
echo "extracting machinekit"
sudo chown machinekit:machinekit ${MK_BUILTFILE_NAME}
tar -jxf ${MK_BUILTFILE_NAME}
}

compress_mk_built(){
cd ${HOME}
if [ -d machinekit ]; then
    echo "the target directory machinekit exists ... post build compressing"
    sudo rm -f ${MK_BUILTFILE_NAME}
    tar -jcf "${MK_BUILTFILE_NAME}" ./machinekit
fi
}


mk_inst_dev_eq() {
# fail the script on any error

sudo apt -y install liburiparser-dev libssl-dev uuid-dev

set -v -x

# make sure some files are in place to finish the build without errors
cd "$SCRATCH/src"
sudo cp ./rtapi/rsyslogd-linuxcnc.conf /etc/rsyslog.d/linuxcnc.conf
sudo touch  /var/log/linuxcnc.log
sudo chmod 644 /var/log/linuxcnc.log
sudo service rsyslog restart
sudo cp ./rtapi/shmdrv/limits.d-machinekit.conf /etc/security/limits.d/linuxcnc.conf
sudo cp ./rtapi/shmdrv/shmdrv.rules /etc/udev/rules.d/50-LINUXCNC-shmdrv.rules

echo installing dependencies
sudo apt -y install --no-install-recommends devscripts equivs


echo "install --no-install-recommends devscripts equivs .. completed"
}

mk_build() {
# fail the script on any error
set -v -x

# setup ccache:
#env CC="ccache gcc" CXX="ccache"

CORES=`nproc`

cd "$SCRATCH"
echo now in directory: `pwd`

debian/configure -pr -t 8.6
sudo sh -c 'echo "y" | mk-build-deps -ir'
#sudo mk-build-deps -ir

echo building in "$SCRATCH/src"
cd "$SCRATCH/src"
echo now in directory: `pwd`

# QA: log what just was checked out

# log the origin
# git remote -v

# show the top commit
# git log -n1

# show that the branch has properly been checked out
# git status

# configure and build
sh autogen.sh

./configure ${CONFIG_ARGS}

make -j $CORES

# Check for sudo
if which sudo >/dev/null 2>&1 ; then
    if sudo -l make setuid >/dev/null 2>&1 ; then
        sudo make setuid
    else
        echo "Cannot run \"sudo make setuid\""
        echo "Please run the following commands as root:"
        echo "  cd $SCRATCH/src"
        echo "  sudo make setuid"
    fi
else
    echo "sudo not found"
    echo "please run the following commands as root:"
    echo "  cd $SCRATCH/src"
    echo "  sudo make setuid"
fi

echo make completed
}

mk_re_build() {
# fail the script on any error
set -v -x

cd "$SCRATCH/src"
# sudo cp ./rtapi/rsyslogd-linuxcnc.conf /etc/rsyslog.d/linuxcnc.conf
# sudo touch  /var/log/linuxcnc.log
# sudo chmod 644 /var/log/linuxcnc.log
# sudo service rsyslog restart
# sudo cp ./rtapi/shmdrv/limits.d-machinekit.conf /etc/security/limits.d/linuxcnc.conf
# sudo cp ./rtapi/shmdrv/shmdrv.rules /etc/udev/rules.d/50-LINUXCNC-shmdrv.rules

# setup ccache:
#env CC="ccache gcc" CXX="ccache"

CORES=`nproc`

# cd "$SCRATCH"
# echo now in directory: `pwd`
#
# debian/configure -pr -t 8.6
# sudo sh -c 'echo "y" | mk-build-deps -ir'
#sudo mk-build-deps -ir

echo building in "$SCRATCH/src"
cd "$SCRATCH/src"
echo now in directory: `pwd`

# QA: log what just was checked out

# log the origin
# git remote -v

# show the top commit
# git log -n1

# show that the branch has properly been checked out
# git status
make clean -j $CORES

# configure and build
sh autogen.sh

./configure ${CONFIG_ARGS}

make -j $CORES
#
# # Check for sudo
# if which sudo >/dev/null 2>&1 ; then
#     if sudo -l make setuid >/dev/null 2>&1 ; then
#         sudo make setuid
#     else
#         echo "Cannot run \"sudo make setuid\""
#         echo "Please run the following commands as root:"
#         echo "  cd $SCRATCH/src"
#         echo "  sudo make setuid"
#     fi
# else
#     echo "sudo not found"
#     echo "please run the following commands as root:"
#     echo "  cd $SCRATCH/src"
#     echo "  sudo make setuid"
# fi
#
echo make completed
}

mk_build_check() {
# check the system configuration (logging etc)

../scripts/check-system-configuration.sh

# no more echo commands during
set +x
echo "looks like the build succeeded!"
echo ""
#echo "to run linuxcnc from this build, please execute first:"
#echo ". $SCRATCH/scripts/rip-environment"
echo " Setting up bashrc for rip-environment"

cmd="grep -o ~/machinekit/scripts/rip-environment /home/machinekit/.bashrc"
resul=eval $cmd
if [ -z "${resul}" ]; then
cat <<EOT >> /home/machinekit/.bashrc

if [ -f ~/machinekit/scripts/rip-environment ]; then
    source ~/machinekit/scripts/rip-environment
    echo "Environment set up for running Machinekit and LinuxCNC"
    echo ""
fi
EOT
else
    echo "Environment allready set up for running Machinekit"
    echo ""
fi

echo ""
echo "bashrc setup OK"
sudo service rsyslog stop
echo ""
echo "Please set your name and email for git with:"
echo "git config --global user.name yourname"
echo "git config --global user.email \"youremail\""
}

#---------------------------------------------------------------------------#
#----------- run functions -------------------------------------------------#
#---------------------------------------------------------------------------#
full_build() {
    install_clone_deps
    sudo mount /dev/shm

    replace_mk_source

    mk_inst_dev_eq

    install_mk_fresh_deps

    mk_build

    mk_build_check

    sudo umount /dev/shm
    compress_mk_built
}

mk_rebuild() {
    sudo mount /dev/shm

    compress_mk_built_source

    replace_mk_built_source

    mk_re_build

    mk_build_check

    sudo umount /dev/shm
    compress_mk_built
}

full_build

#mk_rebuild

echo "'#-------------------------------------------------------------------#"
echo "'#-------------  Mk-Rip Buildscript  Finished -------------------------#"
echo "'#-------------------------------------------------------------------#"
