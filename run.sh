#!/bin/sh
##############################################################################################
# Filename      : run.sh
# Description   : This is the main script for simulating with Flex and renering with Blender.
# Arguments     : None
# Date          : 07/30/2020
#############################################################################################
# 
# This script runs within the singularity container.
#
rm -rf /tmp/* 2> /dev/null

. load_config.sh

## ------------------------- Configuration Variables ------------------------------------- ##
totalStep=4


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


#---- Step 4: Render and make video ----#
#    --> [true|false]: false to skip this step
RENDER=true
#    --> [0|1]: 1 to inhibit stdout from blender; 0 to print Blender stdout in realtime
HIDE_BLENDER_OUTPUTS=1
#    --> [true|false]: false to skip this step    
MAKE_VIDEO=true


(
## ------------------------- Other Variables --------------------------------------------- ##
curDir=$PWD
curFile="${curDir}/$0"

# Simulate
FLEX_SCENES=(${FLEXSCENES['scenes']})
FLEX_SCENES=(${FLEX_SCENES//,/ })
FLEX_MASS=(${FLEXSCENES['mass']})
FLEX_MASS=(${FLEX_MASS//,/ })
FLEX_BENDSTIFF=(${FLEXSCENES['bs']})
FLEX_BENDSTIFF=(${FLEX_BENDSTIFF//,/ })
FLEX_SHEAR=(${FLEXSCENES['sh']})
FLEX_SHEAR=(${FLEX_SHEAR//,/ })
FLEX_STRETCH=(${FLEXSCENES['st']})
FLEX_STRETCH=(${FLEX_STRETCH//,/ })

total_sim_num=$((${#FLEX_SCENES[@]}*${#FLEX_BENDSTIFF[@]}*${#FLEX_SHEAR[@]}*${#FLEX_STRETCH[@]}*${#FLEX_MASS[@]}))

if [ ${total_sim_num} = 0 ]; then
    echo_red "Exit with Error: total_sim_num is 0. Check [default.conf]. Make sure (scenes, bs, sh, st) are not empty ... "
    exit 1
fi


# Render
BLENDER_USE_CONTAINER=(${BLENDER['blender_use_singularity_container']})
BLENDER_VERSION=(${BLENDER['blender_version']})
BLENDER_ROOT_DIR=(${BLENDER['blender_root_path']})
BLENDER_INPUT_DIR=(${BLENDER['blender_input_path']})
BLENDER_OUTPUT_IMG_DIR=(${BLENDER['blender_output_img_path']})
BLENDER_VIDEO_DIR=(${BLENDER['video_path']})
BLENDER_SAMPLE_RATE=(${BLENDER['sample_rate']})

echo -e "----------------------------------------"
echo -e "Scenes: ${FLEX_SCENES[@]}"
echo -e "Mass: ${FLEX_MASS[@]}"
echo -e "Bending: ${FLEX_BENDSTIFF[@]}"
echo -e "Shear: ${FLEX_SHEAR[@]}"
echo -e "Stretch: ${FLEX_STRETCH[@]}"
echo -e "Total num of stimuli: $total_sim_num"
echo -e "----------------------------------------"

## --------------------------Step 1: Check cuda  ----------------------------------------- ##

echo_blue "Step 1/$totalStep:  Check Cuda ... "

if [ ${CHECK_CUDA} = true ]; then
    if [[ ! -e "cuda_test/cuda_test.out" ]]; then
        cd "cuda_test" \
            && nvcc cuda_test.cu -o cuda_test.out \
            && ./cuda_test.out
    else
        cd "cuda_test" && ./cuda_test.out
    fi

    if [ "$?" = "0" ]; then
        echo_blue "     ====> Success"
    else
        # exit when error occurred
        echo_red "     ====> Exit with error: cuda failed. Check [error.log]"
        cd $curDir
        exit 1
    fi

    cd $curDir
fi

## --------------------------Step 2: Build FleX executables  ----------------------------- ##

echo_blue "Step 2/$totalStep:  Run buildFlex.sh ... "

#singularity exec --nv ${ENV['cont']} bash buildFleX.sh --build=$BUILD_EXEC> /dev/null
singularity exec --nv ${ENV['cont']} bash buildFleX.sh --build=${BUILD_EXEC}

#
if [ "$?" = "0" ]; then
    if [ ${BUILD_EXEC} = true ]; then
        echo_blue "     ====> Success"
    else
        echo_blue "     ====> Skip" 
    fi
else
    # exit when error occurred
    echo_red "     ====> Exit with error. Check [error.log]"
    exit 1
fi


## ------------------------- Step 3: Simulate cloths  ------------------------------------ ##
#
# When error occurred, this step does not exit but log the error in [error.log]
#
echo_blue "Step 3/$totalStep:  Simulate cloth dynamics ... "

count=0
catch_error=false

if [ ${SIMULATE} == true ]; then
    # overwrite all previous outputs
    #[[ -e ${FLEX['flex_output_root_path']} ]] && rm -rf ${FLEX['flex_output_root_path']}
    simT=0

    for tmp_scene in "${FLEX_SCENES[@]}"; do
        for tmp_bs in "${FLEX_BENDSTIFF[@]}"; do
            # for tmp_sh in "${FLEX_SHEAR[@]}"; do
            #     for tmp_st in "${FLEX_STRETCH[@]}"; do
                    for tmp_mass in "${FLEX_MASS[@]}"; do
                         tmp_sh=$tmp_bs
                         tmp_st=$tmp_bs
                        count=$(($count+1))
                        echo -n "[$count/$total_sim_num]: scene=${tmp_scene}, mass=${tmp_mass}, bs=${tmp_bs}, shear=${tmp_sh}, stretch=${tmp_st}..."
                        
                        startT=$SECONDS
                        singularity exec --nv ${ENV['cont']} python3.7 main_simulate.py \
                            --flex_verbose=${SHOW_FLEX_OUTPUTS} \
                            --scenes_str=${tmp_scene} \
                            --bstiff_float=${tmp_bs} \
                            --mass=${tmp_mass} \
                            --shstiff=${tmp_sh} \
                            --ststiff=${tmp_st} \
                            --scale=2.0 \
                            --flexRootPath=${FLEX['flex_root_path']} \
                            --flexConfig=${FLEX['flex_config_file']} \
                            --flex_input_root_path=${FLEX['flex_input_root_path']} \
                            --flex_output_root_path=${FLEX['flex_output_root_path']} \
                            --psize=0.02 \
                            --clothNumParticles=105 
                            #--windstrength=0.0#> /dev/null
                        cursimT=$(($SECONDS-$startT))
                        echo -n "Simulate Time: $cursimT"
                        simT=$(($cursimT+$simT))


                        # error handling
                        if [[ ! "$?" -eq 0 ]]; then    
                            echo_red_cross

                            if [ ${catch_error} = false ]; then
                                echo_red "[ERROR: line ${LINENO}] $curFile" 1>&2
                            fi
                            echo_red "    ====> Failed simulate at (scene=${tmp_scene}, mass=${tmp_mass}, bs=${tmp_bs}, shear=${tmp_sh}, stretch=${tmp_st})!" 1>&2
                            catch_error=true
                            #exit 1
                        else
                            echo_green_check
                        fi
                    done
            #     done
            # done
        done
    done
    echo -n "Mean Simulate Time: $(($simT/$count))"
fi


# Does not exit when error occurred
if [ ${catch_error} = false ]; then
    if [ ${SIMULATE} = true ]; then
        echo_blue "     ====> Success"
    else
        echo_blue "     ====> Skip" 
    fi
else
    echo_red "     ====> Check error.log"
fi


## --------------------------Step 4: Render and make videos  ----------------------------- ##

echo_blue "Step 4/$totalStep:  Render and make videos ... "

count=0
catch_error=false


if [ ${RENDER} = true ]; then
    for tmp_scene in "${FLEX_SCENES[@]}"; do
        for tmp_bs in "${FLEX_BENDSTIFF[@]}"; do
            # for tmp_sh in "${FLEX_SHEAR[@]}"; do
            #     for tmp_st in "${FLEX_STRETCH[@]}"; do
                    for tmp_mass in "${FLEX_MASS[@]}"; do
                        tmp_sh=$tmp_bs
                        tmp_st=$tmp_bs
                        count=$(($count+1))
                        echo -n "[$count/$total_sim_num]: scene=${tmp_scene}, mass=${tmp_mass}, bs=${tmp_bs}, shear=${tmp_sh}, stretch=${tmp_st}..."
                        
                        startT=$SECONDS

                        if [ ${BLENDER_USE_CONTAINER} = true ]; then
                            singularity exec ${BLENDER['blender_cont']} python3.7 main_render.py \
                                --blenderBin=/opt/blender-2.79a-linux64/blender \
                                --blenderFile=${BLENDER_ROOT_DIR}${tmp_scene}.blend \
                                --outputError=${HIDE_BLENDER_OUTPUTS} \
                                --erasePrevious=0 \
                                --useConfig=0 \
                                --inputRootPath=${BLENDER_INPUT_DIR} \
                                --folderName=${tmp_scene}_mass_${tmp_mass}_bs_${tmp_bs}_sh_${tmp_sh}_st_${tmp_sh} \
                                --objBaseName=${tmp_scene}_cloth_ \
                                --outputImgRootPath=${BLENDER_OUTPUT_IMG_DIR} \
                                --addMaterial=1 \
                                --sampleRate=10
                        else
                            python main_render.py \
                                --blenderBin=${BLENDER_ROOT_DIR}${BLENDER_VERSION}/blender \
                                --blenderFile=${BLENDER_ROOT_DIR}${tmp_scene}.blend \
                                --outputError=${HIDE_BLENDER_OUTPUTS} \
                                --erasePrevious=1 \
                                --useConfig=0 \
                                --inputRootPath=${BLENDER_INPUT_DIR} \
                                --folderName=${tmp_scene}_mass_${tmp_mass}_bs_${tmp_bs}_sh_${tmp_sh}_st_${tmp_sh} \
                                --objBaseName=${tmp_scene}_cloth_ \
                                --outputImgRootPath=${BLENDER_OUTPUT_IMG_DIR} \
                                --addMaterial=1 \
                                --sampleRate=20
                        fi

                        echo -n "Render Time: $(($SECONDS-$startT))"

                        ## error handling
                        if [[ ! "$?" -eq 0 ]]; then    
                            echo_red_cross

                            if [ $catch_error == false ]; then
                                echo_red "[ERROR: line ${LINENO}] $curFile"
                            fi
                            echo_red "    ====> Failed render at (scene=${tmp_scene}, mass=${tmp_mass}, bs=${tmp_bs}, shear=${tmp_sh}, stretch=${tmp_st})!"
                            catch_error=true
                            #exit 1
                        else
                            echo_green_check
                        fi

                        ## make movies
                        if [ ${MAKE_VIDEO} = true ]; then
                            echo "making movie..."
                            (
                                singularity exec ${BLENDER['blender_cont']} ffmpeg \
                                -framerate 30 \
                                -start_number 0 \
                                -y -i ${BLENDER_OUTPUT_IMG_DIR}${tmp_scene}_mass_${tmp_mass}_bs_${tmp_bs}_sh_${tmp_sh}_st_${tmp_sh}/${tmp_scene}_cloth_%d.png \
                                -vcodec mpeg4 ${BLENDER_VIDEO_DIR}${tmp_scene}_mass_${tmp_mass}_bs_${tmp_bs}_sh_${tmp_sh}_st_${tmp_sh}.mov
                            )>/dev/null
                            
                            if [[ ! "$?" -eq 0 ]]; then    
                                catch_error=true
                                echo_red " [ERROR: line ${LINENO}] $curFile"
                                echo_red "    ====> Failed making video at (scene=${tmp_scene}, mass=${tmp_mass}, bs=${tmp_bs}, shear=${tmp_sh}, stretch=${tmp_st})!"
                            fi
                        fi
                    done
            #     done
            # done
        done
    done
fi


# Do not exit when error occurred
if [ ${catch_error} = false ]; then
    if [ ${RENDER} = true ]; then
        echo_blue "     ====> Success"
    else
        echo_blue "     ====> Skip" 
    fi
else
    echo_red "     ====> Check error.log"
fi

)
#2> error.log
