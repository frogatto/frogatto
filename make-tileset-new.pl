use strict;

die "usage: $0 <tilename> <base> <image>" if @ARGV < 3;

my $tilename = shift @ARGV;
my $base = shift @ARGV;
my $image = shift @ARGV;

my $solid = '';
my $friend = $tilename;

while(my $arg = shift @ARGV) {
	if($arg eq '--solid') {
		$solid = "solid=" . (shift @ARGV);
	} elsif($arg eq '--friend') {
		$friend = shift @ARGV;
	} else {
		die $arg;
	}
}

while((length $tilename) < 3) {
	$tilename = " $tilename";
}

printf qq~
#horizontal tile
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
.* ,   ,.*  ,
$friend,$tilename,$friend,
.* ,   ,.* "
[/tile_pattern]
~, &coord($base, 3, 1);

printf qq~
#horizontal tile with one tile below but not on either side
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
.* ,   ,.*  ,
$friend,$tilename,$friend,
   ,$friend,   "
[/tile_pattern]
~, &coord($base, 0, 5);

printf qq~
#horizontal tile with one tile above but not on either side
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
.* ,   ,.*  ,
$friend,$tilename,$friend,
   ,$friend,   "
[/tile_pattern]
~, &coord($base, 2, 5);

printf qq~
#overhang
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
.* ,   ,.*  ,
   ,$tilename,$friend,
.* ,   ,.* "
[/tile_pattern]
~, &coord($base, 3, 0);

printf qq~
#overhang - reversed
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
.* ,   ,.*  ,
$friend,$tilename,   ,
.* ,   ,.* "
[/tile_pattern]
~, &coord($base, 3, 2);

printf qq~
#sloped
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
   ,    ,$friend?,
   ,$tilename,$friend,
$friend,$friend,$friend"
[/tile_pattern]
~, &coord($base, 0, 7);

printf qq~
#sloped - reversed
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend?,    ,   ,
$friend,$tilename,   ,
$friend,$friend,$friend"
[/tile_pattern]
~, &coord($base, 0, 8);

printf qq~
#single tile by itself
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
 .*,   , .*,
   ,$tilename,   ,
 .*,   , .*"
[/tile_pattern]
~, &coord($base, 3, 3);

printf qq~
#top of thin platform
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
 .*,   , .*,
   ,$tilename,   ,
 .*,$friend, .*"
[/tile_pattern]
~, &coord($base, 0, 3);

printf qq~
#part of thin platform
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
 .*,$friend, .*,
   ,$tilename,   ,
 .*,$friend, .*"
[/tile_pattern]
~, &coord($base, 1, 3);

printf qq~
#bottom of thin platform
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
 .*,$friend, .*,
   ,$tilename,   ,
 .*,   , .*"
[/tile_pattern]
~, &coord($base, 2, 3);

printf qq~
#cliff edge
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
  .*,   ,$friend?,
    ,$tilename,$friend ,
$friend?,$friend,$friend "
[/tile_pattern]
~, &coord($base, 0, 0);

printf qq~
#cliff edge - reverse
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend?,   ,.* ,
$friend,$tilename,   ,
$friend,$friend,$friend?"
[/tile_pattern]
~, &coord($base, 0, 2);

printf qq~
#cliff edge -- version with a corner underneath/opposite
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
  .*,   ,$friend?,
    ,$tilename,$friend ,
$friend?,$friend,    "
[/tile_pattern]
~, &coord($base, 0, 6);

printf qq~
#cliff edge (reversed) -- version with a corner underneath/opposite
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend?,   ,.*,
$friend,$tilename,   ,
    ,$friend,$friend?"
[/tile_pattern]
~, &coord($base, 0, 4);

printf qq~
#middle of a cross
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
   ,$friend,   ,
$friend,$tilename,$friend ,
   ,$friend,   "
[/tile_pattern]
~, &coord($base, 1, 5);

