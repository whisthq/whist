#!/bin/bash
openssl req  -nodes -new -x509 -subj "/C=US/ST=./L=./O=./CN=." -keyout key.pem -out cert.pem
