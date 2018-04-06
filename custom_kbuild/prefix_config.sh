#!/bin/bash
APPEND_LIST="GPIO_BT8XX=n SND_SE6X=n X86_P6_NOP=y"
for i in $APPEND_LIST
do
    sed -i "\$aCONFIG_${i}" .config
done

COMMENT_LIST="GENERIC_CPU"

for i in $COMMENT_LIST
do
    sed -i "/^CONFIG_${i}=/c\# CONFIG_${i} is not set" .config
done

YES_LIST="MNATIVE"

for i in $YES_LIST
do
    sed -i "/# CONFIG_${i} is not set/c\CONFIG_${i}=y" .config
done

