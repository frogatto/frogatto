#!/usr/bin/perl

use strict;

#This util generates a graph of all the levels in the levels folder.
#run using "perl utils/graph-levels.pl | dot -Tpng > out.png" in the root frogatto folder. Outputs to out.png to cwd.

# appending --show_music after utils/graph-levels.pl will write the song used in each level, under the level's name

# appending --show_heart_pieces after utils/graph-levels.pl will write the number of max heart piece objects in the level, by type

my $show_music = 0;
my $show_heart_pieces = 0;
my $show_coins = 0;
my $show_palettes = 0;

while(my $arg = shift @ARGV) {
	if($arg eq '--show_music') {
		$show_music = 1;
	} elsif($arg eq '--show_heart_pieces') {
		$show_heart_pieces = 1;
	} elsif($arg eq '--show_coins') {
		$show_coins = 1;
	} elsif($arg eq '--show_palettes') {
		$show_palettes = 1;
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
	my $fg_palettes = '';
	my $bg_palette = '';
	my $heart_pieces = 0;
	my $full_heart_pieces = 0;
	my $coins = 0;

	while(my $line = <LVL>) {
		if(my ($toilet) = $line =~ /type"?: "(save_toilet|dungeon_save_door)"/) {
			++$saves;
		}

		if(my ($label) = $line =~ /label"?: "(.*)"/) {
			$door = $label;
		}

		if(my ($song_name) = $line =~ /music"?: "(.*)"/) {
			$music = $song_name;
		}
		
		if(my ($fg_palettes_) = $line =~ /palettes"?: (\[(.*)\]|"(.*)")/) {
			$fg_palettes = $fg_palettes_;
			$fg_palettes =~ s/\"//g;
		}
		
		if(my ($bg_palette_) = $line =~ /background_palette"?: "(.*)"/) {
			$bg_palette = $bg_palette_;
		}
		
		if(my ($heart_object) = $line =~ /type"?: "(max_heart_object)"/) {
			++$full_heart_pieces;
		}
		
		if(my ($heart_object) = $line =~ /type"?: "(partial_max_heart_object)"/) {
			++$heart_pieces;
		}
		
		if(my ($heart_object) = $line =~ /type"?: "(coin_silver)"/) {
			$coins += 1;
		}
		
		if(my ($heart_object) = $line =~ /type"?: "(coin_gold)"/) {
			$coins += 5;
		}
		
		if(my ($heart_object) = $line =~ /type"?: "(coin_gold_big)"/) {
			$coins += 20;
		}
		
		if(my ($heart_object) = $line =~ /type"?: "(coin_gold_enormous)"/) {
			$coins += 100;
		}
		
		if(my ($heart_object) = $line =~ /type"?: "(gold_berry)"/) {
			$coins += 1;
		}
		
		if(my ($next_level) = $line =~ /next_level"?: "(.*)"/) {
			push @adj, [$level, $next_level, 'next_level'];
		}

		if(my ($previous_level) = $line =~ /previous_level"?: "(.*)"/) {
			push @adj, [$level, $previous_level, 'prev_level'];
		}

		if(my ($dest_level) = $line =~ /dest_level"?: "(.*)"/) {
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
	if($show_palettes){
		$label .= "\\n";
		$label .= $fg_palettes . " fg";
		$label .= "\\n";
		$label .= $bg_palette . " bg";
	}
	if($show_heart_pieces and ($heart_pieces > 0 or $full_heart_pieces > 0)){
		use integer;
		$label .= "\\n";
		$label .= $full_heart_pieces . " full " . $heart_pieces . " partial hearts";
	}
	if($show_coins and $coins > 0){
		$label .= "\\n";
		$label .= "coins worth " . $coins;
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
