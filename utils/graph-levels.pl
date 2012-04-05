#!/usr/bin/perl

use strict;

#This util generates a graph of all the levels in the levels folder.
#run using "perl utils/graph-levels.pl | dot -Tpng > out.png" in the root frogatto folder. Outputs to out.png to cwd.

# appending --show_music after utils/graph-levels.pl will write the song used in each level, under the level's name

my $show_music = 0;

while(my $arg = shift @ARGV) {
	if($arg eq '--show_music') {
		$show_music = 1;
	} else {
		die "Unrecognized argument: $arg";
	}
}




my @levels = glob("data/level/*");

my %index = ();
my $index = 1;

print qq~digraph "frogatto" {
node [width=1.0,height=1.0];
~;

my @adj = ();

foreach my $level (@levels) {
	open LVL, "<$level" or die;

	$level =~ s/.*\///;

	$index{$level} = $index;

	my $door = '';
	my $saves = 0;
	my $music = '';

	while(my $line = <LVL>) {
		if(my ($toilet) = $line =~ /type: "(save_toilet|dungeon_save_door)"/) {
			++$saves;
		}

		if(my ($label) = $line =~ /label: "(.*)"/) {
			$door = $label;
		}

		if(my ($song_name) = $line =~ /music: "(.*)"/) {
			$music = $song_name;
		}

		if(my ($next_level) = $line =~ /next_level: "(.*)"/) {
			push @adj, [$level, $next_level, 'next_level'];
		}

		if(my ($previous_level) = $line =~ /previous_level: "(.*)"/) {
			push @adj, [$level, $previous_level, 'prev_level'];
		}

		if(my ($dest_level) = $line =~ /dest_level: "(.*)"/) {
			push @adj, [$level, $dest_level, $door];
		}
	}

	close LVL;	

	my $label = $level;
	$label .= " (save)" if $saves == 1;
	$label .= " ($saves saves)" if $saves > 1;
	if($show_music){
		$label .= "\\n";
		$label .= $music;
	}

	print qq~N$index [label="$label",shape=box,fontsize=12];\n~;

	++$index;
}

foreach my $adj (@adj) {
	my ($src, $dst, $door) = @$adj;
	my ($a, $b) = ($index{$src}, $index{$dst});
	print qq~N$a -> N$b [label="$door", weight=1, style="setlinewidth(1.0)"];\n~
	   if $a and $b;
}

print "}";
