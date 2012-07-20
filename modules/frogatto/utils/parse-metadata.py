#!/usr/bin/env python
# -*- coding: utf-8 -*-
files=["../MAC_APP_STORE_METADATA", "../iOS_APP_STORE_METADATA", "../CHANGELOG", "../iOS_CHANGELOG"]
files=[(x, open(x, "r").readlines()) for x in files]
for f in files:
	f = [f[0],[[x+1,f[1][x]] for x in range(len(f[1]))]]
	for line in f[1]:
		if line[1].strip():
			print('#: ' + f[0] + ':' + str(line[0]))
			print('msgid "' + line[1].strip(' * ').strip().replace('"','\\"') + '"')
			print('msgstr ""\n')
