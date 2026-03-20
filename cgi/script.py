#!/usr/bin/env python3

import os
import sys

print("Hello from Python!")

print("\nArguments:")
for i, arg in enumerate(sys.argv):
    print(f"argv[{i}] = {arg}")

print("\nEnvironment variables:")
for key, value in os.environ.items():
    print(f"{key}={value}")