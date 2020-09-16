#!/bin/bash

############################################################################
# Script Name                : setenv.sh
# Description                : download the singularity container for FleX; 
#                              create required folders; and download Blender.
# Args                       : --download_flex=true|false
#                              --download_blender=true|false
#                              --download_blender_container=true|false
# Date                       : 30/07/2020
############################################################################

. load_config.sh

## ------------------------- Adjustable Paras ------------------------ ##

# ---> [true|false]:  Set true to download the singularity container
DOWNLOAD_CONTAINER=false
# ---> [true|false]:  Set true to download blender bin
DOWNLOAD_BLENDER=false
# ---> [true|false]:  Set true to download blender singularity container
DOWNLOAD_BLENDER_CONTAINER=true

## --------------------------- Other Paras ----------------------------##
curDir=$PWD
curFile="${curDir}/$0"

# singularity container name
CONTAINER=(${ENV['cont']})
CONTAINER_URL=(${ENV['cont_link']})

# blender
BLENDER_URL=(${BLENDER['blender_link']})
BLENDER_ROOT_PATH=(${BLENDER['blender_root_path']})

# blender container
BLENDER_CONTAINER=(${BLENDER['blender_cont']})
BLENDER_CONTAINER_URL=(${BLENDER['blender_cont_link']})

# paths
BLENDER_OUT_IMG_PATH=(${BLENDER['blender_output_img_path']})
VID_PATH=(${BLENDER['video_path']})

## ---------------------------- Check args --------------------------- ##
for i in "$@"
do
    case $i in
        --download_flex=*)       # --download_flex: @param {type:"boolean"}
        if [ "${i#*=}" != true ]; then
            if [ "${i#*=}" != false ]; then
                echo_red "[ERROR] $curFile \n    ====> Invalid value for --download_flex=[true|false]"
                exit 1
            fi
        fi
        DOWNLOAD_CONTAINER="${i#*=}"
        shift
        ;;
        --download_blender=*)     # --download_blender: @param {type:"boolean"}
        if [ "${i#*=}" != true ]; then
            if [ "${i#*=}" != false ]; then
                echo_red "[ERROR] $curFile \n    ====> Invalid value for --download_blender=[true|false]"
                exit 1
            fi
        fi
        DOWNLOAD_BLENDER="${i#*=}"
        shift
        ;;
        --download_blender_container=*)   # --download_blender_container: @param {type:"boolean"}
        if [ "${i#*=}" != true ]; then
            if [ "${i#*=}" != false ]; then
                echo_red "[ERROR] $curFile \n    ====> Invalid value for --download_blender_container=[true|false]"
                exit 1
            fi
        fi
        DOWNLOAD_BLENDER_CONTAINER="${i#*=}"
        shift
        ;;
        *)   # unknown options
        echo_red "[ERROR] $curFile \n    ====> Unknown options!"
        exit 1
        ;;
    esac
done


## ------------------ Download Singularity container ----------------- ##
if [ ${DOWNLOAD_CONTAINER} = true ]; then
    echo_blue "Downloading the container... "
    wget "$CONTAINER_URL" -O ${CONTAINER}
fi
# sudo singularity shell --nv $CONTAINER

## ------------------ Download blender ------------------------------- ##
if [ ${DOWNLOAD_BLENDER} = true ]; then
    echo_blue "Downloading Blender files... "
    wget "$BLENDER_URL" -O blender_tmp
    unzip blender_tmp -d ${BLENDER_ROOT_PATH}
    rm blender_tmp
fi

## ------------------ Download Blender container --------------------- ##
if [ ${DOWNLOAD_BLENDER_CONTAINER} = true ]; then
    echo_blue "Downloading the blender container..."
    wget "$BLENDER_CONTAINER_URL" -O $BLENDER_CONTAINER
fi
# sudo singularity shell $BLENDER_CONTAINER

## ------------------ Create folders --------------------------------- ##
if [ ! -d ${BLENDER_OUT_IMG_PATH} ]; then
    mkdir ${BLENDER_OUT_IMG_PATH}
fi

if [ ! -d ${VID_PATH} ]; then
    mkdir ${VID_PATH}
fi
