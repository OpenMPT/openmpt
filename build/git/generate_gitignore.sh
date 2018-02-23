#!/usr/bin/env bash

set -e

echo "# This file was generated automatically by running build/git/generate_gitignore.sh inside a svn working copy." > .gitignore

(
	svn pl --recursive --xml | xpath -q -e '/properties/target[property/@name = "svn:ignore"]' | grep '^<target' | sed 's/<target path=//g' | sed 's/>$//g' | sed 's/"//g'
) | sort | while IFS=$'\n' read -r WCDIR ; do
	if [ "x$WCDIR" = "x." ] ; then
		PREFIX="/"
	else
		PREFIX="/${WCDIR}/"
	fi
	echo "checking ${WCDIR} ..."
	(
		svn pg svn:ignore "${WCDIR}"
	) | sort | while IFS=$'\n' read -r PATTERN ; do
		if [ "x$PATTERN" != "x" ] ; then
			echo " setting ${WCDIR}: ${PREFIX}${PATTERN}"
			echo "${PREFIX}${PATTERN}" >> .gitignore
		fi
	done
done

