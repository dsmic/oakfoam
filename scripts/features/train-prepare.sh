#!/bin/bash

set -eu

sed "s/.* \(0x[0-9a-fA-F]*\)/pattern3x3:\1 1.0/" | sort

