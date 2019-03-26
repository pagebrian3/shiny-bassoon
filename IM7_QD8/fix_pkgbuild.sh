#!/bin/bash

sed -i 's/libmagick\ imagemagick/libmagick-qd8\ imagemagick/' PKGBUILD
sed -i 's/libmagick()/libmagick-qd8()/g' PKGBUILD
sed -i '/\.\/configure/a \ \ \  --with-quantum-depth=8 \\' PKGBUILD
sed -i 's/--enable-hdri/--enable-hdri=no/' PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-tiff \\\\" PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-jbig \\\\" PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-heic \\\\" PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-raqm \\\\" PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-fontconfig \\\\" PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-freetype \\\\" PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-lcms \\\\" PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-pango \\\\" PKGBUILD
sed -i 's/--with-webp/--without-webp/' PKGBUILD
sed -i 's/--with-rsvg/--without-rsvg/' PKGBUILD
sed -i 's/--with-wmf/--without-wmf/' PKGBUILD
sed -i 's/--with-xml/--without-xml/' PKGBUILD
sed -i 's/--with-openexr/--without-openexr/' PKGBUILD
sed -i 's/--with-lqr/--without-lqr/' PKGBUILD
sed -i '/install -Dt/d' PKGBUILD
sed -i '/make\ check/d' PKGBUILD
sed -i '/^makedepends/ {s/openexr//;s/libwmf//;s/librsvg//;s/libwebp//;s/libraw//;}' PKGBUILD
sed -i '/chrpath ocl-icd/ {s/libheif//;s/jbigkit//;}' PKGBUILD
sed -i '/srcdir\/docpkg\/usr\/share\//a \ \ rm\ -r\ \"$pkgdir\"\/etc' PKGBUILD
sed -i '/srcdir\/docpkg\/usr\/share\//a \ \ rm\ -r\ \"$pkgdir\"\/usr\/include' PKGBUILD
sed -i '/srcdir\/docpkg\/usr\/share\//a \ \ rm\ -r\ \"$pkgdir\"\/usr\/share' PKGBUILD
sed -i '/srcdir\/docpkg\/usr\/share\//a \ \ find\ \"$pkgdir\"\/usr\/lib\/pkgconfig\/\ -type\ f\ !\ -name\ \"\*Q8\.pc\"\ -delete' PKGBUILD
sed -i '/srcdir\/docpkg\/usr\/share\//a \ \ sed\ -i\ \x27\/\^Requires:\/\ s\/$\/-7\\\.Q8\/\x27\ $pkgdir\/usr\/lib\/pkgconfig\/\*\.pc' PKGBUILD   

