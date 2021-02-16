#!/usr/bin/env bash

set -e

function checkclean {
	if [ $(svn status | wc -l) -ne 0 ]; then
		return 1
	fi
	return 0
}

checkclean || ( echo "error: Working copy not clean" ; exit 1 )

./build/svn/run_clang-format.sh

checkclean || ( echo "warning: Formatting does not adhere to enforce clang-format rules." ; svn diff )

svn revert -R .