printf qq~
#corner at two angles
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend,$friend,   ,
$friend,$tilename,$friend,
   ,$friend,$friend"
[/tile_pattern]
~, &coord($base, 3, 5);

printf qq~
#corner at two angles (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
   ,$friend,$friend,
$friend,$tilename,$friend,
$friend,$friend,   "
[/tile_pattern]
~, &coord($base, 3, 4);

#TODO: Ask Neorice about this
if(0) {
printf qq~
#corners on the top
[tile_pattern]
image=$image
tiles=%s
rotate=90
$solid
pattern="
   ,$friend,   ,
$friend,$tilename,$friend,
$friend,$friend,$friend"
[/tile_pattern]
~, &coord($base, 1, 2);

printf qq~
#corners on the bottom
[tile_pattern]
image=$image
tiles=%s
rotate=-90
$solid
pattern="
$friend,$tilename,$friend,
$friend,$tilename,$friend,
   ,$friend,   "
[/tile_pattern]
~, &coord($base, 1, 2);
}

printf qq~
#corners both on the same side
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
   ,$friend,$friend,
$friend,$tilename,$friend,
   ,$friend,$friend"
[/tile_pattern]
~, &coord($base, 4, 7);

printf qq~
#corners both on the same side (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend,$friend,   ,
$friend,$tilename,$friend,
$friend,$friend,   "
[/tile_pattern]
~, &coord($base, 4, 6);

printf qq~
#inner top corner piece
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend,$friend,$friend,
$friend,$tilename,$friend,
   ,$friend,$friend"
[/tile_pattern]
~, &coord($base, 4, 1);

printf qq~
#inner top corner piece (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend,$friend,$friend,
$friend,$tilename,$friend,
$friend,$friend,   "
[/tile_pattern]
~, &coord($base, 4, 0);

printf qq~
#inner bottom corner piece
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
   ,$friend,$friend,
$friend,$tilename,$friend,
$friend,$friend,$friend"
[/tile_pattern]
~, &coord($base, 5, 1);

printf qq~
#inner bottom corner piece (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend,$friend,   ,
$friend,$tilename,$friend,
$friend,$friend,$friend"
[/tile_pattern]
~, &coord($base, 5, 0);

printf qq~
#corner at three sides
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
   ,$friend,$friend,
$friend,$tilename,$friend,
   ,$friend,   "
[/tile_pattern]
~, &coord($base, 4, 3);

printf qq~
#corner at three sides (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend,$friend,   ,
$friend,$tilename,$friend,
   ,$friend,   "
[/tile_pattern]
~, &coord($base, 4, 2);

printf qq~
#corner at three sides
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
   ,$friend,   ,
$friend,$tilename,$friend,
   ,$friend,$friend"
[/tile_pattern]
~, &coord($base, 5, 3);

printf qq~
#corner at three sides (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
   ,$friend,   ,
$friend,$tilename,$friend,
$friend,$friend,   "
[/tile_pattern]
~, &coord($base, 5, 2);

printf qq~
#roof at a corner
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
   ,$friend,$friend,
$friend,$tilename,$friend,
 .*,$friend, .*"
[/tile_pattern]
~, &coord($base, 5, 7);

printf qq~
#roof at a corner (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend,$friend,   ,
$friend,$tilename,$friend,
 .*,$friend, .*"
[/tile_pattern]
~, &coord($base, 5, 6);

printf qq~
#roof
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
.* ,$friend, .*,
$friend,$tilename,$friend,
 .*,   , .*"
[/tile_pattern]
~, &coord($base, 2, 1);

printf qq~
#bottom corner
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend?,$friend,$friend,
    ,$tilename,$friend,
.*  ,   , .*"
[/tile_pattern]
~, &coord($base, 2, 0);

printf qq~
#bottom corner (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend,$friend,$friend?,
$friend,$tilename,   ,
.*  ,   , .*"
[/tile_pattern]
~, &coord($base, 2, 2);

