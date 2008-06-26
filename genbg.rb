width = 100
height = 40
divisions = 4

width = ARGV[0].to_i if ARGV.size > 0
height = ARGV[1].to_i if ARGV.size > 1

height_map = [0] * width

height_map[0] = rand*height
height_map[height_map.size-1] = rand*height

def build_height_map(total_height, heights, i1, i2, divisions)
	return if i2 - i1 < 2

	i = (i1 + i2)/2
	if divisions > 0
		heights[i] = rand*total_height
	else
		heights[i] = (heights[i1] + heights[i2])/2 + rand*(i2 - i1) - rand*(i2 - i1)
	end
	build_height_map(total_height, heights, i1, i, divisions - 1)
	build_height_map(total_height, heights, i, i2, divisions - 1)
end

build_height_map(height, height_map, 0, height_map.size-1, divisions)

for y in 0..(height-1)
	for x in 0..(width-1)
		print "g" if y > height_map[x]
		print "," if x != (width-1)
	end
	print "\n"
end
