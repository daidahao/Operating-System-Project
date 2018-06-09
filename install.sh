# download & decompress bochs
wget -O bochs-2.6.7.tar.gz https://sourceforge.net/projects/bochs/files/bochs/2.6.7/bochs-2.6.7.tar.gz/download
tar -zxvf bochs-2.6.7.tar.gz
# install dependencies
sudo apt-get install build-essential
sudo apt-get install xorg-dev
sudo apt-get install bison
sudo apt-get install libgtk2.0-dev
# install bochs
cd bochs-2.6.7
./configure --with-nogui --enable-gdb-stub
make
sudo make install
# test bochs
# bochs --version
# set $PATH properly
cd ..
export PATH="$PATH:$(readlink -f ./src/utils)"