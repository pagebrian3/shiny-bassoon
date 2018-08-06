#!/bin/bash

sed -i "s/pkgbase=linux-zen/pkgbase=linux-cz/" PKGBUILD
sed -i "/make olddefconfig/i \ \ \.\/fix_config\.sh" PKGBUILD
sed -i "/make olddefconfig/i \ \ \.\/prefix_config\.sh" PKGBUILD
sed -i "/make olddefconfig/a \ \ make xconfig" PKGBUILD
sed -i "/make olddefconfig/a \ \ make localmodconfig" PKGBUILD
sed -i '/prepare() {/a \ \ cp \.\.\/fix_config\.sh "\$\{srcdir\}\/\$\{_srcname\}\/fix_config\.sh"' PKGBUILD
sed -i '/prepare() {/a \ \ cp \.\.\/prefix_config\.sh "\$\{srcdir\}\/\$\{_srcname\}\/prefix_config\.sh"' PKGBUILD
