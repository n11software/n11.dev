#!/bin/bash

USERNAME="root"
HOST="n11.dev"

# Command to be executed on the remote server
REMOTE_COMMAND="bash -c 'cd /var/lib/n11/;git pull;sudo systemctl stop n11;./build.sh;sudo cp N11 /usr/local/bin/N11;sudo systemctl restart n11'"

ssh "$USERNAME@$HOST" "$REMOTE_COMMAND"