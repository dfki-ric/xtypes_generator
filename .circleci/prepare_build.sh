#!/bin/bash
export DEBIAN_FRONTEND=noninteractive

# Update and install dependencies
apt-get update
apt-get install -y build-essential git cmake pkg-config  libncurses5-dev python3 python3-jinja2 python3-yaml python3-pip libboost-all-dev
