use strict;

die "usage: $0 <tilename> <base> <image>" if @ARGV < 3;

open DATE, "date |" or die;
my $date = '';
while(my $line = <DATE>) {
	chomp $line;
	$date .= $line;
}
close DATE;

my $command_line = "$0 " . (join ' ', @ARGV);
print "# Generated on $date using:\n#  $command_line\n";

my $tilename = shift @ARGV;
my $base = shift @ARGV;
my $image = shift @ARGV;

$tilename = "($tilename)" if $tilename =~ /[a-zA-Z0-9]+/;

my $solid = '';
my $grass = '';
my $friend = $tilename;
my $noslopes = 0;

while(my $arg = shift @ARGV) {
	if($arg eq '--grass') {
		$grass = shift @ARGV;
	} elsif($arg eq '--solid') {
		$solid = "solid=" . (shift @ARGV);
	} elsif($arg eq '--friend') {
		$friend = shift @ARGV;
	} elsif($arg eq '--noslopes') {
		$noslopes = 1;
	} else {
		die "Unrecognized argument: $arg";
	}
}

while((length $tilename) < 3) {
	$tilename = " $tilename";
}

if($grass) {

printf qq~
#grass top - thin ledge
[tile_pattern]
image=$image
tiles=%s
pattern="
$friend?,$friend?, $friend?,
$friend?,   , $friend?,
$grass, $grass, $grass,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 2, 10), &coord($base, 4, 13);

printf qq~
#grass - thin ledge
[tile_pattern]
image=$image
tiles=%s
solid=yes
pattern="
$friend?,   , $friend?,
$grass, $grass, $grass,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 3, 10), &coord($base, 5, 13);

printf qq~
#grass top - thin ledge with no grass to the left.
[tile_pattern]
image=$image
tiles=%s
solid=no
reverse=no
pattern="
$friend?,$friend?, $friend?,
$friend?,   , $friend?,
$friend, $grass, $grass,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 2, 9), &coord($base, 4, 12);

printf qq~
#grass - thin ledge with no grass to the left.
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
$friend, $grass, $grass,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 3, 9), &coord($base, 5, 12);

printf qq~
#grass top - thin ledge with no grass to the right.
[tile_pattern]
image=$image
tiles=%s
solid=no
reverse=no
pattern="
$friend?,$friend?, $friend?,
$friend?,   , $friend?,
$grass, $grass, $friend,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 2, 11), &coord($base, 4, 14);

printf qq~
#grass - thin ledge with no grass to the right.
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
$grass, $grass, $friend,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 3, 11), &coord($base, 5, 14);

printf qq~
#grass top - thin ledge with no grass on either side.
[tile_pattern]
image=$image
tiles=%s
reverse=no
pattern="
$friend?,$friend?,$friend?,
$friend?,   , $friend?,
$friend, $grass, $friend,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 2, 12), &coord($base, 4, 15);

printf qq~
#grass - thin ledge with no grass on either side.
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
$friend, $grass, $friend,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 3, 12), &coord($base, 5, 15);

printf qq~
#grass top - left edge of thin ledge
[tile_pattern]
image=$image
tiles=%s
reverse=no
pattern="
$friend?,$friend?,$friend?,
$friend?,   , $friend?,
   , $grass, $friend,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 0, 9), &coord($base, 4, 12);

printf qq~
#grass - left edge of thin ledge
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
   , $grass, $friend,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 1, 9), &coord($base, 5, 12);

printf qq~
#grass top - right edge of thin ledge
[tile_pattern]
image=$image
tiles=%s
reverse=no
pattern="
$friend?,$friend?,$friend?,
$friend?,   , $friend?,
$friend, $grass,   ,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 0, 10), &coord($base, 4, 14);

printf qq~
#grass - right edge of thin ledge
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
$friend, $grass,   ,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 1, 10), &coord($base, 5, 14);

printf qq~
#grass top - single tile
[tile_pattern]
image=$image
tiles=%s
reverse=no
pattern="
$friend?,$friend?,$friend?,
$friend?,   , $friend?,
   , $grass,   ,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 0, 11), &coord($base, 4, 15);

printf qq~
#grass - single tile
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
   , $grass,   ,
$friend?,   , $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 1, 11), &coord($base, 5, 15);

printf qq~
#grass top - regular ground as part of a long line of grass
[tile_pattern]
image=$image
tiles=%s
reverse=no
pattern="
$friend?,$friend?, $friend?,
$friend?,   , $friend?,
$grass, $grass,$grass,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 4, 10), &coord($base, 4, 13);

printf qq~
#grass - regular ground as part of a long line of grass
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
$grass, $grass,$grass,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 5, 10), &coord($base, 5, 13);

printf qq~
#grass top - with no grass (just dirt) to the left
[tile_pattern]
image=$image
tiles=%s
reverse=no
pattern="
$friend?,$friend?, $friend?,
$friend?,   , $friend?,
$friend, $grass,$grass,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 0, 12), &coord($base, 4, 12);

printf qq~
#grass - with no grass (just dirt) to the left
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
$friend, $grass,$grass,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 1, 12), &coord($base, 5, 12);

printf qq~
#grass top - with no grass (just dirt) to the right
[tile_pattern]
image=$image
tiles=%s
reverse=no
pattern="
$friend?,$friend?, $friend?,
$friend?,   , $friend?,
$grass, $grass,$friend,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 0, 13), &coord($base, 4, 14);

printf qq~
#grass - with no grass (just dirt) to the right
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
$grass, $grass,$friend,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 1, 13), &coord($base, 5, 14);

printf qq~
#grass top - with dirt on either side
[tile_pattern]
image=$image
tiles=%s
reverse=no
pattern="
$friend?,$friend?, $friend?,
$friend?,   , $friend?,
$friend, $grass,$friend,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 2, 13), &coord($base, 4, 15);

printf qq~
#grass - with dirt on either side
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
$friend, $grass,$friend,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 3, 13), &coord($base, 5, 15);

printf qq~
#grass top - left side of cliff
[tile_pattern]
image=$image
tiles=%s
reverse=no
pattern="
$friend?,$friend?, $friend?,
$friend?,   , $friend?,
   , $grass,$grass,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 4, 9), &coord($base, 4, 12);

printf qq~
#grass - left side of cliff
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
   , $grass,$grass,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 5, 9), &coord($base, 5, 12);

printf qq~
#grass top - right side of cliff
[tile_pattern]
image=$image
tiles=%s
reverse=no
pattern="
$friend?,$friend?, $friend?,
$friend?,   , $friend?,
$grass,$grass,   ,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 4, 11), &coord($base, 4, 14);

printf qq~
#grass - right side of cliff
[tile_pattern]
image=$image
tiles=%s
solid=yes
reverse=no
pattern="
$friend?,   , $friend?,
$grass,$grass,   ,
$friend?,$friend, $friend?"
	[tile]
	image=$image
	tiles=%s
	zorder=1
	[/tile]
[/tile_pattern]
~, &coord($base, 5, 11), &coord($base, 5, 14);

} #end if($grass)

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
~, &coord($base, 0, $noslopes ? 0 : 7);

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
~, &coord($base, 0, $noslopes ? 2 : 8);

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

printf qq~
#corners on the top
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
   ,$friend,   ,
$friend,$tilename,$friend,
$friend,$friend,$friend"
[/tile_pattern]
~, &coord($base, 5, 8);

printf qq~
#corners on the bottom
[tile_pattern]
image=$image
tiles=%s
$solid
pattern="
$friend,$tilename,$friend,
$friend,$tilename,$friend,
   ,$friend,   "
[/tile_pattern]
~, &coord($base, 4, 8);

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
