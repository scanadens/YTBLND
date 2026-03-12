#!/bin/bash

# builds src code using the cmake config. must be run withint the project root (group45) to work correctly

cmake -S . -B build && cmake --build build
