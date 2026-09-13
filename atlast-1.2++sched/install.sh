#!/bin/bash
# set -x

echo "Copy atlast .."
sudo cp ./atlast /usr/local/bin
echo ".. strip executable .."
sudo strip /usr/local/bin/atlast
echo ".. create lib"
sudo mkdir -p /usr/local/lib/atlast
echo ".. copy files FROM repo to  lib"
# sudo cp start.atl  /usr/local/lib/atlast
echo ".. copy script"
cp ./forth $HOME/bin

