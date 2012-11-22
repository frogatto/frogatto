#!/usr/bin/env python
# -*- coding: utf-8 -*-

print "TrainGen - The mass [animation] generator for Cube Trains.\nOutput:\n\n\n	#generated with ct-traingen utility.#"

for a in range(36):
	print """	[animation]
		id=normal-{0}
		image=experimental/cube trains/{1:0>4}.png
		rect=105,128,155,178
	[/animation]""".format(a, a+41)


print "\n\nEnd output."
