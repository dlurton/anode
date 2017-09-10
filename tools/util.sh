#@IgnoreInspection BashAddShebang



LIGHT_GREEN="\e[92m"
LIGHT_YELLOW="\e[93m"
NORMAL="\e[0m"

function say() { 
MSG=$@
	echo -e "$LIGHT_GREEN**************************************************************************"
	echo -e "*$LIGHT_YELLOW $MSG"
	echo -e "$LIGHT_GREEN**************************************************************************$NORMAL"

    #sleep 1 #give time for the human running this script to read it... 
}


# https://www.koszek.com/blog/2016/07/25/dealing-with-large-jobs-on-travis/

travis_fold() {
  local action=$1
  local name=$2
  echo -en "travis_fold:${action}:${name}\r"
}


if [ "$ANODE_BUILD_TYPE" != "Debug" ]; then
    #say "\$ANODE_BUILD_TYPE is not set to 'Debug', defaulting to 'Release'"
    export ANODE_BUILD_TYPE="Release"
fi

# $NUM_CORES is later passed to LLVM's and Anode's make command as the value of the -j argument, which determines build parallelism.
if [ "$NUM_CORES" = "" ]; then
    NUM_CORES=`grep -c ^processor /proc/cpuinfo`
fi
