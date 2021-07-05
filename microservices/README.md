# Fractal Microservices

This repository contains Fractal's microservices. These are self-contained, discrete services that perform certain tasks.

Currently, it only contains Auth0 Rules and related scripts. See [auth0/README.md](auth0/README.md) for more details specific to Auth0 Rules.

## Folder organization

It is intended that the subdirectories of this folder each correspond to a "deploy target". For example, the Auth0 Rules are deployed directly to Auth0 servers. If, for example, in the future we write functions intended to be deployed to AWS Lambda, those would go in a different folder.

Consequently, each subdirectory contains both the code for each group of microservices and the associated scripts to build and deploy them.
