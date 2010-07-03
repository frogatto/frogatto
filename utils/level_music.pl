#!/usr/bin/perl

# This script shows a list of a levels each song is used in (for all songs that are used)
# And a list of all songs not used in any level
# When run, cwd should be the root of the repository

use strict;

my %music = ();
my $list = `grep -rP '(music ?= ?|music(?:_onetime)?\\()' data/level/*.cfg`;
while ($list =~ m#data/level/(.+?):(?:music ?= ?"(.*?)"|.*?music(?:_onetime)?\('(.*?)')#g)
{
	my $music = $2 ? $2 : $3;
	$music{$music} = [] if (!exists $music{$music});
	push(@{$music{$music}}, $1);
}

for (sort keys %music)
{
	print "$_".($_ eq '' ? '<none>' : '').":\n";
	print "\t$_\n" for (@{$music{$_}});
}

print "\nMusic not used:\n";
chdir("music");
for (glob("*.ogg"))
{
	print "\t$_\n" if (!exists $music{$_});
}