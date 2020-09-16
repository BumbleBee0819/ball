#!/bin/sh
##############################################################################################
# Filename        : run_simple1.sh
# Description     : This is the simplified script for simulating with FleX
# Arguments       : None
# Date            : 08/19/2020
#############################################################################################
#
# This script runs within a singularity container.
# sudo singularity shell --nv kimImage.simg
#

rm -rf /tmp/* 2> /dev/null

. load_config.sh


## -------------------- Adjustable Paras ----------------- ##

#---- Step 1: Check cuda ----#
#     --> [true|false]: false to skip this step
CHECK_CUDA=false


#---- Step 2: Build FleX executables ----#
#     --> [true|false]: false to skip this step
BUILD_EXEC=true


#---- Step 3: Simulate ----#
#    --> [true|false]: false to skip this step 
SIMULATE=true
#    --> [0|1]: 0 to inhibit outputs from flex         
SHOW_FLEX_OUTPUTS=1


## -------------------- Other Paras ---------------------- ##
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


## ---------------------Step 1: Check cuda  -------------- ##

if [ ${CHECK_CUDA} = true ]; then
    cd "cuda_test" \
        && nvcc cuda_test.cu -o cuda_test.out \
        && ./cuda_test.out
    cd $curDir
fi

## ---------------------Step 2: Build FleX executables --- ##

bash buildFleX.sh --build=$BUILD_EXEC


## ---------------------Step 3: Simulate cloths  --------- ##

# sudo singularity shell --nv kimImage.simg

if [ ${SIMULATE} = true ]; then
    for i in "${FLEX_SCENES[@]}"; do
        for j in "${FLEX_BENDSTIFF[@]}"; do
            for k in "${FLEX_SHEAR[@]}"; do
                for h in "${FLEX_STRETCH[@]}"; do
                    
                    python3.7 main_simulate.py \
                    --flex_verbose=$SHOW_FLEX_OUTPUTS \
                    --scenes_str=$i \
                    --bstiff_float=$j \
                    --shstiff=$k \
                    --ststiff=$h \
                    --scale=2.0 \
                    --flexRootPath=${FLEX['flex_root_path']} \
                    --flexConfig=${FLEX['flex_config_file']} \
                    --flex_input_root_path=${FLEX['flex_input_root_path']} \
                    --flex_output_root_path=${FLEX['flex_output_root_path']} \
                    --windstrength=0.4 \
                    --psize=0.02 \
                    --clothNumParticles=105 #> /dev/null

                done
            done
        done
    done
fi