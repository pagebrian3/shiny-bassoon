#!/bin/bash
echo "PRE-FIXING CONFIG!!!!!!!"

APPEND_LIST="GPIO_BT8XX=n SND_SE6X=n X86_P6_NOP=y TRIM_UNUSED_KSYMS=y SCHED_MUQSS=y SMT_NICE=y"
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

sed -i "/CONFIG_SCHED_MC_PRIO/a \ \ CONFIG_SHARERQ=2" .config
sed -i "/CONFIG_SCHED_MC_PRIO/a \ \ CONFIG_RQ_MC=y" .config

