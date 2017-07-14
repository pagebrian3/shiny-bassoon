#!/bin/bash

sed -i "s/pkgbase=linux-zen/pkgbase=linux-cz/" PKGBUILD
sed -i "/make prepare/a \ \ \.\/fix_config\.sh" PKGBUILD
sed -i "/make prepare/a \ \ make localmodconfig" PKGBUILD
sed -i "/make prepare/a \ \ \.\/prefix_config\.sh" PKGBUILD
sed -i "s/#make xconfig/make xconfig/" PKGBUILD
sed -i '/prepare() {/a \ \ cp \.\.\/fix_config\.sh "\$\{srcdir\}\/\$\{_srcname\}\/fix_config\.sh"' PKGBUILD
sed -i '/prepare() {/a \ \ cp \.\.\/prefix_config\.sh "\$\{srcdir\}\/\$\{_srcname\}\/prefix_config\.sh"' PKGBUILD
