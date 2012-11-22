#!/usr/bin/perl

# Assumes it's in the root project directory.
# Copy and paste the files into the array to use (I know, super hacky).
# You'll want to check the latest updated files in each directory to see what needs
# updating.

@files = qw(Paddle7.ogg
Paddle6.ogg
Paddle5.ogg
Paddle4.ogg
Paddle3.ogg
Paddle2.ogg
Paddle1.ogg
water-exit.ogg
water-enter.ogg
bubble4.ogg
bubble3.ogg
bubble2.ogg
bubble1.ogg
acid.ogg
laser.ogg
energyshot.ogg
chuff.ogg);

for (@files)
{
	$out = $_;
	$out =~s/\.ogg/.wav/;
	system("ffmpeg -y -i sounds/$_ -ac 1 sounds_wav/$out");
}