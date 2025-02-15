#!/bin/bash

if [ ! -d .git/ ]; then
    sudo mkdir -p /var/lib/n11
    git clone https://github.com/N11Software/n11.dev /var/lib/n11
    cd /var/lib/n11
fi

git pull

# check OS
case "$(uname -s)" in
    Darwin*)
        OS="OSX"
        ;;
    Linux*)
        OS="Linux"
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            DISTRO=$NAME
        else
            echo "OS not supported"
            exit 1
        fi
        ;;
    *)
        echo "OS not supported"
        exit 1
        ;;
esac

# Check for dependencies
# Check for openssl-dev
if [ "$OS" = "Linux" ]; then
    if [ "$DISTRO" = "Ubuntu" ]; then
        if [ ! -f /usr/include/openssl/ssl.h ]; then
            echo "Installing openssl-dev"
            sudo apt-get install -y libssl-dev
        else
            echo "openssl-dev installed"
        fi
    elif [ "$DISTRO" = "Fedora" ]; then
        if [ ! -f /usr/include/openssl/ssl.h ]; then
            echo "Installing openssl-dev"
            sudo dnf install -y openssl-devel
        else
            echo "openssl-dev installed"
        fi
    elif [ "$DISTRO" = "Arch" ]; then
        if [ ! -f /usr/include/openssl/ssl.h ]; then
            echo "Installing openssl-dev"
            sudo pacman -S --noconfirm openssl
        else
            echo "openssl installed"
        fi
    else
        echo "OS not supported"
        exit 1
    fi
elif [ "$OS" = "OSX" ]; then
    if [ ! -d /usr/local/include/openssl/ ]; then
        echo "Installing openssl"
        brew install openssl
    else
        echo "openssl-dev installed"
    fi
else
    echo "OS not supported"
    exit 1
fi

# Check for zlib
if [ ! -f /usr/lib/libz.so ]; then
    if [ "$OS" = "Linux" ]; then
        if [ "$DISTRO" = "Ubuntu" ]; then
            echo "Installing zlib"
            sudo apt-get install -y zlib1g-dev
        elif [ "$DISTRO" = "Fedora" ]; then
            echo "Installing zlib"
            sudo dnf install -y zlib-devel
        elif [ "$DISTRO" = "Arch" ]; then
            echo "Installing zlib"
            sudo pacman -S --noconfirm zlib
        else
            echo "OS not supported"
            exit 1
        fi
    elif [ "$OS" = "OSX" ]; then
        if [ ! -f /usr/local/lib/libz3.dylib ]; then
            echo "Installing zlib"
            brew install zlib
        else
            echo "zlib installed"
        fi
    else
        echo "OS not supported"
        exit 1
    fi
else
    echo "zlib installed"
fi

# check for bz2
if [ ! -f /usr/lib/libbz2.so ]; then
    if [ "$OS" = "Linux" ]; then
        if [ "$DISTRO" = "Ubuntu" ]; then
            echo "Installing libbz2"
            sudo apt-get install -y libbz2-dev
        elif [ "$DISTRO" = "Fedora" ]; then
            echo "Installing libbz2"
            sudo dnf install -y bzip2-devel
        elif [ "$DISTRO" = "Arch" ]; then
            echo "Installing libbz2"
            sudo pacman -S --noconfirm bzip2
        else
            echo "OS not supported"
            exit 1
        fi
    elif [ "$OS" = "OSX" ]; then
        if [ ! -f /usr/bin/bzip2 ]; then
            echo "Installing libbz2"
            brew install bzip2
        else
            echo "libbz2 installed"
        fi
    else
        echo "OS not supported"
        exit 1
    fi
else
    echo "libbz2 installed"
fi

# Check for brotli
if [ ! -f /usr/lib/libbrotlidec.so ]; then
    if [ "$OS" = "Linux" ]; then
        if [ "$DISTRO" = "Ubuntu" ]; then
            echo "Installing brotli"
            sudo apt-get install -y libbrotli-dev
        elif [ "$DISTRO" = "Fedora" ]; then
            echo "Installing brotli"
            sudo dnf install -y brotli-devel
        elif [ "$DISTRO" = "Arch" ]; then
            echo "Installing brotli"
            sudo pacman -S --noconfirm brotli
        else
            echo "OS not supported"
            exit 1
        fi
    elif [ "$OS" = "OSX" ]; then
        if [ ! -f /usr/local/lib/libbrotlidec.dylib ]; then
            echo "Installing brotli"
            brew install brotli
        else
            echo "brotli installed"
        fi
    else
        echo "OS not supported"
        exit 1
    fi
else
    echo "brotli installed"
fi

