# SeamlessDB

SeamlessDB is a cloud-native database prototype for academic purposes, supporting the separation of computation, state, and storage into three layers.

We implemente our designs mentioned in the paper(SeamlessDB: Efficient Failover Handling in Cloud-native Databases with Breakpoint Resumption) based on this system.

## Pre-requisites to Build
- Compiler: g++ 11.4.0
- Operating System: Ubuntu 22.04
- Hardware: 
    - Mellanox InfiniBand NIC (e.g., ConnectX-5) that supports RDMA
    - Mellanox InfiniBand Switch
- brpc
```bash
  sudo apt-get install -y git g++ make libssl-dev libgflags-dev libprotobuf-dev libprotoc-dev protobuf-compiler libleveldb-dev
  cd src/thirdparty/brpc
  cmake -B build && cmake --build build -j6
```
- RocksDB: v6.6.4

## How to build
```bash
# build storage pool, state pool, rw/ro server, proxy
bash build.sh
# build seamless client
cd seamless_client
mkdir build && cd build
cmake .. && cmake --build .
```

## How to run SeamlessDB
Before running the program, you need to fill in the configuration file first. All configuration files are in [here](./src/config/) and we provide a default configuration. You have to replace the 'ip' and 'port' values. Moreover, you need to modify the 'ip' and 'port' settings for the storage pool in [rw_server.cpp](./src/compute_pool/rw_server.cpp).
```bash
cd build
# launch stoarge pool
./bin/storage_pool
# launch state_pool
./bin/state_pool
# launch rw/ro_server
./bin/rw_server active/backup rw/ro
# launch client
cd seamless_client
./build/bin/seamless_client
# launch proxy(be sure that storage pool, state pool and rw server have been launched)
./bin/proxy rw/ro
```

## How to run experiments
We use scripts to run experiments. Before run experiments, you have to make sure that the storage pool, state pool and rw server have been launched.

```bash
./bin/storage_pool
./bin/state_pool
./bin/rw_server active ro
./bin/rw_server backup ro
cd scripts
# You have to update the normal execution latency in run_test.sh
bash run_test 40  # represents that the active_no will be killed at 40% failover point
```

run tpcc-experiments:
```bash
./bin/storage_pool
./bin/state_pool
./bin/rw_server active rw
./bin/rw_server backup rw
cd scripts
bash TPC-C_test.sh
```
