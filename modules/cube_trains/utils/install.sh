#!/bin/bash
#set -e #Messes with return_code, not using.
set -u

#Return codes: 0 true, 1 user aborted, 2 clean failure, 3 dirty failure (installed deps, didn't unzip program).
game_name="Cube Trains"

return_code=0 #Used internally to record the returned code of the last operation, _not_ the code returned by this script when it exits.

_install_frogatto_dependancies()
{
	sudo apt-get install --no-upgrade --no-remove ccache libboost-dev libboost-regex-dev libboost-system-dev libglew1.5-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev libsdl1.2-dev libz-dev libpng12-dev g++ 
	return_code=$?
}

_make_frogatto()
{
	make clean
	make
	return_code=$?
	rm -f *.o *.d
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
	read -p "Installation failed. Press any key to exit. " -n 1 -r
	echo ""
	exit 2
fi
if [ "$return_code" != "0" ] #100: apt-get when it fails
then
	echo "We could not install the dependancies. If you're
running a package manager, try closing it. No
changes have been made."
	read -p "Installation failed. Press any key to exit. " -n 1 -r
	echo ""
	exit 2
fi

read -p "Good, that seems to have worked. Next, we'll
configure $game_name to run on your computer.
This may take quite a while. Continue? (Y/n)
> " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Nn]$ ]]
then
   exit 1
fi

_print_start
_make_frogatto
_print_finish

if [ "$return_code" != "0" ]
then
	echo "Unfortunately, something went wrong and $game_name
couldn't be configured. Sorry about that. If you
want, drop us a line on the forums and we'll see
if we can't get this sorted out. Include all the
above text."
	read -p "Installation failed. Press any key to exit. " -n 1 -r
	echo ""
	exit 3
fi

read -p "Well, that seems to have worked a treat. :)
You can now play $game_name by running the \"game\"
file. Would you like to install a desktop
shortcut for convenience?
(Y/n)
> " -n 1 -r
echo ""
make_shortcut=0
if [[ $REPLY =~ ^[Nn]$ ]]
then
   make_shortcut=1
fi

if [ "$make_shortcut" = "0" ]
then
	install_directory="$HOME/Desktop/$game_name"
	if [ -e "$install_directory" ]
	then
		rm -i "$install_directory"
	fi
	ln -s "$PWD" "$install_directory"
fi

read -p "All done. Run \"game\" now? (Y/n)
> " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Nn]$ ]]
then
   exit 0
fi
./game
exit 0
