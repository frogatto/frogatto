set -e
set -u

if [ -f "bash_autocomplete.sh" ];
then
	relative_file="bash_autocomplete.sh"
else
	if [ -f "utils/bash_autocomplete.sh" ];
	then
		relative_file="utils/bash_autocomplete.sh"
	else
	   echo -e "\e[00;31mError:\e[00m Failed to find bash_autocomplete.sh. (You may need to navigate to the folder it's in.)"
	   exit 1
	fi
fi

sudo rm -f /etc/bash_completion.d/frogatto
sudo ln ${relative_file} /etc/bash_completion.d/frogatto
echo -e "\e[00;32mSuccess:\e[00m Tab completion installed. Please restart your console or run '. ${relative_file}'."