#!/usr/bin/perl

# This script shows a list of a levels each song is used in (for all songs that are used)
# When run, cwd should be the root of the repository

use strict;

my %music = ();
my $list = `grep -rP "music ?= ?" data/level/*.cfg`;
while ($list =~ m#data/level/(.+?):music ?= ?"(.*?)"#g)
{
	$music{$2} = [] if (!exists $music{$2});
	push(@{$music{$2}}, $1);
}

for (sort keys %music)
{
	print "$_".($_ eq '' ? '<none>' : '').":\n";
	print "\t$_\n" for (@{$music{$_}});
}