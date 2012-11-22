#!/usr/bin/perl

use strict;

my %objects = ();
my %events = ();
my $empty_samples = 0;
my $busy_samples = 0;
my $ffl_samples = 0;
while(my $line = <STDIN>) {
	if(my ($samples) = $line =~ /SAMPLES EMPTY: (\d+)/) {
		$empty_samples = $samples;
		next;
	}

	chomp $line;
	my ($obj, $event, $mode) = split /:/, $line or die "invalid line: $line";

	++$ffl_samples if $mode eq 'FFL';
	++$busy_samples;

	$objects{$obj}++;
	$events{"$obj:$event"}++;
}

my $total_samples = $empty_samples + $busy_samples;
print "TOTAL SAMPLES: $total_samples\n";
my $empty_percent = sprintf "%.1f", (100*$empty_samples/$total_samples);
my $ffl_percent = sprintf "%.1f", (100*$ffl_samples/$total_samples);
my $cmd_percent = sprintf "%.1f", (100*($busy_samples - $ffl_samples)/$total_samples);

print "
$empty_percent% of CPU usage was unrelated to object event handling
$ffl_percent% of CPU usage was spent evaluating formulas
$cmd_percent% of CPU usage was spent executing commands returned from formulas

";

sub output_map {
	my $map = shift @_;
	my @array = ();
	foreach my $key (keys %$map) {
		push @array, [$key, $map->{$key}];
	}

	@array = sort {$b->[1] <=> $a->[1]} @array;
	foreach my $item (@array) {
		my $a = $item->[0];
		my $b = $item->[1];
		my $percent = sprintf "%.1f", (100*$b/$busy_samples);
		print "$percent% $a\n";
	}
}

print " == OBJECTS ==\n";
&output_map(\%objects);

print "\n\n == EVENTS ==\n";
&output_map(\%events);
