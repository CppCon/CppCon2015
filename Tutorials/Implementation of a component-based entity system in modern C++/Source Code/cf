#!/bin/bash

MYFLAGS="-std=c++1z -pthread -Wall -Wextra -Wpedantic -Wundef -Wshadow -Wno-missing-field-initializers \
 -Wpointer-arith -Wcast-align -Wwrite-strings -Wno-unreachable-code -Wnon-virtual-dtor -Woverloaded-virtual \
 -Ofast -ffast-math -lsfml-system -lsfml-graphics -lsfml-window -DNDEBUG -DSSVUT_DISABLE -g3"

clang++ -o /tmp/$1 $MYFLAGS ./$1.cpp && /tmp/$1