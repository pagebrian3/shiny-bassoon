#!/bin/bash

rm -rf imagemagick/PKGBUILD
rm -rf imagemagick/src
rm -rf imagemagick/pkg
asp update
asp -f export extra/imagemagick
cp fix_pkgbuild.sh imagemagick/
cd imagemagick
./fix_pkgbuild.sh
makepkg -sf --skipinteg
