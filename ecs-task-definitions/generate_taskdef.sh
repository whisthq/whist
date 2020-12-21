#!/bin/bash

cat fractal-base.json | jq '.family |= "'$1' > $1.json
