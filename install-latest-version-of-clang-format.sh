echo "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye main" | sudo tee /etc/apt/sources.list.d/llvm.list
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
sudo apt-get update

