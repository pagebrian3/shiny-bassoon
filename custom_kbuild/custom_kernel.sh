#!/bin/bash

rm -rf linux-zen/PKGBUILD
rm -rf linux-zen/src
rm -rf linux-zen/pkg
asp update
asp -f export extra/linux-zen
cp fix_config.sh linux-zen/
cp prefix_config.sh linux-zen/
cp fix_pkgbuild.sh linux-zen/
cd linux-zen
./fix_pkgbuild.sh
makepkg -sf --skippgpcheck
