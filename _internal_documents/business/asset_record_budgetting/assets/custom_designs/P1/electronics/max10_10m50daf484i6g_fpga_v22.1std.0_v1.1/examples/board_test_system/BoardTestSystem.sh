#!/bin/sh

echo "****************************************"
echo "*          BOARD TEST SYSTEM           *"
echo "****************************************"
echo ""

if [ "$QUARTUS_ROOTDIR" == ""  -o ! -d $QUARTUS_ROOTDIR/linux64/jre64/bin ]; then
        echo "---------------------------"
        echo "JRE library is not correct!"
        echo "---------------------------"

        if [ "$QUARTUS_ROOTDIR" == "" ]; then
			echo "The environment variable QUARTUS_ROOTDIR cannot be found."
			echo "Please add QUARTUS_ROOTDIR with command or script."
			echo "For example: export QUARTUS_ROOTDIR=\"<quartus-install-path>/quartus\""
			echo ""
			exit
		fi

		if [ ! -d $QUARTUS_ROOTDIR/linux64/jre64/bin ]; then
			echo "$QUARTUS_ROOTDIR/linux64/jre64/bin cannot be found."
			echo "Please check the directory."
			echo "Or you could put the OpenJRE 8 to the directory."
			echo ""
			exit
		fi

	exit
fi

if [ -f bts_log.txt ]; then
        chmod 777 bts_log.txt
fi

echo "Board Test System starting..."
export PATH=$QUARTUS_ROOTDIR/linux64/jre64/bin:$PATH
java -Xmx256m -jar bts.jar