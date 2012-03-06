#!/usr/bin/perl

use strict;

my %titles;
for my $f (glob("data/level/*.cfg"))
{
	my $file = `cat $f`;
	my $title = "NONE";
	my $dimensions = "NONE";
	if ($file=~/title="(.+?)"/)
	{
		$title = $1;
	}
	if ($file=~/dimensions="(.+?)"/)
	{
		$dimensions = $1;
	}
	
	$titles{$title} = [] if (ref($titles{$title}) ne "ARRAY");
	$f=~s/.+\/(.+)/$1/;
	push(@{$titles{$title}}, "$f: $dimensions");
}

for (keys %titles)
{
	if (scalar(@{$titles{$_}}) > 1)
	{
		print "$_: ".scalar(@{$titles{$_}})."\n";
		for (@{$titles{$_}})
		{
			print "\t$_\n";
		}
	}
}