#!/bin/bash
echo "FIXING PKGBUILD!!!!!!!"
sed -i "s/pkgbase=imagemagick/pkgbase=im7-qd8/" PKGBUILD
sed -i "s/--prefix=\/usr/--prefix=\/opt/" PKGBUILD
sed -i "/\.\/configure/a \\ \\ \\  --with-quantum-depth=8 \\\\" PKGBUILD
sed -i 's/--enable-hdri/--enable-hdri=no/' PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-tiff \\\\" PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-jbig \\\\" PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-heic \\\\" PKGBUILD
sed -i 's/--with-webp/--with-webp=no/' PKGBUILD
sed -i 's/--with-rsvg/--with-rsvg=no/' PKGBUILD
sed -i 's/--with-wmf/--with-wmf=no/' PKGBUILD
sed -i 's/--with-openexr/--with-openexr=no/' PKGBUILD
sed -i '/^makedepends/ {s/openexr//;s/libwmf//;s/librsvg//;s/libwebp//;s/libraw//;}' PKGBUILD
sed -i '/chrpath ocl-icd/ {s/libheif//;s/jbigkit//;}' PKGBUILD
sed -i 's/[[:space:]]usr\//\ opt\//g' PKGBUILD
sed -i 's/pkgdir\"\/usr\/lib/pkgdir\"\/opt\/lib/' PKGBUILD
sed -i 's/pkgdir\/usr\/bin/pkgdir\/opt\/bin/' PKGBUILD
sed -i 's/pkgdir\/usr\/share\/doc/pkgdir\/opt\/share\/doc/' PKGBUILD
sed -i 's/pkgdir\/usr\/lib\/perl5\"\ -name/pkgdir\/opt\/lib\/perl5\"\ -name/' PKGBUILD


