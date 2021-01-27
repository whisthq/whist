# Fractal Application Streaming

This repository contains the end-to-end code for the Fractal Application Streaming product, following a [Monorepo](https://en.wikipedia.org/wiki/Monorepo) structure.

## Table of Contents

- [Introduction](#introduction)
  - [Repository Structure](#repository-structure)
- [Development](#introduction)
- [Publishing](#publishing)
- [Styling](#styling)
- [Appendix](#appendix)
  - [Useful Monorepo git Tricks](#useful-monorepo-git-tricks)
  - [Example of Bad Commit History](#example-of-bad-commit-history)

# ===

## Introduction

Application Streaming is Fractal's core service. It consists in running an application in a GPU-enabled Linux Docker container on a powerful host server in the cloud, currently EC2 instances, and streaming the content of the container to a user device.

At a high-level, Fractal works the following way:






- Users download the Fractal Electron application for their OS

- 





First, the user downloads the client-applications Electron App. They log-in, and launch a Blender container for instance.
- The log-in and launch process are REST API requests sent to the main-webserver.
- The main-webserver will receive the launch request and will proceed to send an ecs-task-definitions task definition to AWS ECS.
- This will either (A) Use an EC2 Instance that is already spun-up, or (B) This will spin up a new EC2 Instance, and then run the ecs-host-setup scripts on it. ecs-host-setup will install dependencies, and install ecs-host-service.
- After (A) or (B) happens, an EC2 Instance will now be available one way or another, with potentially other Docker containers already running on it.
- The ecs-task-definitions task definition will then spin up an additional Docker container on that chosen EC2 Instance using a Dockerfile from container-images, specifically choosing the Blender Dockerfile (Or whichever application they happened to choose).
- This container-images Dockerfile will install dependencies, including the Server protocol executable, then install and run Blender, and then proceed to execute the Server protocol executable.
- The client-application Electron App will then execute the Client protocol executable and pass in the IP address of the Server as received from main-webserver. A window will then open, giving the user a low-latency 60 FPS Blender experience.

For more in-depth explanations of each subrepo, simply peruse the README's of the respective file from the root Fractal repo.







### Repository Structure




| Subrepository        | Description            |
| -------------------- | ----------------- |
| client-applications  | Insert Title Here |
| container-images     | Insert Title Here |
| ecs-host-service     | Insert Title Here |
| ecs-host-setup       | Insert Title Here |
| ecs-task-definitions | Insert Title Here |
| main-webserver       | Insert Title Here |
| protocol             | Insert Title Here |



The Fractal monorepository contains 7 Fractal subrepositories:

- client-applications -- The client-side Electron application users download and interact with to stream a Fractal application
- container-images -- The Dockerfiles for the application containers users stream
- ecs-host-service -- The Fractal services which runs on hosts and orchestrates container management
- ecs-host-setup -- The Scripts to setup an EC2 instance into a Fractal-optimized host ready to run Fractal containers
- ecs-task-definitions -- This contains the JSON task definitions for each of the applications we stream via containers on AWS ECS
- main-webserver -- The REST API for managing our AWS infrastructure, supporting our front-end and handling communicating between our frontend and container infrastructure when users use Fractal
- protocol -- The streaming protocol, both client and server, which streams the content of a Fractal container to a user and streams the user's actions back to the container. This program is run via commandline.







## Development







## Publishing





## Styling





Each subfolder is its own project with dedicated style

[Documentation & Code Standards](https://www.notion.so/tryfractal/Documentation-Code-Standards-54f2d68a37824742b8feb6303359a597)

[Engineering Guidelines](https://www.notion.so/tryfractal/Engineering-Guidelines-d8a1d5ff06074ddeb8e5510b4412033b)





* [Workflow and Conventions](#workflow-and-conventions)
  - [`master` is for releases only. `staging` is "almost `master`"](#master-is-for-releases-only-staging-is-almost-master)
  - [`dev` is for Development](#dev-is-for-development)
    - [While on feature branches, `git rebase dev` is your friend. `git merge dev` is not.](#while-on-feature-branches-git-rebase-dev-is-your-friend-git-merge-dev-is-not)
    - [But when merging branches, _usually_ do a merge instead of a rebase.](#but-when-merging-feature-branches-usually-do-a-merge-instead-of-a-rebase)
    - [Your branch is yours. Our branches are _ours_.](#your-branch-is-yours-our-branches-are-ours)
  - [On commit logs](#on-commit-logs)
  - [HOTfixes (i.e. prod is on fire)](#hotfixes-ie-prod-is-on-fire)






To see the list of supported applications, check

All of the following applications are based off of the **Ubuntu 20.04 Base Image**.

| Browsers         | Creative   | Productivity |
| ---------------- | ---------- | ------------ |
| Google Chrome    | Blender    | Slack        |
| Mozilla Firefox  | Blockbench | Notion       |
| Brave Browser    | Figma      | Discord      |
| Sidekick Browser | TextureLab |              |
|                  | Gimp       |              |
|                  | Lightworks |              |

Note that before your new task definition is ready to go into production, you need to also edit the database with the app's logo, terms of service link, description, task definition link, etc.


## Workflow and Conventions

### `master` is for releases only. `staging` is "almost `master`".

At Fractal, we maintain a `master` branch for releases only, and auto-tag every commit on `master` with a release tag (TODO).

We also maintain a `staging` branch that serves the same purpose as "release branches" under the [Gitflow workflow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow). Therefore, `staging` will usually be identical to `master`, except as we approach a release date.

When we approach a release milestone, we:

1. Fork `staging` off `master`.
2. Merge `dev` into `staging`. This signifies a feature freeze for that release, and we only perform bug fixes.
3. Perform extensive, end-to-end testing.
   - We fix small (i.e. single-commit) bugs by forking a branch off `staging` and then fast-forward merging it into `staging` (having the same effect as committing directly to `staging`, but with a PR for review and CI).
   - For larger bugs, we still fork a branch off `staging`, but merge it back into `staging` with an explicit merge commit.
4. To release, we merge `staging` back into `master`, creates a tagged release commit. This triggers our auto-deployment workflows, and we push to production.
5. If changes were made to `staging` before release, we merge `staging` into `dev` as well.

### `dev` is for Development

We do all feature development and most bug-fixing on feature branches that are forked off `dev`.

**Feature branches are named as follows:**

- Project-specific feature branches (i.e. those restricted to a single top-level directory in the repo) are named in the form `<author>/<project>/<feature-verb>`. For instance, a branch authored by Savvy and designed to make the ECS host service llama-themed would be called `djsavvy/ecs-host-service/add-llama-theme`.
- Broader feature branches that touch multiple projects should be named in the form `<author>/<feature-verb>`.

Note that the last part of the branch name should almost always be an **action** instead of an object (i.e. `add-llama-theme` instead of `llama-theme`). This makes commit logs much easier to parse.

#### While on feature branches, `git rebase dev` is your friend. `git merge dev` is not.

When making a PR for a feature branch into `dev`, you'll usually find that there's been changes to `dev` since you last branched off. Both the frequency and scope of this situation are magnified by the complexity of a monorepo.

The problem is that you need to catch your branch up to `dev` before your PR can be merged.

One tempting solution is to merge `dev` into your feature branch. **DO NOT do this.**

If you create a pull request for a branch based on an outdated version of `dev`, Github provides you with a shiny button to automatically merge `dev` into your feature branch. **DO NOT use this button.** This leads to git histories that look like [this](#an-example-of-bad-commit-history).

**Here's what to do instead:**

```
git checkout dev
git pull --rebase
git checkout feature-branch
git rebase dev
```

Most of the time, that rebase will work silently, since most changes to dev should not affect the feature branch. If there are any conflicts, that means that someone else is working on the same files as you, and you'll have to manually resolve the conflicts, just as with a merge. If you're having problems rebasing, try `git rebase -r dev` (which handles merge commits differently), then come ask an org admin for help.

#### But when merging feature branches, _usually_ do a merge instead of a rebase.

PRs with really small changes can just be fast-forwarded onto dev, which gives the illusion later of having committed directly onto dev, simplifying the commit log. This can be done on the Github PR page:

![Picture of "Rebase and merge" option](https://ntsim.uk/static/77d197658c4f050ba0a4080747505d8e/3346a/github-rebase-and-merge.png)

In all other cases, select the first option ("Create a merge commit"). **Never** make a squash commit. If you want cleaner git history, rewrite and force-push your own branch (with carefully-applied `git rebase`), then merge it into dev.

#### Your branch is yours. Our branches are _ours_.

In general, feel free to rebase your personal feature branches, amend commit messages/commits for clarify, force-push, or otherwise rewrite git history on them.

However, in the less common case where multiple people are working on a single feature together, they belong on a single feature branch with consistent history. The usual rules of not rewriting published commits apply there. Before making a PR into dev, the point person for that feature should let everyone else know to stop working on that branch and rebase the feature onto dev.

### On commit logs

Clear commit logs are _far_ more important in a monorepo than when spread over several multirepos. There's so many more moving parts, so it's increasingly important to be able to filter a list of commits to what's actually relevant. Tooling can help with this (e.g. `git log -- <subdirectory>`), but in the end we are still limited by the quality of our own commit logs.

Like branch names, commit messages should be descriptive and contain enough context that they make sense _on their own_. Commit messages like "fix" or "works" can be annoyingly unhelpful. Even something as innocuous-sounding as "staging merge" can be confusing --- are we merging into `staging`, or merging `staging` into another branch? What other branch(es) are involved?

Some good rules of thumb:

- Commit messages, like branch suffixes, should be _actions_ instead of objects. If you're parsing a `git log` from a year ago, trying to understand some design decisions, what's more helpful: `"network connections"`, or `"Cut down on extra network connections in handshake"`?
- Add as much context as you possibly can. Imagine you're digging though a commit history, trying to figure out which one may have introduced a critical bug into production. Which helps more: `"increase timeout"`, or `"Increase service A timeout waiting for service B to complete handshake"`?
- Forget the "50 characters max" or whatever artificially-low number Github and Linus Torvalds start complaining at. It's far more useful to have a clear, contextualized commit message that spills over than an artificially terse mess. In the first case, future you (or a colleague) simply has to click once to show the rest of the commit message on Github, or maybe run `git log` in her terminal. In the second case, she's simply left wondering what you meant.

In general, if you find yourself staring at a historic commit log, it's probably because something's gone wrong, and you're trying to piece together what happened. Or maybe you're trying to understand why some code is organized the way it is, and seeing how it evolved can lead you to that understanding. Either way, you've probably lost the mental context you had when writing the code (or you never even had it), so the quality of breadcrumbs (i.e. commit messages) left in the commit log correlates directly with the quality of your well-being in that situation.

### HOTfixes (i.e. prod is on fire)

Eventually, but hopefully rarely, production will be on fire, and we will need to deploy a quick fix ASAP.

Here's the workflow:

1. Fork a branch with name starting with `hotfix/` off `master`. (This is important, as the CI will prevent merging in a non-`staging` branch that doesn't meet this criterion (TODO).)
2. Make the fix on the hotfix branch, and PR it directly to `master`.
3. Get as many eyeballs on the PR as possible, and approve it if it looks good.
4. Wait for all CI checks and tests to pass. This is important --- we will **not** skip CI and force a hotfix PR through to master. The CI is our last line of defense, and the time saved by "skipping it" is not worth the increased chance of pushing another bad commit to production.
5. Merge the hotfix into `master`.
6. Once the fix is confirmed to work, fast-forward `staging` to match `master`.
7. Merge the hotfix into `dev` as well.
8. Write a regression test to make sure the same issue never occurs again, and add it to CI.

# ===

## Appendix

### Useful Monorepo git Tricks

- Viewing a log of only the commits affecting a given file or subdirectory: `git log -- <path>`

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
