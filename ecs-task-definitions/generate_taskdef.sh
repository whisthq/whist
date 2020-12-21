#!/bin/bash

cat fractal-template.json | jq '.family |= "'$1'"' > $1.json
