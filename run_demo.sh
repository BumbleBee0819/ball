#!/bin/bash

mkdir -p experiments/renderings/trialObjs/wind

#cd FleX/bin/linux64
./FleX/bin/linux64/NvFlexDemoReleaseCUDA_x64_wind -obj=./dataset/trialObjs/wind/model1.obj -config=./FleXConfigs/flexConfig.yml -rx=0 -ry=0 -rz=0 -ccolor_id=7 -nofloor -offline -use_euler -export=experiments/renderings/trialObjs/wind// -psize=0.007800 -clothsize=210 -randomSeed=-1 -randomClothMinRes=145 -randomClothMaxRes=215 -saveClothPerSimStep=1