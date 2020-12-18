#!/bin/bash

sed -i '/package_imagemagick-doc()/,$d' PKGBUILD
sed -i 's/imagemagick()/libmagick-qd8()/g' PKGBUILD
sed -i '/\.\/configure/a \ \ \  --disable-docs \\' PKGBUILD
sed -i '/\.\/configure/a \ \ \  --with-quantum-depth=8 \\' PKGBUILD
sed -i 's/--enable-hdri/--enable-hdri=no/' PKGBUILD
cfg_list="tiff jbig heic raqm fontconfig freetype lcms pango lzma x bzlib zlib zstd ziplib jng"
for config in $cfg_list
do
    sed -i "/--with-xml/a \\ \\ \\  --without-$config \\\\" PKGBUILD
done
cfg_list2="webp rsvg wmf xml openexr lqr openjp2 perl djvu"
for config2 in $cfg_list2
do
    sed -i "s/--with-$config2/--without-$config2/" PKGBUILD
done
sed -i 's/^pkgname=.*/pkgname=libmagick-qd8/' PKGBUILD
sed -i '/perl-options/d' PKGBUILD
sed -i '/make\ check/d' PKGBUILD
sed -i '/backup=/d' PKGBUILD
# n means next line, {} group of commands.
sed -i '/^makedepends/ {s/openexr //;s/libwmf //;s/librsvg //;s/libwebp //;s/libraw //;s/ghostscript //;}' PKGBUILD
sed -i '/^makedepends/ {n;s/ghostpcl //;s/ghostxps //;s/libheif //;s/jbigkit //;}' PKGBUILD
sed -i '/^checkdepends/ s/gsfonts //' PKGBUILD
sed -i '/srcdir\/docpkg\/usr\/share\//a \ \ rm\ -r\ \"$pkgdir\"\/etc' PKGBUILD
sed -i '/srcdir\/docpkg\/usr\/share\//a \ \ rm\ -r\ \"$pkgdir\"\/usr\/include' PKGBUILD
sed -i '/srcdir\/docpkg\/usr\/share\//a \ \ rm\ -r\ \"$pkgdir\"\/usr\/share' PKGBUILD
sed -i '/srcdir\/docpkg\/usr\/share\//a \ \ rm\ -r\ \"$pkgdir\"\/usr\/bin' PKGBUILD
sed -i '/srcdir\/docpkg\/usr\/share\//a \ \ find\ \"$pkgdir\"\/usr\/lib\/pkgconfig\/\ -type\ f\ !\ -name\ \"\*Q8\.pc\"\ -delete' PKGBUILD
sed -i '/pkgdir\/usr\//d' PKGBUILD
sed -i '/policymap/d' PKGBUILD

