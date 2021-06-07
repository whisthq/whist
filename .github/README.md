# Fractal GitHub Utilities

This folder contains the various utilities that we have in placeto interface with GitHub, our version-control, code-hosting, and continous integration provider. The `workflows` and `actions` folders are for continuous integration, while the other folders and files are for general work management of dependencies and work between engineers on the repository.

## GitHub Pull Request Templates

The `PULL_REQUEST_TEMPLATE` directory contains our extra GitHub pull request templates, which we use for our different types of pull requests, in addition to our regular PR template (i.e. promotion vs regular feature/bug development).

Unfortunately, as of June 2021, GitHub does not have a UI to auto-populate pull requests by selecting amongst multiple PR templates when creating a PR from the GitHub website (unnlike for GitHub Issues, which have this functionality already). It does, however, support auto-populating a PR template by modifying the PR creation URL once on the pull request creation page.

As such, we keep our default development pull request template in this folder as `PULL_REQUEST_TEMPLATE.md`, which gets auto-populated for every pull request created. To swap to a different PR template from this subfolder instead, simply modify the PR URL to append `expand=1&template=<template-name.md>`. In the case of a promotion from `dev` to `staging`, your URL should be:

```
https://github.com/fractal/fractal/compare/staging...dev?expand=1&template=promotion.md
```
