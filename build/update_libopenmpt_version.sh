#!/usr/bin/env bash

set -e

MODE="${1}"

function checkclean {
	if [ $(svn status | wc -l) -ne 0 ]; then
		echo "error: Working copy not clean"
		exit 1
	fi
}

DATE=$(date --iso)

source libopenmpt/libopenmpt_version.mk
LIBOPENMPT_VERSION_PRERELVER=${LIBOPENMPT_VERSION_PREREL/-pre./}

function setprerel {
	cat libopenmpt/libopenmpt_version.h | sed -e 's/#define OPENMPT_API_VERSION_IS_PREREL .*/#define OPENMPT_API_VERSION_IS_PREREL 1/' > libopenmpt/libopenmpt_version.h.tmp && mv libopenmpt/libopenmpt_version.h.tmp libopenmpt/libopenmpt_version.h
}

function addchangelog {
	echo > libopenmpt/dox/changelog.md.tmp
	cat libopenmpt/dox/changelog.md | head -n 7 >> libopenmpt/dox/changelog.md.tmp
	echo "### libopenmpt ${LIBOPENMPT_VERSION_MAJOR}.${LIBOPENMPT_VERSION_MINOR}.${LIBOPENMPT_VERSION_PATCH}-pre" >> libopenmpt/dox/changelog.md.tmp
	echo "" >> libopenmpt/dox/changelog.md.tmp
	cat libopenmpt/dox/changelog.md | tail -n +8 >> libopenmpt/dox/changelog.md.tmp
	mv libopenmpt/dox/changelog.md.tmp libopenmpt/dox/changelog.md
}

function writeall {

	cat libopenmpt/libopenmpt_version.h | sed -e s/#define\ OPENMPT_API_VERSION_MAJOR\ .\*/#define\ OPENMPT_API_VERSION_MAJOR\ $LIBOPENMPT_VERSION_MAJOR/ > libopenmpt/libopenmpt_version.h.tmp && mv libopenmpt/libopenmpt_version.h.tmp libopenmpt/libopenmpt_version.h
	cat libopenmpt/libopenmpt_version.h | sed -e s/#define\ OPENMPT_API_VERSION_MINOR\ .\*/#define\ OPENMPT_API_VERSION_MINOR\ $LIBOPENMPT_VERSION_MINOR/ > libopenmpt/libopenmpt_version.h.tmp && mv libopenmpt/libopenmpt_version.h.tmp libopenmpt/libopenmpt_version.h
	cat libopenmpt/libopenmpt_version.h | sed -e s/#define\ OPENMPT_API_VERSION_PATCH\ .\*/#define\ OPENMPT_API_VERSION_PATCH\ $LIBOPENMPT_VERSION_PATCH/ > libopenmpt/libopenmpt_version.h.tmp && mv libopenmpt/libopenmpt_version.h.tmp libopenmpt/libopenmpt_version.h
	cat libopenmpt/libopenmpt_version.h | sed -e s/#define\ OPENMPT_API_VERSION_PREREL\ .\*/#define\ OPENMPT_API_VERSION_PREREL\ \"$LIBOPENMPT_VERSION_PREREL\"/ > libopenmpt/libopenmpt_version.h.tmp && mv libopenmpt/libopenmpt_version.h.tmp libopenmpt/libopenmpt_version.h

	cat libopenmpt/libopenmpt_version.mk | sed -e s/LIBOPENMPT_VERSION_MAJOR\=.\*/LIBOPENMPT_VERSION_MAJOR\=$LIBOPENMPT_VERSION_MAJOR/ > libopenmpt/libopenmpt_version.mk.tmp && mv libopenmpt/libopenmpt_version.mk.tmp libopenmpt/libopenmpt_version.mk
	cat libopenmpt/libopenmpt_version.mk | sed -e s/LIBOPENMPT_VERSION_MINOR\=.\*/LIBOPENMPT_VERSION_MINOR\=$LIBOPENMPT_VERSION_MINOR/ > libopenmpt/libopenmpt_version.mk.tmp && mv libopenmpt/libopenmpt_version.mk.tmp libopenmpt/libopenmpt_version.mk
	cat libopenmpt/libopenmpt_version.mk | sed -e s/LIBOPENMPT_VERSION_PATCH\=.\*/LIBOPENMPT_VERSION_PATCH\=$LIBOPENMPT_VERSION_PATCH/ > libopenmpt/libopenmpt_version.mk.tmp && mv libopenmpt/libopenmpt_version.mk.tmp libopenmpt/libopenmpt_version.mk
	cat libopenmpt/libopenmpt_version.mk | sed -e s/LIBOPENMPT_VERSION_PREREL\=.\*/LIBOPENMPT_VERSION_PREREL\=$LIBOPENMPT_VERSION_PREREL/ > libopenmpt/libopenmpt_version.mk.tmp && mv libopenmpt/libopenmpt_version.mk.tmp libopenmpt/libopenmpt_version.mk

	cat libopenmpt/libopenmpt_version.mk | sed -e s/LIBOPENMPT_LTVER_CURRENT\=.\*/LIBOPENMPT_LTVER_CURRENT\=$LIBOPENMPT_LTVER_CURRENT/ > libopenmpt/libopenmpt_version.mk.tmp && mv libopenmpt/libopenmpt_version.mk.tmp libopenmpt/libopenmpt_version.mk
	cat libopenmpt/libopenmpt_version.mk | sed -e s/LIBOPENMPT_LTVER_REVISION\=.\*/LIBOPENMPT_LTVER_REVISION\=$LIBOPENMPT_LTVER_REVISION/ > libopenmpt/libopenmpt_version.mk.tmp && mv libopenmpt/libopenmpt_version.mk.tmp libopenmpt/libopenmpt_version.mk
	cat libopenmpt/libopenmpt_version.mk | sed -e s/LIBOPENMPT_LTVER_AGE\=.\*/LIBOPENMPT_LTVER_AGE\=$LIBOPENMPT_LTVER_AGE/ > libopenmpt/libopenmpt_version.mk.tmp && mv libopenmpt/libopenmpt_version.mk.tmp libopenmpt/libopenmpt_version.mk

}

