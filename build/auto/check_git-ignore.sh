#!/usr/bin/env bash

set -e

function checkclean {
	if [ $(svn status | wc -l) -ne 0 ]; then
		return 1
	fi
	return 0
}

checkclean || ( echo "error: Working copy not clean" ; exit 1 )

./build/git/generate_gitignore.sh

checkclean || ( echo "warning: .gitignore does not match svn:ignore properties." ; svn diff )

svn revert -R .
