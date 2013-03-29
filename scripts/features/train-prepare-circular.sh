#!/bin/bash

set -eu

sed "s/.* \([0-9a-fA-F:]*\)/circpatt:\1 1.0/" | sort

