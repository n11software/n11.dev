#!/bin/bash

USERNAME="root"
HOST="n11.dev"

# Command to be executed on the remote server
REMOTE_COMMAND="bash -c 'echo dns_cloudflare_api_token =  > cloudflare.ini; apt update;apt upgrade -y;apt install sudo git curl -y;curl -sSf https://raw.githubusercontent.com/n11software/n11.dev/refs/heads/main/install.sh | sh;cd /var/lib/n11/;rm config.json;cp prod.json config.json;sudo apt install certbot python3-certbot-dns-cloudflare -y;sudo certbot certonly --dns-cloudflare --dns-cloudflare-credentials cloudflare.ini --non-interactive --agree-tos --email levih@n11.dev -d *.n11.dev; cp /etc/letsencrypt/live/n11.dev*/fullchain.pem /var/lib/n11/certificate.pem; cp /etc/letsencrypt/live/n11.dev*/privkey.pem /var/lib/n11/key.pem; sudo systemctl stop nginx; sudo killall -9 nginx; sudo apt remove nginx certbot python3-certbot-dns-cloudflare -y; chmod 777 key.pem; sudo systemctl restart n11'"

ssh "$USERNAME@$HOST" "$REMOTE_COMMAND"