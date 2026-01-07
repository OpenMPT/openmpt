#!/usr/bin/env bash

# stop on error
set -e

# normalize current directory to project root
cd build 2>&1 > /dev/null || true
cd ..

function download () {
 set -e
 MPT_GET_FILE_NAME="$1"
 MPT_GET_FILE_SIZE="$2"
 MPT_GET_FILE_CHECKSUM="$3"
 shift
 shift
 shift
 echo "Checking '$MPT_GET_FILE_NAME' ..."
 if [ -f "$MPT_GET_FILE_NAME" ]; then
  FILE_SIZE=$(find "$MPT_GET_FILE_NAME" -printf '%s')
  if [ ! "x$FILE_SIZE" = "x$MPT_GET_FILE_SIZE" ]; then
   echo "$FILE_SIZE does not match expected file size $MPT_GET_FILE_SIZE. Redownloading."
   rm -f "$MPT_GET_FILE_NAME"
  fi
 fi
 if [ -f "$MPT_GET_FILE_NAME" ]; then
  FILE_CHECKSUM=$(sha512sum "$MPT_GET_FILE_NAME" | awk '{print $1;}')
  if [ ! "x$FILE_CHECKSUM" = "x$MPT_GET_FILE_CHECKSUM" ]; then
   echo "$FILE_CHECKSUM does not match expected file checksum $MPT_GET_FILE_CHECKSUM. Redownloading."
   rm -f "$MPT_GET_FILE_NAME"
  fi
 fi
 while (( "$#" )); do
  URL="$(echo ""$1"" | sed 's/ /%20/g')"
  if [ ! -f "$MPT_GET_FILE_NAME" ]; then
   echo "Downloading '$MPT_GET_FILE_NAME' from '$URL' ..."
   if [ -t 1 ] ; then
    if command -v wcurl &> /dev/null ; then
     wcurl --curl-options="--progress-bar" "$URL" --output "$MPT_GET_FILE_NAME" || true
    elif command -v curl &> /dev/null ; then
     curl --progress-bar --location -o "$MPT_GET_FILE_NAME" "$URL" || true
    elif command -v wget &> /dev/null ; then
     wget --progress=bar:force -O "$MPT_GET_FILE_NAME" "$URL" || true
    fi
   else
    if command -v wcurl &> /dev/null ; then
     wcurl --curl-options="--no-progress-meter" "$URL" --output "$MPT_GET_FILE_NAME" || true
    elif command -v curl &> /dev/null ; then
     curl --silent --show-error --location -o "$MPT_GET_FILE_NAME" "$URL" || true
    elif command -v wget &> /dev/null ; then
     wget --no-verbose -O "$MPT_GET_FILE_NAME" "$URL" || true
    fi
   fi
   echo "Verifying '$URL' ..."
   if [ -f "$MPT_GET_FILE_NAME" ]; then
    FILE_SIZE=$(find "$MPT_GET_FILE_NAME" -printf '%s')
    if [ ! "x$FILE_SIZE" = "x$MPT_GET_FILE_SIZE" ]; then
     echo "$FILE_SIZE does not match expected file size $MPT_GET_FILE_SIZE."
     rm -f "$MPT_GET_FILE_NAME"
    fi
   fi
   if [ -f "$MPT_GET_FILE_NAME" ]; then
    FILE_CHECKSUM=$(sha512sum "$MPT_GET_FILE_NAME" | awk '{print $1;}')
    if [ ! "x$FILE_CHECKSUM" = "x$MPT_GET_FILE_CHECKSUM" ]; then
     echo "$FILE_CHECKSUM does not match expected file checksum $MPT_GET_FILE_CHECKSUM."  
     rm -f "$MPT_GET_FILE_NAME"
    fi
   fi
  fi
  shift
 done
 if [ ! -f "$MPT_GET_FILE_NAME" ]; then
  echo "Failed to download '$MPT_GET_FILE_NAME'."
  return 1
 fi
 return 0
}

MPT_XML_BACKEND=sed
if command -v xmllint &> /dev/null ; then
 MPT_XML_BACKEND=xmllint
elif command -v xpath &> /dev/null ; then
 MPT_XML_BACKEND=xpath
fi

xml_xpath () {
 set -e
 MPT_XPATH_FILENAME="${1}"
 shift
 MPT_XPATH_QUERY="${1}"
 shift
 if [ "${MPT_XML_BACKEND}" == "xmllint" ] ; then
  < "${MPT_XPATH_FILENAME}" sed 's/ xmlns="urn:ietf:params:xml:ns:metalink"//g' | xmllint --xpath "${MPT_XPATH_QUERY}" -
 elif [ "${MPT_XML_BACKEND}" == "xpath" ] ; then
  xpath -q -e "${MPT_XPATH_QUERY}" "${MPT_XPATH_FILENAME}"
 fi
}

