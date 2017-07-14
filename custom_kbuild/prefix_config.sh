#!/bin/bash
APPEND_LIST="GPIO_BT8XX=n SND_SE6X=n"
for i in $APPEND_LIST
do
    sed -i "\$aCONFIG_${i}" .config
done
