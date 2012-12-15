#!/bin/bash
#set -e #Messes with return_code, not using.
set -u

#Return codes: 0 true, 1 user aborted, 2 clean failure, 3 dirty failure (installed deps, didn't unzip program).
game_name="Cube Trains"
REPLY="y"

return_code=0 #Used internally to record the returned code of the last operation, _not_ the code returned by this script when it exits.

_install_frogatto_dependancies()
{
	sudo apt-get install ccache libboost-dev libboost-regex-dev libboost-system-dev libglew1.5-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev libsdl1.2-dev libz-dev libpng12-dev	# < Yeah, install the dev versions, because I have nfi what the proper versions are!
#	sudo apt-get install ccache libboost libboost-regex libboost-system libglew1.6 libsdl-image1.2 libsdl-mixer1.2 libsdl-ttf2.0 libsdl1.2 libz libpng12											# < These are not the proper versions, of course. :|
	return_code=$?
}

_print_start()
{
	echo "

=============== Start. =============== 
 "
}

_print_finish()
{
	echo "
=============== Finish. =============== "
echo "(Code" $return_code".)"
echo "
"
}

read -p "Hello! This script will install $game_name on your
computer. It will prompt you at every step. Would
you like to install $game_name now? (Y/n)
> " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Nn]$ ]]
then
	exit 1
fi

read -p "Great! The first thing we need to do is to install
some stuff $game_name needs to run. We will need
root permission for this, so you might be
prompted to type in a password. This step might
take a while to finish, and requires an internet
connection.
Continue? (Y/n)
> " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Nn]$ ]]
then
   exit 1
fi

_print_start
_install_frogatto_dependancies
_print_finish

if [ "$return_code" == "1" ] #1: sudo on auth failure
then
	echo "We could not get permission to run the
installation command, so nothing's been changed.
(Try a different password?)"
	read -p "Installation failed. Press any key to exit." -n 1 -r
	exit 2
fi
if [ "$return_code" != "0" ] #100: apt-get when it fails
then
	echo "We could not install the dependancies. If you're
running a package manager, try closing it. No
changes have been made."
	read -p "Installation failed. Press any key to exit." -n 1 -r
	exit 2
fi

read -p "Well, that worked a treat. :)
You can now play $game_name by clicking either
\"game32\" or \"game64\", depending on your OS,
or by typing \"./game32\" or \"./game64\". If the
game does not work, please drop us a line!

Game installed. Press any key to exit." -n 1 -r
exit 0