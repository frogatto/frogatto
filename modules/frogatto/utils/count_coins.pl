#!/usr/bin/perl

use strict;

my $total = 0;

for my $file (@ARGV)
{
	next if (!-f $file);
	my $count = 0;
	my $str = `cat "$file"`;
	$count += ($2 ? 10 : 1) while ($str=~/type="((big_)?gold_coin|gold_berry)"/g);
	print "$file: $count\n";
	$total += $count;
}

print "Total: $total\n";
