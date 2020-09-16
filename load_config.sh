#!/bin/bash
#
############################################################################
# Filename      : load_config.sh
# Description   : This script loads from configuration file "default.conf"
# Arguments     : None
# Date          : 08/01/2020
############################################################################
#
#
#-------------- function definition -----------------------
# Here defines the global functions

echo_blue () {
    echo -e "\033[0;36m $@ \033[0m"
}

echo_red () {
    echo -e "\033[0;31m $@ \033[0m"
}

echo_green_check() {
    echo -e "\033[0;32m \u2714 \033[0m"
}

echo_red_cross() {
    echo -e "\033[0;31m x \033[0m"
}

 
#--------------  load config -------------------------------

if [ -f "user.conf" ]; then
    echo "Found user config, overriding default..."
    CFGFILE="user.conf"
else
    echo "No user config found, using default"
    CFGFILE="default.conf"
fi


while read line; do
    if [[ $line =~ ^"["(.+)"]"$ ]]; then
        arrname=${BASH_REMATCH[1]}
        declare -A $arrname
    elif [[ $line =~ ^"#"(.+)$ ]]; then   # comments start with #
        :
    elif [[ $line =~ ^([_[:alpha:]][_[:alnum:]]*)":"(.*) ]]; then
        tmpkey="${BASH_REMATCH[1]}"
        tmpvalue="${BASH_REMATCH[2]}"

        if [[ $tmpvalue =~ ^"("(.+)")"$ ]]; then
            tmp=${BASH_REMATCH[1]}
            declare ${arrname}[$tmpkey]=${tmp[@]}
        else
            declare ${arrname}[$tmpkey]="${tmpvalue}"
        fi
    fi
done < $CFGFILE

