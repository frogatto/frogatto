import sys

def main(f):
	tiles = open(f, 'r').readlines()
	tiles = "".join(tiles).replace("\n","")
	tiles = "".join(['{0},'.format(x) for x in tiles])[:-1]
	tiles = "land = [" + tiles + "]"
	print tiles

if __name__ == "__main__":
	if len(sys.argv) == 2:
		main(sys.argv[1])
		
