#!/bin/bash

MYFLAGS="-std=c++1z -pthread -Wall -Wextra -Wpedantic -Wundef \
-Wno-missing-field-initializers -Wpointer-arith -Wcast-align -Wwrite-strings \
-Wno-unreachable-code -Wnon-virtual-dtor -Woverloaded-virtual -O0 -lsfml-system \
-lsfml-graphics -lsfml-window -DSSVUT_DISABLE -g3"

# g++ -o /tmp/$1 $MYFLAGS ./$1.cpp \
# && echo "g++ ok" \
# && clang++ -o /tmp/$1 $MYFLAGS ./$1.cpp \
# && echo "clang++ ok" \
# && /tmp/$1

clang++ -o /tmp/$1 $MYFLAGS ./$1.cpp && /tmp/$1