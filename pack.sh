#!/bin/bash

ARCHIVE=cpicsk_gen.zip
DSTDIR=cpicsk_gen
CONFIG_DIR=kabuki_config_gen

mkdir -p $DSTDIR

cp cpicsk_gen.exe $DSTDIR/
cp template.hex $DSTDIR/
cp $CONFIG_DIR/kabuki_config_gen.exe $DSTDIR/kabuki_config_gen.exe
cp README.txt $DSTDIR/

zip -FS -r $ARCHIVE $DSTDIR
