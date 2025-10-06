#!/usr/bin/env bash
set -e

#
# Dist script for libopenmpt.
#
# This is meant to be run by the libopenmpt maintainers.
#
# WARNING: The script expects the be run from the root of an OpenMPT svn
#    checkout. It invests no effort in verifying this precondition.
#

# We want ccache
if [ "${MSYSTEM}x" == "x" ]; then
 export PATH="/usr/lib/ccache:$PATH"
#else
# export PATH="/usr/lib/ccache/bin:$PATH"
fi

# Create bin directory
mkdir -p bin

# Check that the API headers are standard compliant

echo "Checking C header ..."
function headercheck_c() {
	HEADERCHECK_NAME="${1}"
	HEADERCHECK_COMPILER="${2}"
	HEADERCHECK_STANDARD="${3}"
	HEADERCHECK_OPTIONS="${4}"
	echo '#include <stddef.h>' > bin/empty.c
	if ${HEADERCHECK_COMPILER} ${HEADERCHECK_STANDARD} -c bin/empty.c -o bin/empty.${HEADERCHECK_NAME}.out > /dev/null 2>&1 ; then
		echo '' > bin/headercheck.c
		echo '#include "libopenmpt/libopenmpt.h"' >> bin/headercheck.c
		echo 'int main(void) { return 0; }' >> bin/headercheck.c
		echo " ${HEADERCHECK_COMPILER} ${HEADERCHECK_STANDARD}"
		${HEADERCHECK_COMPILER} ${HEADERCHECK_STANDARD} ${HEADERCHECK_OPTIONS} -I. bin/headercheck.c -o bin/headercheck.${HEADERCHECK_NAME}.out 2>bin/headercheck.${HEADERCHECK_NAME}.err
		( cat bin/headercheck.${HEADERCHECK_NAME}.err | grep -v 'gnu-ld: warning:' | grep -v 'gnu-ld: NOTE:' >&2 ) || true
		rm bin/headercheck.c
	fi
	rm bin/empty.c
}
headercheck_c "cc"         "cc"    ""                    "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "ccansi"     "cc"    "-ansi"               "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc90iso"    "cc"    "-std=iso9899:1990"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc94iso"    "cc"    "-std=iso9899:199409" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc99iso"    "cc"    "-std=iso9899:1999"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc11iso"    "cc"    "-std=iso9899:2011"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc17iso"    "cc"    "-std=iso9899:2017"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc18iso"    "cc"    "-std=iso9899:2018"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc24iso"    "cc"    "-std=iso9899:2024"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc89"       "cc"    "-std=c89"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc90"       "cc"    "-std=c90"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc94"       "cc"    "-std=c94"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc95"       "cc"    "-std=c95"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc99"       "cc"    "-std=c99"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc11"       "cc"    "-std=c11"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc17"       "cc"    "-std=c17"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc18"       "cc"    "-std=c18"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc23"       "cc"    "-std=c23"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc89gnu"    "cc"    "-std=gnu89"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc90gnu"    "cc"    "-std=gnu90"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc99gnu"    "cc"    "-std=gnu99"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc11gnu"    "cc"    "-std=gnu11"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc17gnu"    "cc"    "-std=gnu17"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc18gnu"    "cc"    "-std=gnu18"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "cc23gnu"    "cc"    "-std=gnu23"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc"        "gcc"   ""                    "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gccansi"    "gcc"   "-ansi"               "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc90iso"   "gcc"   "-std=iso9899:1990"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc94iso"   "gcc"   "-std=iso9899:199409" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc99iso"   "gcc"   "-std=iso9899:1999"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc11iso"   "gcc"   "-std=iso9899:2011"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc17iso"   "gcc"   "-std=iso9899:2017"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc18iso"   "gcc"   "-std=iso9899:2018"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc24iso"   "gcc"   "-std=iso9899:2024"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc89"      "gcc"   "-std=c89"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc90"      "gcc"   "-std=c90"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc94"      "gcc"   "-std=c94"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc95"      "gcc"   "-std=c95"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc99"      "gcc"   "-std=c99"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc11"      "gcc"   "-std=c11"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc17"      "gcc"   "-std=c17"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc18"      "gcc"   "-std=c18"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc23"      "gcc"   "-std=c23"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc89gnu"   "gcc"   "-std=gnu89"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc90gnu"   "gcc"   "-std=gnu90"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc99gnu"   "gcc"   "-std=gnu99"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc11gnu"   "gcc"   "-std=gnu11"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc17gnu"   "gcc"   "-std=gnu17"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc18gnu"   "gcc"   "-std=gnu18"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "gcc23gnu"   "gcc"   "-std=gnu23"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang"      "clang" ""                    "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clangansi"  "clang" "-ansi"               "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang90iso" "clang" "-std=iso9899:1990"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang94iso" "clang" "-std=iso9899:199409" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang99iso" "clang" "-std=iso9899:1999"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang11iso" "clang" "-std=iso9899:2011"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang17iso" "clang" "-std=iso9899:2017"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang18iso" "clang" "-std=iso9899:2018"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang24iso" "clang" "-std=iso9899:2024"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang89"    "clang" "-std=c89"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang90"    "clang" "-std=c90"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang94"    "clang" "-std=c94"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang95"    "clang" "-std=c95"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang99"    "clang" "-std=c99"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang11"    "clang" "-std=c11"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang17"    "clang" "-std=c17"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang18"    "clang" "-std=c18"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang23"    "clang" "-std=c23"            "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang89gnu" "clang" "-std=gnu89"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang90gnu" "clang" "-std=gnu90"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang99gnu" "clang" "-std=gnu99"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang11gnu" "clang" "-std=gnu11"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang17gnu" "clang" "-std=gnu17"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang18gnu" "clang" "-std=gnu18"          "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "clang23gnu" "clang" "-std=gnu23"          "-Wall -Wextra -Wpedantic -Werror"
if [ `uname -s` != "Darwin" ] ; then
if [ `uname -m` == "x86_64" ] ; then
if [ "${MSYSTEM}x" == "x" ]; then
headercheck_c "pcc"        "pcc"   ""                    ""
headercheck_c "pccansi"    "pcc"   "-ansi"               ""
headercheck_c "pcc90iso"   "pcc"   "-std=iso9899:1990"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "pcc94iso"   "pcc"   "-std=iso9899:199409" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "pcc99iso"   "pcc"   "-std=iso9899:1999"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "pcc11iso"   "pcc"   "-std=iso9899:2011"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "pcc17iso"   "pcc"   "-std=iso9899:2017"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "pcc18iso"   "pcc"   "-std=iso9899:2018"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "pcc24iso"   "pcc"   "-std=iso9899:2024"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c "pcc89"      "pcc"   "-std=c89"            ""
headercheck_c "pcc90"      "pcc"   "-std=c90"            ""
headercheck_c "pcc94"      "pcc"   "-std=c94"            ""
headercheck_c "pcc95"      "pcc"   "-std=c95"            ""
headercheck_c "pcc99"      "pcc"   "-std=c99"            ""
headercheck_c "pcc11"      "pcc"   "-std=c11"            ""
headercheck_c "pcc17"      "pcc"   "-std=c17"            ""
headercheck_c "pcc18"      "pcc"   "-std=c18"            ""
headercheck_c "pcc23"      "pcc"   "-std=c23"            ""
headercheck_c "pcc89gnu"   "pcc"   "-std=gnu89"          ""
headercheck_c "pcc90gnu"   "pcc"   "-std=gnu90"          ""
headercheck_c "pcc99gnu"   "pcc"   "-std=gnu99"          ""
headercheck_c "pcc11gnu"   "pcc"   "-std=gnu11"          ""
headercheck_c "pcc17gnu"   "pcc"   "-std=gnu17"          ""
headercheck_c "pcc18gnu"   "pcc"   "-std=gnu18"          ""
headercheck_c "pcc23gnu"   "pcc"   "-std=gnu23"          ""
headercheck_c "tcc"        "tcc"   ""                    "-Wall -Wunusupported -Wwrite-strings -Werror"
fi
fi
fi
rm -f bin/headercheck.*.out
rm -f bin/headercheck.*.err

