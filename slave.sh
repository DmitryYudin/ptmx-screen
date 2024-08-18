#!/usr/bin/env bash

set -eu

pts=$(</tmp/master.pts)
screen -S ctrl -d -m $pts
screen -S ctrl -X stuff "hello^M"
screen -S ctrl -d -r
