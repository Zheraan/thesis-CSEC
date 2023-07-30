#!/bin/bash

if [ $# -ne 2 ]
  then
    echo "Usage: generate_dockerfiles.sh <nb_masters> <nb_servers>"
    exit 1
fi

set -e

mkdir -p "dockergen"
find dockergen -path 'dockergen/[!.]*' -prune -exec rm -r -- {} +

# Start of docker-compose file
cat <<EOT > ./dockergen/docker-compose.yaml
version: "3.9"
services:
EOT

node_number=0
host_number=0
address=10
# For each master node
while [[ $node_number -lt "$1" ]]; do
  # Generate one dockerfile
  cat <<EOT > ./dockergen/raft_master_dockerfile$node_number
FROM ubuntu:latest

COPY ./cmake-build-debug/raft-master ./local
COPY ./dockergen/hostfile$host_number.txt ./local
WORKDIR ./local/

EXPOSE 35007
EXPOSE 35008
CMD ["./raft_master", "hostfile$host_number.txt"]
EOT
  # Generate a docker-compose service
  cat <<EOT >> ./dockergen/docker-compose.yaml

  raft-master$node_number:
    build:
      context: ..
      dockerfile: ./dockergen/raft_master_dockerfile$node_number
    cap_add:
      - NET_ADMIN
    networks:
      global:
        ipv6_address: 2001:0db8:0002:0000:0000:0000:0000:$address
    deploy:
      replicas: 1
EOT
  # Generate a hostfile line in all hostfiles
  k=0
  while [[ $k -lt $(($1 + $2)) ]]; do
    if [[ $k -ne $host_number ]]; then
    cat <<EOT >> ./dockergen/hostfile$k.txt
# Master node named master$node_number that is distant
M,master$node_number,D,2001:0db8:0002:0000:0000:0000:0000:$address

EOT
    else
      # If current iteration concerns the local node, write a local line
      cat <<EOT >> ./dockergen/hostfile$k.txt
# Master node named master$node_number that is local
M,master$node_number,L,2001:0db8:0002:0000:0000:0000:0000:$address

EOT
    fi

    (( k += 1 ))
  done
  (( address += 1 ))
  (( node_number += 1 ))
  (( host_number += 1 ))
done

node_number=0
# For each server node
while [[ $node_number -lt "$2" ]]; do
  # Generate one dockerfile
  cat <<EOT > ./dockergen/raft_server_dockerfile$node_number
FROM ubuntu:latest

COPY ./cmake-build-debug/raft-server ./local
COPY ./dockergen/hostfile$host_number.txt ./local
WORKDIR ./local/

EXPOSE 35007
EXPOSE 35008
CMD ["./raft_server", "hostfile$host_number.txt"]
EOT
  # Generate a docker-compose service
  cat <<EOT >> ./dockergen/docker-compose.yaml

  raft-server$node_number:
    build:
      context: ..
      dockerfile: ./dockergen/raft_server_dockerfile$node_number
    cap_add:
      - NET_ADMIN
    networks:
      global:
        ipv6_address: 2001:0db8:0002:0000:0000:0000:0000:$address
    deploy:
      replicas: 1
EOT
  # Generate a hostfile line in all hostfiles
  k=0
  while [[ $k -lt $(($1 + $2)) ]]; do
    if [[ $k -ne $host_number ]]; then
    cat <<EOT >> ./dockergen/hostfile$k.txt
# Server node named server$node_number that is distant
S,server$node_number,D,2001:0db8:0002:0000:0000:0000:0000:$address

EOT
    else
      # If current iteration concerns the local node, write a local line
      cat <<EOT >> ./dockergen/hostfile$k.txt
# Server node named server$node_number that is local
S,server$node_number,L,2001:0db8:0002:0000:0000:0000:0000:$address

EOT
    fi
    (( k += 1 ))
  done
  (( address += 1 ))
  (( node_number += 1 ))
  (( host_number += 1 ))
done

# End of docker-compose file
cat <<EOT >> ./dockergen/docker-compose.yaml

networks:
  global:
    driver: bridge
    enable_ipv6: true
    ipam:
      driver: default
      config:
        - subnet: 2001:db8:2::/64
          gateway: 2001:0db8:0002:0000:0000:0000:0000:0004
EOT

exit 0