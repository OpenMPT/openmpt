#!/bin/bash
rm -rf html/*.*
mkdir html
./wiki.py
tar -cvzf html.tgz html