#!/usr/bin/env bash

yarn build && electron-builder build --publish never
