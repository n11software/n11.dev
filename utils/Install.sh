sudo apt update;sudo apt upgrade -y;sudo apt install -y git nodejs npm;

NODE=$(which node)

# Create N11 user
useradd -m n11

# clone the repo
git clone https://github.com/N11Software/n11.dev /home/n11/n11.dev

setcap 'cap_net_bind_service=+ep' $NODE

# Install the dependencies
cd /home/n11/n11.dev
npm install

# move the service file
cp /home/n11/n11.dev/utils/N11.service /etc/systemd/system/n11.service
systemctl enable --now n11

cp /home/n11/n11.dev/utils/Surreal.service /etc/systemd/system/surreal.service
systemctl enable --now surreal