parse_metalink4 () {
 set -e
 MPT_METAFILE="$1"
 shift
 if [ "${MPT_XML_BACKEND}" == "xmllint" ] || [ "${MPT_XML_BACKEND}" == "xpath" ] ; then
  for filenum in $(seq 1 $(xml_xpath "${MPT_METAFILE}" "count(/metalink/file)")); do
   MPT_METAFILE_FILENAME=$(xml_xpath "${MPT_METAFILE}" "string(/metalink/file[${filenum}]/@name)")
   echo -n "${MPT_METAFILE_FILENAME}"
   echo -n " "
   MPT_METAFILE_FILESIZE=$(xml_xpath "${MPT_METAFILE}" "string(/metalink/file[${filenum}]/size/text())")
   echo -n "${MPT_METAFILE_FILESIZE}"
   echo -n " "
   MPT_METAFILE_FILEHASH=$(xml_xpath "${MPT_METAFILE}" "string(/metalink/file[${filenum}]/hash/text())")
   echo -n "${MPT_METAFILE_FILEHASH}"
   for urlnum in $(seq 1 $(xml_xpath "${MPT_METAFILE}" "count(/metalink/file[${filenum}]/url)")); do
    MPT_METAFILE_FILEURL=$(xml_xpath "${MPT_METAFILE}" "string(/metalink/file[${filenum}]/url[${urlnum}]/text())")
    echo -n " "
    echo -n "\"${MPT_METAFILE_FILEURL}\""
   done
   echo ""
  done
 else
  cat "${MPT_METAFILE}" \
   | sed 's/\r//g' | tr '\n' '\t' | sed 's/\t//g' | sed 's/> *</></g' \
   | sed 's/<file/\n<file/g' \
   | sed 's/\/file>/\/file>\n/g' \
   | grep '\<file' \
   | sed 's/<file name="//g' | sed 's/"><size>/ /g' | sed 's/<\/size><hash type="sha-512">/ /g' | sed 's/<\/hash>//g' | sed 's/<\/file>//g' \
   | sed 's/<url>/ "/g' | sed 's/<\/url>/"/g' \
   | cat
 fi
}

function unpack () {
 set -e
 MPT_GET_DESTDIR="$1"
 MPT_GET_FILE="$2"
 MPT_GET_SUBDIR="$3"
 echo "Extracting '$MPT_GET_DESTDIR' from '$MPT_GET_FILE:$MPT_GET_SUBDIR' ..."
 EXTENSION="${MPT_GET_FILE##*.}"
 if [ -d "$MPT_GET_DESTDIR" ]; then
  rm -rf "$MPT_GET_DESTDIR"
 fi
 mkdir "$MPT_GET_DESTDIR"
 case "$EXTENSION" in
  tar)
   tar -xvaf "$MPT_GET_FILE" -C "$MPT_GET_DESTDIR"
   ;;
  zip)
   unzip -d "$MPT_GET_DESTDIR" "$MPT_GET_FILE"
   ;;
  7z)
   7z x -o"$MPT_GET_DESTDIR" "$MPT_GET_FILE"
   ;;
  exe)
   7z x -o"$MPT_GET_DESTDIR" "$MPT_GET_FILE"
   ;;
 esac
 if [ ! "$MPT_GET_SUBDIR" = "." ]; then
  mv "$MPT_GET_DESTDIR" "$MPT_GET_DESTDIR.tmp"
  mv "$MPT_GET_DESTDIR.tmp/$MPT_GET_SUBDIR" "$MPT_GET_DESTDIR"
 fi
 return 0
}

if [ ! -d "build/externals" ]; then
 mkdir build/externals
fi
if [ ! -d "build/tools" ]; then
 mkdir build/tools
fi

# download
parse_metalink4 build/download_externals.meta4 | (
 while IFS=$'\n' read -r URL; do
  eval download $URL
 done
)

unpack "include/allegro42" "build/externals/allegro-4.2.3.1-hg.8+r8500.zip" "."
unpack "include/cwsdpmi"   "build/externals/csdpmi7b.zip"                   "."
unpack "include/winamp"    "build/externals/WA5.55_SDK.exe"                 "."
unpack "include/xmplay"    "build/externals/xmp-sdk.zip"                    "."

ln -s OUT.H include/winamp/Winamp/out.h

mkdir -p build/tools/svn_apply_autoprops
cp "build/externals/svn_apply_autoprops.py" "build/tools/svn_apply_autoprops/"
chmod u+x "build/tools/svn_apply_autoprops/svn_apply_autoprops.py"
