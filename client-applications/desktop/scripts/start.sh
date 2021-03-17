#!/usr/bin/env bash

concurrently \
    "snowpack dev" \
    "electron build/dist/main"
