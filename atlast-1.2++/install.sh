#!/bin/bash
set -x

echo "Copy atlast .."
sudo cp ./atlast /usr/local/bin
echo ".. strip exexutable .."
sudo strip /usr/local/bin/atlast
echo ".. create lib"
sudo mkdir -p /usr/local/lib/Small