echo "Checking C header in C++ mode ..."
function headercheck_c_cpp() {
	HEADERCHECK_NAME="${1}"
	HEADERCHECK_COMPILER="${2}"
	HEADERCHECK_STANDARD="${3}"
	HEADERCHECK_OPTIONS="${4}"
	echo '#include <vector>' > bin/empty.cpp
	if ${HEADERCHECK_COMPILER} ${HEADERCHECK_STANDARD} -c bin/empty.cpp -o bin/empty.${HEADERCHECK_NAME}.out > /dev/null 2>&1 ; then
		echo '' > bin/headercheck.c
		echo '#include "libopenmpt/libopenmpt.h"' >> bin/headercheck.cpp
		echo 'int main() { return 0; }' >> bin/headercheck.cpp
		echo " ${HEADERCHECK_COMPILER} ${HEADERCHECK_STANDARD}"
		${HEADERCHECK_COMPILER} ${HEADERCHECK_STANDARD} ${HEADERCHECK_OPTIONS} -I. bin/headercheck.cpp -o bin/headercheck.${HEADERCHECK_NAME}.out 2>bin/headercheck.${HEADERCHECK_NAME}.err
		rm bin/headercheck.cpp
	fi
	rm bin/empty.cpp
}
headercheck_c_cpp "c++"          "c++"       ""             "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++98"        "c++"       "-std=c++98"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++03"        "c++"       "-std=c++03"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++11"        "c++"       "-std=c++11"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++14"        "c++"       "-std=c++14"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++17"        "c++"       "-std=c++17"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++20"        "c++"       "-std=c++20"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++23"        "c++"       "-std=c++23"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++98gnu"     "c++"       "-std=gnu++98" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++03gnu"     "c++"       "-std=gnu++03" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++11gnu"     "c++"       "-std=gnu++11" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++14gnu"     "c++"       "-std=gnu++14" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++17gnu"     "c++"       "-std=gnu++17" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++30gnu"     "c++"       "-std=gnu++20" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "c++23gnu"     "c++"       "-std=gnu++23" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++"          "g++"       ""             "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++98"        "g++"       "-std=c++98"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++03"        "g++"       "-std=c++03"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++11"        "g++"       "-std=c++11"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++14"        "g++"       "-std=c++14"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++17"        "g++"       "-std=c++17"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++20"        "g++"       "-std=c++20"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++23"        "g++"       "-std=c++23"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++98gnu"     "g++"       "-std=gnu++98" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++03gnu"     "g++"       "-std=gnu++03" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++11gnu"     "g++"       "-std=gnu++11" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++14gnu"     "g++"       "-std=gnu++14" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++17gnu"     "g++"       "-std=gnu++17" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++20gnu"     "g++"       "-std=gnu++20" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "g++23gnu"     "g++"       "-std=gnu++23" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++"      "clang++"   ""             "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++98"    "clang++"   "-std=c++98"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++03"    "clang++"   "-std=c++03"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++11"    "clang++"   "-std=c++11"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++14"    "clang++"   "-std=c++14"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++17"    "clang++"   "-std=c++17"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++20"    "clang++"   "-std=c++20"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++23"    "clang++"   "-std=c++23"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++98gnu" "clang++"   "-std=gnu++98" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++03gnu" "clang++"   "-std=gnu++03" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++11gnu" "clang++"   "-std=gnu++11" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++14gnu" "clang++"   "-std=gnu++14" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++17gnu" "clang++"   "-std=gnu++17" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++20gnu" "clang++"   "-std=gnu++20" "-Wall -Wextra -Wpedantic -Werror"
headercheck_c_cpp "clang++23gnu" "clang++"   "-std=gnu++23" "-Wall -Wextra -Wpedantic -Werror"
rm -f bin/headercheck.*.out
rm -f bin/headercheck.*.err

