#!/bin/bash
perl utils/graph-levels.pl $@ > foo2.bak
cat foo2.bak | grep -v '>' | grep -o 'N[0-9]* ' > foo.bak
for name in `cat foo.bak`
do
grep -c $name' ' foo2.bak
done > foo3.bak
paste foo.bak foo3.bak | grep  "\s1$" | sed -e 's/\s1//g' > foo4.bak
grep -v -f foo4.bak foo2.bak
rm foo.bak foo2.bak foo3.bak foo4.bak