case $MODE in

	release)
		checkclean
		cat libopenmpt/libopenmpt_version.h | sed -e 's/#define OPENMPT_API_VERSION_PREREL.*/#define OPENMPT_API_VERSION_PREREL ""/' > libopenmpt/libopenmpt_version.h.tmp && mv libopenmpt/libopenmpt_version.h.tmp libopenmpt/libopenmpt_version.h
		cat libopenmpt/libopenmpt_version.h | sed -e 's/#define OPENMPT_API_VERSION_IS_PREREL.*/#define OPENMPT_API_VERSION_IS_PREREL 0/' > libopenmpt/libopenmpt_version.h.tmp && mv libopenmpt/libopenmpt_version.h.tmp libopenmpt/libopenmpt_version.h
		cat libopenmpt/libopenmpt_version.mk | sed -e 's/LIBOPENMPT_VERSION_PREREL=.*/LIBOPENMPT_VERSION_PREREL=/' > libopenmpt/libopenmpt_version.mk.tmp && mv libopenmpt/libopenmpt_version.mk.tmp libopenmpt/libopenmpt_version.mk
		echo > cat libopenmpt/dox/changelog.md.tmp
		cat libopenmpt/dox/changelog.md | head -n 7 >> libopenmpt/dox/changelog.md.tmp
		cat libopenmpt/dox/changelog.md | head -n 8 | tail -n 1 | sed -e s/-pre/\ \(${DATE}\)/ >> libopenmpt/dox/changelog.md.tmp
		cat libopenmpt/dox/changelog.md | tail -n +9 | sed -e 's/-pre//' >> libopenmpt/dox/changelog.md.tmp
		mv libopenmpt/dox/changelog.md.tmp libopenmpt/dox/changelog.md
		;;

	bumpmajor)
		checkclean
		LIBOPENMPT_VERSION_MAJOR=$(($LIBOPENMPT_VERSION_MAJOR + 1))
		LIBOPENMPT_VERSION_MINOR=0
		LIBOPENMPT_VERSION_PATCH=0
		LIBOPENMPT_VERSION_PRERELVER=0
		LIBOPENMPT_VERSION_PREREL=-pre.0
		writeall
		setprerel
		addchangelog
		;;
	bumpminor)
		checkclean
		LIBOPENMPT_VERSION_MINOR=$(($LIBOPENMPT_VERSION_MINOR + 1))
		LIBOPENMPT_VERSION_PATCH=0
		LIBOPENMPT_VERSION_PRERELVER=0
		LIBOPENMPT_VERSION_PREREL=-pre.0
		writeall
		setprerel
		addchangelog
		;;
	bumppatch)
		checkclean
		LIBOPENMPT_VERSION_PATCH=$(($LIBOPENMPT_VERSION_PATCH + 1))
		LIBOPENMPT_VERSION_PRERELVER=0
		LIBOPENMPT_VERSION_PREREL=-pre.0
		writeall
		setprerel
		addchangelog
		;;
	bumpprerel)
		LIBOPENMPT_VERSION_PRERELVER=$(($LIBOPENMPT_VERSION_PRERELVER + 1))
		LIBOPENMPT_VERSION_PREREL=-pre.$LIBOPENMPT_VERSION_PRERELVER
		writeall
		setprerel
		;;

	breakltabi)
		checkclean
		LIBOPENMPT_LTVER_CURRENT=$(($LIBOPENMPT_LTVER_CURRENT + 1))
		LIBOPENMPT_LTVER_REVISION=0
		LIBOPENMPT_LTVER_AGE=0
		writeall
		;;
	bumpltabi)
		LIBOPENMPT_LTVER_CURRENT=$(($LIBOPENMPT_LTVER_CURRENT + 1))
		LIBOPENMPT_LTVER_REVISION=0
		LIBOPENMPT_LTVER_AGE=$(($LIBOPENMPT_LTVER_AGE + 1))
		writeall
		;;
	bumpltrev)
		LIBOPENMPT_LTVER_REVISION=$(($LIBOPENMPT_LTVER_REVISION + 1))
		writeall
		;;

	*)
		echo "error: Wrong argument"
		exit 1
		;;

esac

