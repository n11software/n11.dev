#!/bin/bash

USERNAME="root"
HOST="178.156.163.28"

# Command to be executed on the remote server
REMOTE_COMMAND="bash -c 'apt update;apt upgrade -y;apt install sudo git curl -y;curl -sSf https://raw.githubusercontent.com/n11software/n11.dev/refs/heads/main/install.sh | sh;cd /var/lib/n11/;rm config.json;cp prod.json config.json;echo dns_cloudflare_api_key = 85e8043ecaf58c6c0f4b4fc16a100c814d9d0 > cloudflare.ini;echo dns_cloudflare_email = levicowan2005@icloud.com >> cloudflare.ini;sudo apt install certbot python3-certbot-dns-cloudflare -y;sudo certbot certonly --dns-cloudflare --dns-cloudflare-credentials cloudflare.ini --non-interactive --agree-tos --email levih@n11.dev -d *.n11.dev,n11.dev --expand; cp /etc/letsencrypt/live/n11.dev*/fullchain.pem /var/lib/n11/certificate.pem; cp /etc/letsencrypt/live/n11.dev*/privkey.pem /var/lib/n11/key.pem; sudo systemctl stop nginx; sudo killall -9 nginx; sudo apt remove nginx certbot python3-certbot-dns-cloudflare -y; chmod 777 key.pem; sudo systemctl restart n11;reboot'"

ssh "$USERNAME@$HOST" "$REMOTE_COMMAND"