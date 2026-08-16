#!/usr/bin/fish

set -x REDIS_IP "192.168.10.124"

echo $REDIS_IP

atlast  -v -I testEnv.atl