# Check for Build-essential
if [ "$OS" = "Linux" ]; then
    if [ "$DISTRO" = "Ubuntu" ]; then
        if [ ! -f /usr/bin/g++ ]; then
            echo "Installing build-essential"
            sudo apt-get install -y build-essential
        else
            echo "build-essential installed"
        fi
    elif [ "$DISTRO" = "Fedora" ]; then
        if [ ! -f /usr/bin/g++ ]; then
            echo "Installing GCC"
            sudo dnf install -y gcc-c++
        else
            echo "GCC installed"
        fi
    elif [ "$DISTRO" = "Arch" ]; then
        if [ ! -f /usr/bin/g++ ]; then
            echo "Installing Dev Tools"
            sudo pacman -S --noconfirm base-devel
        else
            echo "Dev Tools installed"
        fi
    else
        echo "OS not supported"
        exit 1
    fi
elif [ "$OS" = "OSX" ]; then
    if [ ! -f /usr/bin/g++ ]; then
        echo "Installing Build Tools"
        xcode-select --install
    else
        echo "Build Tools installed"
    fi
else
    echo "OS not supported"
    exit 1
fi

# Check for lib Link
if [ ! -f /usr/local/lib/liblink.so ]; then
    echo "Installing liblink.so"
    git clone https://github.com/N11Software/Link.git
    cd Link
    ./build.sh;./build.sh install
    cd ..
    rm -r Link
else
    echo "Link installed"
fi

# Check for lib OQS
if [ ! -d /usr/local/include/oqs/ ]; then
    if [ "$OS" = "Linux" ]; then
        if [ "$DISTRO" = "Ubuntu" ]; then
            echo "Installing OQS"
            sudo apt install astyle cmake gcc ninja-build libssl-dev python3-pytest python3-pytest-xdist unzip xsltproc doxygen graphviz python3-yaml valgrind -y
            git clone -b main https://github.com/open-quantum-safe/liboqs.git
            cd liboqs
            mkdir build && cd build
            cmake -GNinja ..
            ninja
            sudo ninja install
            cd ../../
            rm -r liboqs
        elif [ "$DISTRO" = "Fedora" ]; then
            echo "Installing OQS"
            sudo dnf install -y oqs-devel
        elif [ "$DISTRO" = "Arch" ]; then
            echo "Installing OQS"
            sudo pacman -S --noconfirm oqs
        else
            echo "OS not supported"
            exit 1
        fi
    elif [ "$OS" = "OSX" ]; then
        if [ ! -f /usr/local/lib/liboqs.a ]; then
            echo "Installing OQS"
            brew install liboqs
        else
            echo "brotli installed"
        fi
    else
        echo "OS not supported"
        exit 1
    fi
else
    echo "OQS installed"
fi

# Check for CryptoPP
if [ ! -f /usr/lib/libcryptopp.a ]; then
    if [ "$OS" = "Linux" ]; then
        if [ "$DISTRO" = "Ubuntu" ]; then
            echo "Installing CryptoPP"
            sudo apt-get install -y libcrypto++-dev
        elif [ "$DISTRO" = "Fedora" ]; then
            echo "Installing CryptoPP"
            sudo dnf install -y cryptopp-devel
        elif [ "$DISTRO" = "Arch" ]; then
            echo "Installing CryptoPP"
            sudo pacman -S --noconfirm cryptopp
        else
            echo "OS not supported"
            exit 1
        fi
    elif [ "$OS" = "OSX" ]; then
        if [ ! -f /usr/local/lib/libcryptopp.a ]; then
            echo "Installing CryptoPP"
            brew install cryptopp
        else
            echo "cryptopp installed"
        fi
    else
        echo "OS not supported"
        exit 1
    fi
else
    echo "cryptopp installed"
fi

# Check if `which surreal` exists
surreal=$(which surreal)
if [ -z "$surreal" ]; then
    echo "Installing surreal"
    curl -sSf https://install.surrealdb.com | sh
else
    echo "surreal installed"
fi

# Create DB and start surreal
surreal start --user root --pass root rocksdb:n11.db > /dev/null 2>&1 & 
# Connect to it and run "DEFINE NAMESPACE N11;"
sleep 2;
echo "DEFINE NAMESPACE N11;USE NS N11;DEFINE DATABASE N11;" | surreal sql --user root --pass root > /dev/null 2>&1
# Surreal is now ready to use
# Kill surreal
killall surreal

# Build N11
echo "Building N11"
./build.sh

# Install N11 to /usr/local/bin
echo "Installing N11"
sudo cp N11 /usr/local/bin/N11

# Create daemon if linux
if [ "$OS" = "Linux" ]; then
    sudo useradd -G sudo n11
    sudo chown n11:n11 /var/lib/n11

    # Create daemon if linux
    if [ ! -f /etc/systemd/system/n11.service ]; then
        sudo cp n11.service /etc/systemd/system/n11.service
        sudo systemctl enable n11
        sudo systemctl start n11
    fi

    # Create daemon if linux
    if [ ! -f /etc/systemd/system/surreal.service ]; then
        sudo cp surreal.service /etc/systemd/system/surreal.service
        sudo systemctl enable surreal
        sudo systemctl start surreal
    fi
fi