echo "Checking C++ header ..."
function headercheck_cpp() {
	HEADERCHECK_NAME="${1}"
	HEADERCHECK_COMPILER="${2}"
	HEADERCHECK_STANDARD="${3}"
	HEADERCHECK_OPTIONS="${4}"
	echo '#include <array>' > bin/empty.cpp
	if ${HEADERCHECK_COMPILER} ${HEADERCHECK_STANDARD} -c bin/empty.cpp -o bin/empty.${HEADERCHECK_NAME}.out > /dev/null 2>&1 ; then
		echo '' > bin/headercheck.c
		echo '#include "libopenmpt/libopenmpt.hpp"' >> bin/headercheck.cpp
		echo 'int main() { return 0; }' >> bin/headercheck.cpp
		echo " ${HEADERCHECK_COMPILER} ${HEADERCHECK_STANDARD}"
		${HEADERCHECK_COMPILER} ${HEADERCHECK_STANDARD} ${HEADERCHECK_OPTIONS} -I. bin/headercheck.cpp -o bin/headercheck.${HEADERCHECK_NAME}.out 2>bin/headercheck.${HEADERCHECK_NAME}.err
		rm bin/headercheck.cpp
	fi
	rm bin/empty.cpp
}
#headercheck_cpp "c++"          "c++"       ""             "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "c++98"        "c++"       "-std=c++98"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "c++03"        "c++"       "-std=c++03"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "c++11"        "c++"       "-std=c++11"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "c++14"        "c++"       "-std=c++14"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "c++17"        "c++"       "-std=c++17"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "c++20"        "c++"       "-std=c++20"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "c++23"        "c++"       "-std=c++23"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "c++98gnu"     "c++"       "-std=gnu++98" "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "c++03gnu"     "c++"       "-std=gnu++03" "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "c++11gnu"     "c++"       "-std=gnu++11" "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "c++14gnu"     "c++"       "-std=gnu++14" "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "c++17gnu"     "c++"       "-std=gnu++17" "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "c++30gnu"     "c++"       "-std=gnu++20" "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "c++23gnu"     "c++"       "-std=gnu++23" "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "g++"          "g++"       ""             "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "g++98"        "g++"       "-std=c++98"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "g++03"        "g++"       "-std=c++03"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "g++11"        "g++"       "-std=c++11"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "g++14"        "g++"       "-std=c++14"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "g++17"        "g++"       "-std=c++17"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "g++20"        "g++"       "-std=c++20"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "g++23"        "g++"       "-std=c++23"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "g++98gnu"     "g++"       "-std=gnu++98" "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "g++03gnu"     "g++"       "-std=gnu++03" "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "g++11gnu"     "g++"       "-std=gnu++11" "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "g++14gnu"     "g++"       "-std=gnu++14" "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "g++17gnu"     "g++"       "-std=gnu++17" "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "g++20gnu"     "g++"       "-std=gnu++20" "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "g++23gnu"     "g++"       "-std=gnu++23" "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "clang++"      "clang++"   ""             "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "clang++98"    "clang++"   "-std=c++98"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "clang++03"    "clang++"   "-std=c++03"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "clang++11"    "clang++"   "-std=c++11"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "clang++14"    "clang++"   "-std=c++14"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "clang++17"    "clang++"   "-std=c++17"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "clang++20"    "clang++"   "-std=c++20"   "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "clang++23"    "clang++"   "-std=c++23"   "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "clang++98gnu" "clang++"   "-std=gnu++98" "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "clang++03gnu" "clang++"   "-std=gnu++03" "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "clang++11gnu" "clang++"   "-std=gnu++11" "-Wall -Wextra -Wpedantic -Werror"
#headercheck_cpp "clang++14gnu" "clang++"   "-std=gnu++14" "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "clang++17gnu" "clang++"   "-std=gnu++17" "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "clang++20gnu" "clang++"   "-std=gnu++20" "-Wall -Wextra -Wpedantic -Werror"
headercheck_cpp "clang++23gnu" "clang++"   "-std=gnu++23" "-Wall -Wextra -Wpedantic -Werror"
rm -f bin/headercheck.*.out
rm -f bin/headercheck.*.err

echo "Checking version helper ..."
c++ -Wall -Wextra -I. -Isrc -Icommon build/auto/helper_get_openmpt_version.cpp -o bin/helper_get_openmpt_version
rm bin/helper_get_openmpt_version

# Clean dist
make NO_SDL=1 NO_SDL2=1 clean-dist

# Check the build
make NO_SDL=1 NO_SDL2=1 clean
make NO_SDL=1 NO_SDL2=1
make NO_SDL=1 NO_SDL2=1 check
make NO_SDL=1 NO_SDL2=1 clean

# Build Unix-like tarball, Windows zipfile and docs tarball
if `svn info . > /dev/null 2>&1` ; then
make NO_SDL=1 NO_SDL2=1 SILENT_DOCS=1 dist
fi

# Clean
make NO_SDL=1 NO_SDL2=1 clean

# Build autoconfiscated tarball
./build/autotools/autoconfiscate.sh

# Test autotools tarball
./build/autotools/test_tarball.sh


