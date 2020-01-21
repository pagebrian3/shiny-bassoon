#!/bin/bash

btype="release"
bdir="qvdbmt_rel"
if [ "$1" == "debug" ]; then
    btype="debug"
    bdir="qvdbmt_deb"
fi


if [ ! -d "$bdir" ]; then
    meson setup $bdir --buildtype $btype
fi
cd $bdir
ninja
cd ..
if [ "$btype" == "debug" ]  && [ ! -f "qvbrowser-deb" ] ; then
   ln -s qvdbmt_deb/qvbrowser qvbrowser-deb
elif [ ! -f "qvbrowser" ] ; then
    ln -s qvdbmt_rel/qvbrowser qvbrowser
fi
