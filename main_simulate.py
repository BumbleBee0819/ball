import os, subprocess, re, sys
from utils import runCmd, mkdirs, fileExist, rm
import json
import numpy as np
import random
import torch
import argparse

"""
##########################################################################################
Script Name                : main_simulate.py
Description                : This script runs FleX executable
Usage                      : 1) Show outputs: flex_verbose=Ture
                             2) Use parameters defined in the flexConfig.yml, set
                                --mass=-1.0, --bstiff=-1.0, --shstiff=-1.0, --ststiff=-1.0
Output                     : [outputFolder]/*.obj
Date                       : 14/08/2020
##########################################################################################
"""


def main_simulate():
    parser = argparse.ArgumentParser(description='This script use Flex to simulate cloth dynamics.')
    # Global
    parser.add_argument('--randomSeed', type=int, default=14, help='Fix the random seed.')
    parser.add_argument('--flex_verbose', type=int, default=0, help='0 to inhibit printings from FleX')
    # Experiments
    parser.add_argument('--scenes_str', type=str, default="wind", help='Scene name [If there are multiple scenes, use --scenes_list]')
    parser.add_argument('--scenes_list', type=list, default=[], help='a list of scene names [If there is only one scecne, use --scenes_str]')
    parser.add_argument('--bstiff_float', type=float, default=1.0, help='bend_stiffness [If there are multiple bs values, use --bstiff_list]')
    parser.add_argument('--bstiff_list', type=list, default=[], help='a list of bend_stiffness values [If there is only one bs value, use --bstiff_float]')
    # Flex
    parser.add_argument('--flexRootPath', type=str, default="FleX/", help='path to FleX root directory')
    parser.add_argument('--flexConfig', type=str, default="FleXConfigs/flexConfig.yml", help='FleX config file')
    parser.add_argument('--flex_input_root_path', type=str, default="dataset/trialObjs/", help='root path to FleX input .obj file')
    parser.add_argument('--obj_input',  type=str, default="model1.obj", help='name of the FleX input .obj file')    
    parser.add_argument('--flex_output_root_path',  type=str, default="experiments/simulation/trialObjs/", help='root path to FleX output .obj files')
    # Flex simulation paras
    parser.add_argument('--windstrength', type=float, default=0.3, help='wind strength')
    parser.add_argument('--scale', type=float, default=1.0, help='object scale')
    parser.add_argument('--rot', default=(0,0,0), help='Rotation of the object.')
    parser.add_argument('--local', type=bool, default=True, help='run Flex locally')
    #parser.add_argument('--clothColor', type=str, default="violet", help='')
    parser.add_argument('--floor', type=bool, default=False, help='')
    parser.add_argument('--offline', type=bool, default=True, help='')
    #parser.add_argument('--occluded', type=bool, default=True, help='')
    parser.add_argument('--useQuat', type=bool, default=False, help='')
    parser.add_argument('--psize', type=float, default=0.01, help='')
    parser.add_argument('--clothNumParticles', type=int, default=210, help='')
    parser.add_argument('--flexrandomSeed', type=int, default=-1, help='')
    parser.add_argument('--visSaveClothPerSimStep', type=bool, default=False, help='')
    parser.add_argument('--randomClothMinRes', type=int, default=145, help='')
    parser.add_argument('--randomClothMaxRes', type=int, default=215, help='')
    parser.add_argument('--mass', type=float, default=-1.0, help='invMass [Set -1 to use the value defined in --flexConfig]')
    parser.add_argument('--shstiff', type=float, default=-1.0, help='shear stiffness [Set -1 to use the value defined in --flexConfig]')
    parser.add_argument('--ststiff', type=float, default=-1.0, help='stretch stiffnessetch [Set -1 to use the value defined in --flexConfig]')
    parser.add_argument('--clothLift', type=float, default=-1.0, help='')
    parser.add_argument('--clothDrag', type=float, default=-1.0, help='')
    parser.add_argument('--clothFriction', type=float, default=1.1, help='')
    #parser.add_argument('--outputPath', type=str, default="", help='')
    parser.add_argument('--saveClothPerSimStep', type=int, default=1, help='')
    args = parser.parse_args()
    #print(args)
    # ----------------------------
    np.random.seed(args.randomSeed)
    random.seed(args.randomSeed)
    torch.manual_seed(args.randomSeed)
    torch.cuda.manual_seed(args.randomSeed)
    # ----------------------------


    # If there are multiple scenes
    if len(args.scenes_list) != 0:
        scenes = args.scenes_list
    else:
        scenes = [args.scenes_str]

    # If there are multiple bstiff values
    if len(args.bstiff_list) != 0:
        bstiff = [float(i) for i in args.bstiff_list]
    else:
        bstiff = [float(args.bstiff_float)]

    shstiff=float(args.shstiff)
    ststiff=float(args.ststiff)
    mass=float(args.mass)

    # Simulate .obj files for each scene
    for tmp_scene in scenes:
        objPath = os.path.join(args.flex_input_root_path, tmp_scene, args.obj_input)
        
        # ----- Setup environment ----- #
        FLEX_BIN_PATH = os.path.join(args.flexRootPath, "bin", "linux64", "NvFlexDemoReleaseCUDA_x64_" + tmp_scene)
        if not os.path.exists(FLEX_BIN_PATH):
            errorMessage="==> Error: No FleX binary found. Make sure you have set the right path and compiled FleX.\nTo compile FleX: singularity exec --nv kimImage.simg bash buildFleX.sh --build=true"
            sys.exit(errorMessage)
        else:
            os.environ["FLEX_PATH"] = args.flexRootPath
            # For libGLEW.so.1.10
        os.environ["LD_LIBRARY_PATH"] += ":{}".format(os.path.join(args.flexRootPath, "external"))
        # ----- End Setup environment ----- #

        for i in bstiff:
            '''
            Params:
                rot: Euler angles (in degrees) or quaternion (max unit norm)
            '''

            # e.g., "experiments/renderings/trialObjs/wind/wind"  The last wind is the base name for the output .obj files 
            outputPath = os.path.join(args.flex_output_root_path, tmp_scene+"_mass_"+str(mass)+"_bs_"+str(i)+"_sh_"+str(shstiff)+"_st_"+str(ststiff), tmp_scene)
            outputFolder = ('/').join(outputPath.split('/')[0:-1])
            rm(outputFolder)
            mkdirs(outputFolder)

            sim_cmd = [FLEX_BIN_PATH,
                "-obj={}".format(os.path.abspath(objPath)),
                "-config={}".format(os.path.abspath(args.flexConfig)),
                "-export={}".format(os.path.abspath(outputPath)),
                #"-iters={}".format(n_frames),
                #"-ss={}".format(n_substeps_per_frame),
                # rotate
                not args.useQuat and "-rx={} -ry={} -rz={}".format(*args.rot) or "-rx={} -ry={} -rz={} -rw={}".format(*args.rot),
                # cloth properties
                #"-stiff_scale={}".format(stiffness),
                # "-ccolor_id={}".format(COLOR_MAP[clothColor]),
            ]

            if not args.floor:
                sim_cmd.append("-nofloor")

            if args.offline:
                sim_cmd.append("-offline")
            # if not occluded:
            #     sim_cmd.append("-clothsize=1")
                # sim_cmd.append("-cam_dist={}".format(4.5))

            if args.useQuat:
                sim_cmd.append("-use_quat")
            else:
                sim_cmd.append("-use_euler")

            if args.visSaveClothPerSimStep:
                sim_cmd.append("-saveClothPerSimStep")

            if args.flex_verbose:
                sim_cmd.append("-v")
        
            sim_cmd.append("-bstiff={0:f}".format(i))
            sim_cmd.append("-psize={0:f}".format(args.psize))
            sim_cmd.append("-clothsize={0:d}".format(args.clothNumParticles))
            sim_cmd.append("-randomSeed={0:d}".format(args.flexrandomSeed))
            sim_cmd.append("-randomClothMinRes={0:d}".format(args.randomClothMinRes))
            sim_cmd.append("-randomClothMaxRes={0:d}".format(args.randomClothMaxRes))
            sim_cmd.append("-clothLift={0:f}".format(args.clothLift))
            sim_cmd.append("-scale={0:f}".format(args.scale))
            sim_cmd.append("-windstrength={0:f}".format(args.windstrength))
            sim_cmd.append("-mass={0:f}".format(float(args.mass)))
            sim_cmd.append("-shstiff={0:f}".format(float(args.shstiff)))
            sim_cmd.append("-ststiff={0:f}".format(float(args.ststiff)))
            sim_cmd.append("-clothDrag={0:f}".format(args.clothDrag))
            sim_cmd.append("-clothFriction={0:f}".format(args.clothFriction))
            sim_cmd.append("-saveClothPerSimStep={0:d}".format(args.saveClothPerSimStep))

            env = {}

            if args.local:
                env["DISPLAY"] = ":0"
            else:
                sim_cmd.insert(0, "vglrun")

            #print("---- Output Folder: ", outputFolder)

            # save params
            with open ((outputPath+'_main_simulate_paras.txt'), "w") as txt_file:
                for line in sim_cmd:
                    txt_file.write("".join(line) + "\n")

            lines_stdout = runCmd(" ".join(sim_cmd), extra_vars=env, verbose=args.flex_verbose)

            if (len(lines_stdout) < 50):
                print(lines_stdout, file=sys.stderr)
                sys.exit(1)



if __name__ == "__main__":
    main_simulate()