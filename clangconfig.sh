#!/bin/bash

LD=${1:-bfd}
./prodconfig.sh CC=clang LDFLAGS=-fuse-ld=${LD}
