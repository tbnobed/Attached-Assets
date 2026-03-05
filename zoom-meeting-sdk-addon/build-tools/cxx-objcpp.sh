#!/bin/bash
for arg in "$@"; do
  if [ "$arg" = "-c" ]; then
    exec clang++ -x objective-c++ "$@"
  fi
done
exec clang++ "$@"
