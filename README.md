[![GitHub issues](https://img.shields.io/github/issues/Naereen/StrapDown.js.svg)](https://github.com/CNCLgithub/crossdomain_cloth_perception/issues/)
> Author: Wenyan Bi, Marc Harary, Ilker Yildirim

# Crossdomain_cloth_perception

```bash

Folder Structure
=================

    ├── FleX                   # FleX codes
    ├── dataset                # input.obj file to FleX
    ├── experiments             
        ├── obj2mov            # blender-2.79a-linux64, and all .blend files
        ├── simulation         # output folder of FleX
        ├── rendering          # rendered images from Blender
        ├── video              # final output videos
    ├── run.sh                 # main process flow control            

```





## Instruction ##

1- Clone the repo and cd into it and do the following steps in the repo's root directory

2- Run `bash setenv.sh --download_flex=true --download_blender=true`

3- Run `sudo bash run.sh`.
  This script will do the following things in order, and output errors to `error.log`: 
 - [*check cuda*]:  to make sure cuda will be initialized within singularity container (this step must be run outside singularity container).
 - [*build flex executables*]:  it will create one executable for each scene that is defined in ${FLEXSCENES['scenes']} in [default.conf](https://github.com/CNCLgithub/crossdomain_cloth_perception/blob/master/default.conf)
 - [*simulate with flex*]: it will read 3 stiffness values from [default.conf](https://github.com/CNCLgithub/crossdomain_cloth_perception/blob/master/default.conf). If the values are undefined (i.e., equals -1), it will instead read input from [flexConfig.yml](https://github.com/CNCLgithub/crossdomain_cloth_perception/blob/master/FleXConfigs/flexConfig.yml).
 - [*render with blender*]: by default it uses blender-2.79a-linux64.
 - [*make videos with ffmpeg*]. 

If you want to skip some steps, change the according paras in run.sh to false.

Before you do 3 make sure you have put your `obj` files in [`dataset/trialObjs/[scene]`](https://github.com/CNCLgithub/crossdomain_cloth_perception/tree/master/dataset/trialObjs/wind). Make sure your `obj` files are named as `model1.obj`.

Check [`main_simulate.py`](https://github.com/CNCLgithub/crossdomain_cloth_perception/blob/master/main_simulate.py) to see all of the input arguments for flex simulation. \
Check [`obj2mov.py`](https://github.com/CNCLgithub/crossdomain_cloth_perception/blob/master/obj2mov.py) to see all of the input arguments for blender rendering.





## Notes ##
1. Flex:
2. Blender:
3. Important files and folders:
