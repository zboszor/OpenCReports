#!/bin/bash

autoreconf -vif -Wall
./configure --prefix=/usr --libdir=/usr/lib64
