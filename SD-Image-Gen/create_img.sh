#!/bin/bash

# Create, partition and mount SD-Card image:
#------------------------------------------------------------------------------------------------------
# Variables
#------------------------------------------------------------------------------------------------------
CURRENT_DIR=`pwd`
WORK_DIR=$1
IMG_FILE=$2

#SD_IMG=${WORK_DIR}/mksoc_sdcard.img
ROOTFS_IMG=${WORK_DIR}/rootfs.img
DRIVE=/dev/mapper/loop2
F_DRIVE=`eval /sbin/losetup -f`
ROOTFS_TYPE=ext4
ROOTFS_LABEL=rootfs

ext4_options="-O ^metadata_csum,^64bit"
mkfs_options="${ext4_options}"

mkfs="mkfs.${ROOTFS_TYPE}"
media_prefix=${DRIVE}
media_rootfs_partition=p2

mkfs_partition="${media_prefix}${media_rootfs_partition}"
mkfs_label="-L ${ROOTFS_LABEL}"


fdisk_2part() {
sudo fdisk ${F_DRIVE} << EOF
n
p
1

+1M
t
a2
n
p
2


w
EOF
}

fdisk_3part() {
sudo fdisk ${F_DRIVE} << EOF
n
p
1

+1M
t
a2
n
p
2

+96M
n
p
3


t
2
b
w
EOF
}

create_sdcard_img() {
#--------------- Initial sd-card image - partitioned --------------
echo "#-------------------------------------------------------------------------------#"
echo "#-----------------------------          ----------------------------------------#"
echo "#---------------     +++ generating sd-card image  zzz  +++ ........  ----------#"
echo "#---------------------------  Please  wait   -----------------------------------#"
echo "#-------------------------------------------------------------------------------#"
sudo dd if=/dev/zero of=${IMG_FILE}  bs=1024 count=3700K

#sudo losetup --show -f $SD_IMG
sudo kpartx -a -v -s ${IMG_FILE}

fdisk_2part
#fdisk_3part

#sudo partprobe $DRIVE
sudo kpartx -u -s -v ${IMG_FILE}

echo "creating file systems"

#sudo mkfs -j -v -L "rootfs" ${DRIVE}p2
#sudo sh -c "LC_ALL=C ${mkfs} ${mkfs_partition} ${mkfs_label}"
sudo sh -c "LC_ALL=C ${mkfs} ${mkfs_options} ${mkfs_partition} ${mkfs_label}"

#sudo mkfs.vfat -F 32 -n "BOOT" ${DRIVE}p2
#sudo mkfs -j -v -L "rootfs" ${DRIVE}p3

sync
sudo kpartx -d -s -v ${IMG_FILE}

}

create_rootfs_img() {
echo "#-------------------------------------------------------------------------------#"
echo "#-----------------------------          ----------------------------------------#"
echo "#----------------     +++ generating rootfs image  zzz  +++ ........  ----------#"
echo "#-----------------------------   wait   ----------------------------------------#"
echo "#-------------------------------------------------------------------------------#"
sudo dd if=/dev/zero of=${ROOTFS_IMG}  bs=1024 count=3600K
sudo mke2fs -j -L "rootfs" ${ROOTFS_IMG}
}

if [ ! -z "${WORK_DIR}" ]; then
    if [ -f ${SD_IMG} ]; then
        echo "Deleting old imagefile"
        rm -f ${SD_IMG}
    fi
    if [ -f ${ROOTFS_IMG} ]; then
        echo "Deleting old imagefile"
        rm -f ${ROOTFS_IMG}
    fi
#create_rootfs_img
create_sdcard_img

echo "#---------------------------------------------------------------------------------- "
echo "#-------------  create_img.sh Finished with Success  ------------------------------ "
echo "#---------------------------------------------------------------------------------- "

else
    echo "#---------------------------------------------------------------------------------- "
    echo "#----------------  create_img.sh Ended Unsuccessfully    -------------------------- "
    echo "#-------------  workdir parameter missing     ------------------------------------- "
    echo "#---------------------------------------------------------------------------------- "
fi


