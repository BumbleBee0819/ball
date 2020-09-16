#!/bin/bash

############################################################################
# Script Name                : buildFleX.sh
# Description                : build FleX executables for each scene.
# Env                        : kimImage.simg [sudo singularity shell --nv kimImage.simg]
# Args                       : --build=true|false
# Date                       : 30/07/2020
# Email:                     : wenyan.bi@yale.edu
############################################################################

. load_config.sh > /dev/null


# Usage:
# --build=[true|false]: Set true to build FleX executable from source
BUILD_FLEX=true

## ----------------------- Paras -----------------------------##
curDir=$PWD
curFile="${curDir}/$0"

FLEX_ROOT=(${FLEX['flex_root_path']})
FLEX_SCENES=(${FLEXSCENES['scenes']})   # the full list of the flex scenes, seperated by ,
FLEX_SCENES=(${FLEX_SCENES//,/ })



## --------------------- Check args --------------------------##
for i in "$@"
do
    case $i in
        --build=*)
        if [ "${i#*=}" != true ]; then
            if [ "${i#*=}" != false ]; then
                echo_red "[ERROR: line ${LINENO}] $curFile \n    ====> Invalid value for --build=[true|false]"1>&2
                exit 1
            fi
        fi
        BUILD_FLEX="${i#*=}"
        shift
        ;;
        *)   # unknown options
        echo_red "[ERROR: line ${LINENO}] $curFile \n    ====> Unknown options! \n          Usage: --build=[true|false]"1>&2
        exit 1
        ;;
    esac
done



## ---------- Build executables for each scene -------------##
if [ ${BUILD_FLEX} = true ]; then
    
    # If the path does not end with '/', add '/'
    if [[ ! ${FLEX_ROOT} =~ '/'$ ]]; then
        FLEX_ROOT="${FLEX_ROOT}/"
    fi

    cd "./${FLEX_ROOT}src/compiler/makelinux64"

    catch_error=false
    # Build FleX exeutables for each FLEX_SCENES
    for i in "${FLEX_SCENES[@]}"; do
        make -f Makefile.$i clean && make -f Makefile.$i -j 4
        if [ "$?" = "0" ]; then
            echo_blue "Successfully built FleX executable for [$i]"
        else
            if [ $catch_error == false ]; then
                echo_red "[Error]: $curFile" 1>&2
            fi
            echo_red "    ====> Failed to build FleX executables for [$i]!" 1>&2
            catch_error=true
            #exit 1
        fi
    done

    cd $curDir

    if [ ${catch_error} = true ]; then
        exit 1
    fi
fi
