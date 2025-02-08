g++ Source/N11.cpp Source/Json.cpp \
    -o N11 \
    -I/usr/local/include/link/ \
    -L/usr/local/lib/ \
    -lssl -lcrypto -lcryptopp -llink -loqs \
    -pthread \
    -std=c++17 -Wno-deprecated-declarations