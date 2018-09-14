#!/bin/bash
echo "FIXING PKGBUILD!!!!!!!"
sed -i "s/pkgbase=linux-zen/pkgbase=linux-cz/" PKGBUILD
sed -i "/make olddefconfig/a \ \ make xconfig" PKGBUILD
sed -i "/make olddefconfig/a \ \ \.\/fix_config\.sh" PKGBUILD
sed -i "/make olddefconfig/a \ \ make localmodconfig" PKGBUILD
sed -i "/make olddefconfig/a \ \ \.\/prefix_config\.sh" PKGBUILD
sed -i '/make olddefconfig/d' PKGBUILD
sed -i '/prepare() {/a \ \ cp \.\.\/fix_config\.sh "\$\{srcdir\}\/\$\{_srcname\}\/fix_config\.sh"' PKGBUILD
sed -i '/prepare() {/a \ \ cp \.\.\/prefix_config\.sh "\$\{srcdir\}\/\$\{_srcname\}\/prefix_config\.sh"' PKGBUILD
sed -i '/doctrees/d' PKGBUILD
sed -i 's/htmldocs//' PKGBUILD
