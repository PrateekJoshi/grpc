FROM ubuntu:latest

#Stop apt update asking for time zone in apt update
ARG DEBIAN_FRONTEND=noninteractive

RUN export MY_INSTALL_DIR=${HOME}/.local && \
mkdir -p ${MY_INSTALL_DIR} && \
export PATH="${PATH}:${MY_INSTALL_DIR}/bin" && \
apt update && \
apt install -y cmake wget vim build-essential autoconf libtool pkg-config git libssl-dev && \
wget -q -O cmake-linux.sh https://github.com/Kitware/CMake/releases/download/v3.17.0/cmake-3.17.0-Linux-x86_64.sh && \
sh cmake-linux.sh -- --skip-license --prefix=${MY_INSTALL_DIR} && \
rm cmake-linux.sh && \
git clone --depth 1 -b v1.34.0 https://github.com/grpc/grpc && \
cd grpc && \
git submodule update --init


RUN export MY_INSTALL_DIR=${HOME}/.local && \
cd grpc && \
mkdir -p cmake/build && \
cd cmake/build && \
cmake -DBUILD_SHARED_LIBS=ON -DgRPC_INSTALL=ON -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DCMAKE_BUILD_TYPE=Release ../.. && \
make -j && \
make install && \
cd -

#Persist env variable
RUN export MY_INSTALL_DIR=${HOME}/.local && \
echo "export MY_INSTALL_DIR=${HOME}/.local" >> /root/.bashrc && \
echo "export PATH=${PATH}:${MY_INSTALL_DIR}/bin" >> /root/.bashrc && \
echo "export LD_LIBRARY_PATH=${MY_INSTALL_DIR}/lib" >> /root/.bashrc && \
echo "export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:${MY_INSTALL_DIR}/lib/pkgconfig:${MY_INSTALL_DIR}/lib64/pkgconfig" >> /root/.bashrc

ENTRYPOINT [ "bash" ]






