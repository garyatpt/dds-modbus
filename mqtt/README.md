# mqtt samples

### Install libmosquitto

```bash
# Install mosquitto library
sudo apt-get install build-essential 
sudo apt-get install libssl-dev libwrap0-dev libc-ares-dev uuid-dev
wget http://mosquitto.org/files/source/mosquitto-1.4.8.tar.gz
tar zxvf mosquitto-1.4.8.tar.gz
make all
sudo make install
sudo ldconfig
```

### Build

```bash
./build.sh
```
