#!/bin/sh

# Strip {stuff like this} from stdin and output everything else
# as is
sed -e 's/{[^}]*}//g'
