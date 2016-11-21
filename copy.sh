#!/bin/sh

make
cp diffrelax_{seq,prl} ~/scratch
cp diffrelax.slm ~/scratch
cp -r data/ ~/scratch
cp -r results/ ~/scratch
