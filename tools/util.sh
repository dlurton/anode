

NUM_CORES=`grep -c ^processor /proc/cpuinfo`

LIGHT_GREEN="\e[92m"
LIGHT_YELLOW="\e[93m"
NORMAL="\e[0m"

function say() { 
MSG=$@
	echo -e "$LIGHT_GREEN**************************************************************************"
	echo -e "*$LIGHT_YELLOW $MSG"
	echo -e "$LIGHT_GREEN**************************************************************************$NORMAL"

    sleep 1 #give time for the human running this script to read it... 
}