printf qq~
#bottom corner with corner on opposite side
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend?,$friend,   ,
    ,$tilename,$friend,
.*  ,   , .*"
[/tile_pattern]
~, &coord($base, 2, 6);

printf qq~
#bottom corner with corner on opposite side (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
   ,$friend,$friend?,
$friend,$tilename,   ,
.*  ,   , .*"
[/tile_pattern]
~, &coord($base, 2, 4);

printf qq~
#solid
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
$friend?,$friend,$friend?,
$friend,$tilename,$friend,
$friend?,$friend,$friend?"
[/tile_pattern]
~, &coord($base, 1, 1);

printf qq~
#cliff face coming up from a one-tile thick cliff and expanding out
#in one direction
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
$friend,$friend,.* ,
$friend,$tilename,   ,
   ,$friend,.* "
[/tile_pattern]
~, &coord($base, 4, 5);

printf qq~
#cliff face coming up from a one-tile thick cliff and expanding out
#in one direction (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
.* ,$friend,$friend,
   ,$tilename,$friend,
.* ,$friend,   "
[/tile_pattern]
~, &coord($base, 4, 4);

printf qq~
#cliff face coming down from a one-tile thick cliff and expanding out
#in one direction
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
   ,$friend,.* ,
$friend,$tilename,   ,
$friend,$friend,.* "
[/tile_pattern]
~, &coord($base, 5, 5);

printf qq~
#cliff face coming down from a one-tile thick cliff and expanding out
#in one direction (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
.* ,$friend,  ,
   ,$tilename,$friend,
.* ,$friend,$friend"
[/tile_pattern]
~, &coord($base, 5, 4);

printf qq~
#cliff face coming both up and down from a one-tile thick cliff and expanding
#out into a ledge in one direction
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
   ,$friend,.* ,
$friend,$tilename,   ,
   ,$friend,.* "
[/tile_pattern]
~, &coord($base, 1, 4);

printf qq~
#cliff face coming both up and down from a one-tile thick cliff and expanding
#out into a ledge in one direction (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
.* ,$friend,   ,
   ,$tilename,$friend,
.* ,$friend,   "
[/tile_pattern]
~, &coord($base, 1, 6);

printf qq~
#cliff face
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
.* ,$friend,.* ,
   ,$tilename,$friend,
.* ,$friend,.* "
[/tile_pattern]
~, &coord($base, 1, 0);

printf qq~
#cliff face (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
.* ,$friend,.* ,
$friend,$tilename,   ,
.* ,$friend,.* "
[/tile_pattern]
~, &coord($base, 1, 2);

printf qq~
#ground - with a corner on one side beneath
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
.* ,   ,.* ,
$friend,$tilename,$friend,
   ,$friend,$friend"
[/tile_pattern]
~, &coord($base, 3, 7);

printf qq~
#ground - with a corner on one side beneath (reversed)
[tile_pattern]
image=$image
reverse=no
tiles=%s
$solid
pattern="
.* ,   ,.* ,
$friend,$tilename,$friend,
$friend,$friend,   "
[/tile_pattern]
~, &coord($base, 3, 6);

printf qq~
#ground
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
$friend?,    ,$friend?,
$friend ,$tilename ,$friend ,
$friend?,$friend?,$friend?"
[/tile_pattern]
~, &coord($base, 0, 1);

sub base_unencode($) {
	my $base = lc(shift @_);
	$base = 10 + ord($base) - ord('a') if ord($base) >= ord('a') and ord($base) <= ord('z');
	return $base;
}

sub base_encode($) {
	my $val = shift @_;
	$val = chr(ord('a') + ($val - 10)) if $val >= 10;
	return $val;
}

sub coord($$$) {
	my ($base, $row, $col) = @_;
	my @base = split //, $base;
	$row += &base_unencode($base[0]);
	$col += &base_unencode($base[1]);

	return &base_encode($row) . &base_encode($col);
}
