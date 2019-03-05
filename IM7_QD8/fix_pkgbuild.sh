#!/bin/bash
echo "FIXING PKGBUILD!!!!!!!"
sed -i 's/pkgbase=imagemagick/pkgbase=imagemagick-qd8/' PKGBUILD
sed -i 's/libmagick\ imagemagick/libmagick-qd8\ imagemagick-qd8/' PKGBUILD
sed -i 's/magick()/magick-qd8()/g' PKGBUILD
sed -i "/\.\/configure/a \\ \\ \\  --with-quantum-depth=8 \\\\" PKGBUILD
sed -i 's/--enable-hdri/--enable-hdri=no/' PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-tiff \\\\" PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-jbig \\\\" PKGBUILD
sed -i "/--with-xml/a \\ \\ \\  --without-heic \\\\" PKGBUILD
sed -i 's/--with-webp/--with-webp=no/' PKGBUILD
sed -i 's/--with-rsvg/--with-rsvg=no/' PKGBUILD
sed -i 's/--with-wmf/--with-wmf=no/' PKGBUILD
sed -i 's/--with-perl/--with-perl=no/' PKGBUILD
sed -i '/--with-perl-options/d' PKGBUILD
sed -i 's/--with-gslib/--with-gslib=no/' PKGBUILD
sed -i 's/--with-xml/--with-xml=no/' PKGBUILD
sed -i 's/--with-openexr/--with-openexr=no/' PKGBUILD
sed -i '/install -Dt/d' PKGBUILD
sed -i '/^makedepends/ {s/openexr//;s/libwmf//;s/librsvg//;s/libwebp//;s/libraw//;}' PKGBUILD
sed -i '/chrpath ocl-icd/ {s/libheif//;s/jbigkit//;}' PKGBUILD
#sed -i 's/\ usr\//\ usr\/local\//g' PKGBUILD
#sed -i 's/pkgdir\"\/usr\/lib/pkgdir\"\/usr\/local\/lib/' PKGBUILD
sed -i '/backup=/d' PKGBUILD
sed -i '/perl5/d' PKGBUILD
sed -i 's/pkgdir\" install/pkgdir\" install-libLTLIBRARIES/'
#sed -i 's/pkgdir\"\/etc.*/pkgdir\"\/etc/' PKGBUILD
#sed -i 's/pkgdir\/usr\/bin/pkgdir\/opt\/bin/' PKGBUILD



