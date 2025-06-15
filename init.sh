#!/bin/sh
sudo apt-get install libglu1-mesa-dev libegl1-mesa-dev libgl1-mesa-dev libwayland-dev || exit 1
git submodule update --init --recursive
