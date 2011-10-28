#!/bin/bash
# this script uses ffmpeg to convert all of our ogg/wav sound effects to the exact wav format amenable to the iPhone's audio coprocessor.
# cwd should be the base of the git repo
# you'll naturally need to compile ffmpeg, which at the time of this writing, also requires yasm

src_directory="./sounds"
dst_directory="./sounds_wav"
ffmpeg_settings="-ac 1 -ar 44100 -acodec pcm_s16le"
if [ -d $src_directory ] && [ -d $dst_directory ]; then
	for f in $( find $src_directory ); do
		destfile=( `echo $f | sed 's/sounds/sounds_wav/' | sed 's/\.[^.]*$//'`.wav );
		if [ ! -d `dirname $destfile` ]; then
			#echo "dir doesn't exist:" `dirname $destfile`;
			mkdir `dirname $destfile`;
		fi
		#the regex in the `` code will replace any file extension with wav
		ffmpeg -i $f $ffmpeg_settings $destfile;
	done
fi

