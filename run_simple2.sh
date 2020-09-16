#!/bin/sh
##############################################################################################
# Filename      : run_simple2.sh
# Description   : This is the simplified script for render and making videos
# Arguments     : None
# Date          : 08/19/2020
#############################################################################################
# This script can be either run within a singularity container or on your own machine.
# 
# 1. To run within a singularity container, you need:
#    1) Download the container at:   
#    2) Shell into the container:    singularity shell blender_sing.simg
#    3) Run this script:             bash run_simple2.sh
#
# 2. To run on your own machine, you need:
#    1) Download blender 2.79 package and put it in "experiments/obj2mov". 
#       An easy way of doing this is to edit "setenv.sh" by setting "DOWNLOAD_BLENDER=true".
#       Then run the script by "bash setenv.sh".
#    2) Install ffmpeg:   pacman -S ffmpeg/apt-get install ffmpeg
#    3) Run the script:   bash run_simple2.sh
#
############################################################################################

rm -rf /tmp/* 2> /dev/null

. load_config.sh


# Download Blender and set path


## ------------------------- Adjustable Paras ------------------------ ##

#    --> [true|false]: false to skip this step
RENDER=true
#    --> [0|1]: 1 to inhibit stdout from blender; 0 to print Blender stdout in realtime
HIDE_BLENDER_OUTPUTS=1
#    --> [true|false]: false to skip this step    
MAKE_VIDEO=true


## --------------------------- Other Paras ----------------------------##
curDir=$PWD
curFile="${curDir}/$0"

# Simulate
FLEX_SCENES=(${FLEXSCENES['scenes']})                
FLEX_SCENES=(${FLEX_SCENES//,/ })
FLEX_BENDSTIFF=(${FLEXSCENES['bs']})
FLEX_BENDSTIFF=(${FLEX_BENDSTIFF//,/ })
FLEX_SHEAR=(${FLEXSCENES['sh']})                
FLEX_SHEAR=(${FLEX_SHEAR//,/ })
FLEX_STRETCH=(${FLEXSCENES['st']})                
FLEX_STRETCH=(${FLEX_STRETCH//,/ })



# Render
BLENDER_USE_CONTAINER=(${BLENDER['blender_use_singularity_container']})
BLENDER_VERSION=(${BLENDER['blender_version']})
BLENDER_ROOT_DIR=(${BLENDER['blender_root_path']})
BLENDER_INPUT_DIR=(${BLENDER['blender_input_path']})
BLENDER_OUTPUT_IMG_DIR=(${BLENDER['blender_output_img_path']})
BLENDER_VIDEO_DIR=(${BLENDER['video_path']})


## --------------------------- Render ---------------------------------##

#sudo singularity shell blender_sing.simg

if [ ${RENDER} = true ]; then
    for i in "${FLEX_SCENES[@]}"; do
        for j in "${FLEX_BENDSTIFF[@]}"; do
            for k in "${FLEX_SHEAR[@]}"; do
                for h in "${FLEX_STRETCH[@]}"; do
                    echo -n "[$count/$total_sim_num]: scene=$i, bs=$j, shear=$k, stretch=$h..."
    
                    python3.7 main_render.py \
                        --blenderBin=/opt/blender-2.79a-linux64/blender \
                        --blenderFile=${BLENDER_ROOT_DIR}$i.blend \
                        --outputError=${HIDE_BLENDER_OUTPUTS} \
                        --erasePrevious=0 \
                        --useConfig=0 \
                        --inputRootPath=${BLENDER_INPUT_DIR} \
                        --folderName=${i}_bs_${j}_sh_${k}_st_${h} \
                        --objBaseName=${i}_cloth_ \
                        --outputImgRootPath=${BLENDER_OUTPUT_IMG_DIR} \
                        --addMaterial=1

                done
            done
        done
    done
fi

## ----------------------------Make movie  --------------------------- ##

if [ ${MAKE_VIDEO} = true ]; then
    for i in "${FLEX_SCENES[@]}"; do
        for j in "${FLEX_BENDSTIFF[@]}"; do
            for k in "${FLEX_SHEAR[@]}"; do
                for h in "${FLEX_STRETCH[@]}"; do
                    ffmpeg -framerate 30 \
                           -start_number 0 \
                           -y -i ${BLENDER_OUTPUT_IMG_DIR}${i}_bs_${j}_sh_${k}_st_${h}/${i}_cloth_%d.png \
                           -vcodec mpeg4 ${BLENDER_VIDEO_DIR}${i}_bs_${j}_sh_${k}_st_${h}.mov                
                done
            done
        done
    done
fi