#!/usr/bin/env bash

set -e

MODE="${1}"

function checkclean {
	if [ $(svn status | wc -l) -ne 0 ]; then
		echo "error: Working copy not clean"
		exit 1
	fi
}

VER_MAJOR=$(cat common/versionNumber.h | grep "VER_MAJORMAJOR " | awk '{print $3;}')
VER_MINOR=$(cat common/versionNumber.h | grep "VER_MAJOR " | awk '{print $3;}')
VER_PATCH=$(cat common/versionNumber.h | grep "VER_MINOR " | awk '{print $3;}')
VER_BUILD=$(cat common/versionNumber.h | grep "VER_MINORMINOR " | awk '{print $3;}')

function writeall {

	cat common/versionNumber.h | sed -e s/#define\ VER_MAJORMAJOR\ \ .\*/#define\ VER_MAJORMAJOR\ \ $VER_MAJOR/ > common/versionNumber.h.tmp && mv common/versionNumber.h.tmp common/versionNumber.h
	cat common/versionNumber.h | sed -e s/#define\ VER_MAJOR\ \ \ \ \ \ .\*/#define\ VER_MAJOR\ \ \ \ \ \ $VER_MINOR/ > common/versionNumber.h.tmp && mv common/versionNumber.h.tmp common/versionNumber.h
	cat common/versionNumber.h | sed -e s/#define\ VER_MINOR\ \ \ \ \ \ .\*/#define\ VER_MINOR\ \ \ \ \ \ $VER_PATCH/ > common/versionNumber.h.tmp && mv common/versionNumber.h.tmp common/versionNumber.h
	cat common/versionNumber.h | sed -e s/#define\ VER_MINORMINOR\ .\*/#define\ VER_MINORMINOR\ $VER_BUILD/ > common/versionNumber.h.tmp && mv common/versionNumber.h.tmp common/versionNumber.h

}

case $MODE in

	bumpmajor)
		checkclean
		VER_MAJOR=$(($VER_MAJOR + 1))
		VER_MINOR=00
		VER_PATCH=00
		VER_BUILD=01
		writeall
		;;
	bumpminor)
		checkclean
		VER_MINOR=$(printf "%02d" $(($(echo $VER_MINOR | sed 's/^0*//') + 1)))
		VER_PATCH=00
		VER_BUILD=01
		writeall
		;;
	bumppatch)
		checkclean
                VER_PATCH=$(printf "%02d" $(($(echo $VER_PATCH | sed 's/^0*//') + 1)))
                VER_BUILD=00
		writeall
		;;
	bumpbuild)
		VER_BUILD=$(printf "%02d" $(($(echo $VER_BUILD | sed 's/^0*//') + 1)))
		writeall
		;;

	*)
		echo "error: Wrong argument"
		exit 1
		;;

esac

