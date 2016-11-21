#!/bin/sh

make
cp diffrelax_{seq,prl} ~/scratch
cp -r jobfiles ~/scratch
cp -r data/ ~/scratch
mkdir ~/scratch/balena_results
