# Fractal Application Streaming

This repository contains the end-to-end code for the Fractal Application Streaming product, following a [Monorepo](https://en.wikipedia.org/wiki/Monorepo) structure.

## Table of Contents

- [Introduction](#introduction)
  - [Repository Structure](#repository-structure)
- [Development](#introduction)
  - [Branch Conventions](#branches-convention)
    - [`prod` is for releases only; `staging` is "almost `prod`"](#prod-is-for-releases-only-staging-is-almost-prod)
    - [`dev` is for development](#dev-is-for-development)
    - [Your branch is yours; our branches are _ours_](#your-branch-is-yours-our-branches-are-ours)
  - [Git Best Practices](#git-best-practices)
    - [On feature branches, rebase onto dev instead of merging dev into your branch](#on-feature-branches-rebase-onto-dev-instead-of-merging-dev-into-your-branch)
    - [Options to mainline your PRs](#options-to-mainline-your-prs)
    - [On commit logs](#on-commit-logs)
  - [Hotfixes (i.e. production is on fire)](#hotfixes-ie-prod-is-on-fire)
- [Publishing](#publishing)
- [Styling](#styling)
- [Appendix](#appendix)
  - [Useful Monorepo Git Tricks](#useful-monorepo-git-tricks)
  - [Example of Bad Commit History](#example-of-bad-commit-history)

## Introduction

Application Streaming is Fractal's core service. It consists of running an application in a GPU-enabled Linux Docker container (called a "mandelbox" here at Fractal) on a powerful host server in the cloud (currently an EC2 instance) and streaming the audio and video output of the mandelbox to a user device.

At a high-level, Fractal works the following way:

- Users download the Fractal Electron application for their OS and log in to launch the streamed application, like Chrome.
- The login and launch processes are REST API requests to the Fractal webserver, which is responsible for picking an EC2 instance with available space (or creating a new one if there aren't any) and passing the request over to the Host Service on the chosen EC2 instance, which is responsible for allocating the user to a container (mandelbox) on that EC2 instance.
  - If all existing EC2 instances are at maxed capacity of mandelboxes running on them, the webserver will spin up a new EC2 instance based off of a base operating system image (AMI) that was configured using the `/host-setup` scripts and has the `/host-service` preinstalled, and tell the `/host-service` that this user needs a mandelbox.
- If there is available capacity on existing EC2 instances, or after a new EC2 instance has been spun up, the `/host-service` running on the chosen EC2 instance will allocate the user to a mandelbox. The Fractal protocol server inside this mandelbox image will be started and will notify the webserver/Electron application that it is ready to stream, and the Electron application will launch the Fractal protocol client, which will start the stream.
  - The mandelbox images are based off of `/mandelboxes` and are pre-built and stored in GitHub Container Registry, where our Host Servicepulls the images from.

### Repository Structure

The Fractal monorepository contains 8 Fractal subrepositories:

| Subrepository       | Description                                                                                           |
| ------------------- | ----------------------------------------------------------------------------------------------------- |
| client-applications | The client-side Electron-based application users download and use to launch a streamed application.   |
| core-ts             | The Fractal internal TypeScript library of utilities and reusable components.                         |
| host-service        | The Fractal service which runs on EC2 instance hosts and orchestrates mandelbox management.           |
| host-setup          | The scripts to setup an EC2 innstance into a Fractal-optimized host ready to run Fractal mandelboxes. |
| mandelboxes         | The Dockerfiles defining the mandelbox images and helper scripts for the applications we stream.      |
| microservices       | Code we deploy to other platforms, like Auth0.                                                        |
| protocol            | The streaming technology API, both client and server, for streaming applications to users.            |
| webserver           | The REST API for managing our AWS infrastructure, supporting our front-end, and connecting the two.   |
| website             | The website hosted at `fractal.co`.                                                                   |

For more in-depth explanations of each subrepository, see that subrepository's README.

Note that there is also additional documentation for some subrepos (and other Fractal repos) at [docs.fractal.co](https://docs.fractal.co):

- [docs.fractal.co/protocol](https://docs.fractal.co/protocol)
- [docs.fractal.co/webserver](https://docs.fractal.co/webserver)
- [docs.fractal.co/SDL](https://docs.fractal.co/SDL)
- [docs.fractal.co/FFmpeg](https://docs.fractal.co/FFmpeg)

## Development

To get started with development, clone this repository and navigate to a specific subrepository. While it is likely that you will work on a feature that touches multiple subrepositories, each subrepository has its own development and styling standards which you should follow, in addition to the usual [Fractal Engineering Guidelines](https://www.notion.so/tryfractal/Engineering-Guidelines-d8a1d5ff06074ddeb8e5510b4412033b).

To avoid pushing code that does not follow our coding guidelines, we recommend you install pre-commit hooks by running `pip install pre-commit`, followed by `pre-commit install` in the top-level directory. This will install the linting rules specified in `.pre-commit-config.yaml` and prevent you from pushing if your code is not linted.

**NOTE**: Also, once you clone the repo run `git config pull.ff only` inside the repo. This will help prevent pushing unnecessary merge commits (see below).

### Branch Conventions

#### `prod` is for Releases only; `staging` is "almost `prod`"

At Fractal, we maintain a `prod` branch for releases only, and auto-tag every commit on `prod` with a release tag [[TODO]](https://github.com/fractal/fractal/issues/1139).

We also maintain a `staging` branch for release candidates. Therefore, `staging` will always be ahead of `prod`, except after a production release, when they will be even.

When we approach a release milestone, we:

1. Fast-forward `staging` to match `dev`. This signifies a feature freeze for that release, and we only perform critical bug fixes.
2. Perform extensive, end-to-end testing (in practice, this is accomplished by us dogfooding our own product).
3. We hotfix bugs by forking a branch off `staging` and then fast-forward merging it into `staging` (having the same effect as committing directly to `staging`, but with a PR for review and CI).
4. To release, we fast-forward `prod` to match `staging` and create a tagged release commit. This triggers our auto-deployment workflows, and we push to production.
5. If changes were made to `staging` before release, we merge `staging` into `dev` as well (one of the rare instances that calls for a merge commit, to prevent rewriting history on `dev`).

Note that we always try to maintain the invariant that `prod` can be fast-forwarded to match `staging`, and `staging` can be fast-forwarded to match `dev`. The only exception is when we have to make hotfixes to `staging` or `prod`, and we always merge these changes back.

### `dev` is for development

We do all feature development and most bug-fixing on feature branches that are forked off `dev`.

**Feature branches are named as follows:**

- Project-specific feature branches (i.e. those restricted to a single top-level directory in the repo) are named in the form `<author>/<project>/<feature-verb>`. For instance, a branch authored by Savvy and designed to make the Host Service llama-themed would be called `djsavvy/host-service/add-llama-theme`.
- Broader feature branches that touch multiple projects should be named in the form `<author>/<feature-verb>`.

Note that the last part of the branch name should almost always be an **action** instead of an object (i.e. `add-llama-theme` instead of `llama-theme`). This makes commit logs much easier to parse.

#### Your branch is yours; our branches are _ours_

In general, feel free to rebase your personal feature branches, amend commit messages/commits for clarify, force-push, or otherwise rewrite git history on them.

However, in the less common case where multiple people are working on a single feature together, they belong on a single feature branch with consistent history, typically with branch name `project/[feature]`. The usual rules of not rewriting published commits apply there. Before making a PR into dev, the point person for that feature should let everyone else know to stop working on that branch and rebase the feature onto dev.

### Git Best Practices

#### On feature branches, rebase onto dev instead of merging dev into your branch

When making a PR for a feature branch into `dev`, you'll usually find that there's been changes to `dev` since you last branched off. Both the frequency and scope of this situation are magnified by the complexity of a monorepo.

The problem is that you need to catch your branch up to `dev` before your PR can be merged.

One tempting solution is to merge `dev` into your feature branch. **DO NOT do this.**

If you create a pull request for a branch based on an outdated version of `dev`, Github provides you with a shiny button to automatically merge `dev` into your feature branch. **DO NOT use this button.** This leads to git histories that look like [this](#an-example-of-bad-commit-history).

**Here's what to do instead:**

```bash
git checkout dev
git pull --rebase
git checkout feature-branch
git rebase dev
```

Most of the time, that rebase will work silently, since most changes to dev should not affect the feature branch. If there are any conflicts, that means that someone else is working on the same files as you, and you'll have to manually resolve the conflicts, just as with a merge. If you're having problems rebasing, try `git rebase -r dev` (which handles merge commits differently), then come ask an org admin for help.

#### Options to mainline your PRs

PRs should mostly be fast-forwarded onto dev, which gives the illusion later of having committed directly onto dev, simplifying the commit log. This can be done on the Github PR page by selecting the "Rebase and merge" option:

![Picture of "Rebase and merge" option](https://ntsim.uk/static/77d197658c4f050ba0a4080747505d8e/3346a/github-rebase-and-merge.png)

**Never** make a merge commit. If you want cleaner git history, rewrite and force-push your own branch (with carefully-applied `git rebase`), then merge it into dev, or use the "Squash and Merge" option. This is the recommended option if you have too many commits on a given branch, or some "oops" commits that don't need to make it onto the permanent history in `dev`.

### On commit logs

Clear commit logs are _far_ more important in a monorepo than when spread over several multirepos. There's so many more moving parts, so it's increasingly important to be able to filter a list of commits to what's actually relevant. Tooling can help with this (e.g. `git log -- <subdirectory or file>`), but in the end we are still limited by the quality of our own commit logs.

Like branch names, commit messages should be descriptive and contain enough context that they make sense _on their own_. Commit messages like "fix", "works", or "oops" can be annoyingly unhelpful. Even something as innocuous-sounding as "staging merge" can be confusing --- are we merging into `staging`, or merging `staging` into another branch? What other branch(es) are involved?

Some good rules of thumb:

- Commit messages, like branch suffixes, should be _actions_ instead of objects. If you're parsing a `git log` from a year ago, trying to understand some design decisions, what's more helpful: `"network connections"`, or `"Cut down on extra network connections in handshake"`?
- Add as much context as you possibly can. Imagine you're digging though a commit history, trying to figure out which one may have introduced a critical bug into production. Which helps more: `"increase timeout"`, or `"Increase service A timeout waiting for service B to complete handshake"`?
- Forget the "50 characters max" or whatever artificially-low number Github and Linus Torvalds start complaining at. It's far more useful to have a clear, contextualized commit message that spills over than an artificially terse mess. In the first case, future you (or a colleague) simply has to click once to show the rest of the commit message on Github, or maybe run `git log` in her terminal. In the second case, she's simply left wondering what you meant.

In general, if you find yourself staring at a historic commit log, it's probably because something's gone wrong, and you're trying to piece together what happened. Or maybe you're trying to understand why some code is organized the way it is, and seeing how it evolved can lead you to that understanding. Either way, you've probably lost the mental context you had when writing the code (or you never even had it), so the quality of breadcrumbs (i.e. commit messages) left in the commit log correlates directly with the quality of your well-being in that situation.

### Hotfixes (i.e. prod is on fire)

Eventually, but hopefully rarely, production will be on fire, and we will need to deploy a quick fix ASAP.

Here's the workflow:

1. Fork a branch with name starting with `hotfix/` off `prod`. (This is important, as the CI will prevent merging in a non-`staging` branch that doesn't meet this criterion (TODO).)
2. Make the fix on the hotfix branch, and PR it directly to `prod`.
3. Get as many eyeballs on the PR as possible, and approve it if it looks good.
4. Wait for all CI checks and tests to pass. This is important --- we will **not** skip CI and force a hotfix PR through to prod. The CI is our last line of defense, and the time saved by "skipping it" is not worth the increased chance of pushing another bad commit to production.
5. Merge the hotfix into `prod`.
6. Once the fix is confirmed to work, merge `prod` back into `staging`, and `staging` back into `dev`.
7. Write a regression test to make sure the same issue never occurs again, and add it to CI.

### GitHub Pull Request Templates

The `PULL_REQUEST_TEMPLATE` directory contains our extra GitHub pull request templates, which we use for our different types of pull requests, in addition to our regular PR template (i.e. promotion vs regular feature/bug development).

Unfortunately, as of June 2021, GitHub does not have a UI to auto-populate pull requests by selecting amongst multiple PR templates when creating a PR from the GitHub website (unnlike for GitHub Issues, which have this functionality already). It does, however, support auto-populating a PR template by modifying the PR creation URL once on the pull request creation page.

As such, we keep our default development pull request template in this folder as `PULL_REQUEST_TEMPLATE.md`, which gets auto-populated for every pull request created. To swap to a different PR template from this subfolder instead, simply modify the PR URL to append `expand=1&template=<template-name.md>`. In the case of a promotion from `dev` to `staging`, your URL should be:

```
https://github.com/fractal/fractal/compare/staging...dev?expand=1&template=promotion.md
```

## Publishing

We have developed a complex continuous deployment pipeline via GitHub Actions, which enables us to automatically deploy all subrepositories of this monorepositories in the right order when pushing to `prod`, `dev`, and `staging`. See `.github/workflows/fractal-build-and-deploy.yml` to see how we deploy, which AWS regions and which streamed applications get deployed, and more. If something goes wrong in the continuous deployment pipeline and a specific job fails, it is possible to manually trigger a specific job of the `fractal-publish-build.yml` workflow via the GitHub Actions console.

## Styling

Each subfolder in this monorepository is its own project with its dedicated style, which you must follow. All work done on this monorepository must follow the [Documentation & Code Standards](https://www.notion.so/tryfractal/Documentation-Code-Standards-54f2d68a37824742b8feb6303359a597) and the [Engineering Guidelines](https://www.notion.so/tryfractal/Engineering-Guidelines-d8a1d5ff06074ddeb8e5510b4412033b).

## Appendix

### Useful Monorepo git Tricks

- Viewing a log of only the commits affecting a given file or subdirectory: `git log -- <path>`
- Making sure you don't accidentally introduce merge commits from a `git pull`: `git config pull.ff only`

### Example of Bad Commit History

This is an example of a bad commit history, which exhibits a phenomenon caused by allowing merge commits into feature branches. There's exactly two "useful" commits in the mess below. The rest are merge commits from `dev` into a feature branch, which should be avoided.

[Back to Text](#while-on-feature-branches-git-rebase-dev-is-your-friend-git-merge-dev-is-not)

```
| | | | |/| | | | | |
| | | | | | | * | | |   8bd2acce - Merge branch 'dev' into feature_branch (8 weeks ago) <developer2>
| | | | | | | |\ \ \ \
| | | | | | | | | |_|/
| | | | | | | | |/| |
| | | | | | | * | | |   ee0961e5 - Merge branch 'dev' into feature_branch (8 weeks ago) <developer1>
| | | | | | | |\ \ \ \
| | | | | | | * \ \ \ \   c4301083 - Merge branch 'dev' into feature_branch (8 weeks ago) <developer2>
| | | | | | | |\ \ \ \ \
| | | | | | | * \ \ \ \ \   4c521824 - Merge branch 'dev' into feature_branch (10 weeks ago) <developer1>
| | | | | | | |\ \ \ \ \ \
| | | | | | | * \ \ \ \ \ \   a882b67c - Merge branch 'dev' into feature_branch (10 weeks ago) <developer1>
| | | | | | | |\ \ \ \ \ \ \
| | | | | | | * | | | | | | | 34ca8f9b - clang format (10 weeks ago) <developer1>
| | | | | | | * | | | | | | |   6e1501d6 - Merge branch 'dev' into feature_branch (10 weeks ago) <developer1>
| | | | | | | |\ \ \ \ \ \ \ \
| | | | | | | * \ \ \ \ \ \ \ \   1fc9f76e - Merge branch 'dev' into feature_branch (10 weeks ago) <developer2>
| | | | | | | |\ \ \ \ \ \ \ \ \
| | | | | | | * \ \ \ \ \ \ \ \ \   d3fabb5e - Merge branch 'dev' into feature_branch (2 months ago) <developer1>
| | | | | | | |\ \ \ \ \ \ \ \ \ \
| | | | | | | * \ \ \ \ \ \ \ \ \ \   4458e26b - Merge branch 'dev' into feature_branch (3 months ago) <developer1>
| | | | | | | |\ \ \ \ \ \ \ \ \ \ \
| | | | | | | * \ \ \ \ \ \ \ \ \ \ \   1de676f6 - Merge branch 'dev' into feature_branch (3 months ago) <developer3>
| | | | | | | |\ \ \ \ \ \ \ \ \ \ \ \
| | | | | | | * | | | | | | | | | | | | 7b478fd4 - a commit for feature_branch (3 months ago) <developer3>
```
