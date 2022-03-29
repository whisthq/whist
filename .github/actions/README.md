# Custom GitHub Actions

This directory contains custom GitHub Actions that have been written in order to reduce the length of and code duplication in our GitHub Workflows. Each Action is represented by its own directory containing at least a file called `action.yml`.

The syntax of the `action.yml` file is documented [here](https://docs.github.com/en/actions/creating-actions/metadata-syntax-for-github-actions).

Note that in order to call a custom Action in a workflow, the repository must be checked out (using the `actions/checkout` action) first. For example:

```yaml
name: My Workflow
on: push
jobs:
  call-my-action:
    name: Call my custom GitHub Action
    runs-on: ubuntu-20.04
    steps:
      - name: Checkout the repository
        uses: actions/checkout@v3
        with:
          ref: ${{ github.ref }} # Use this input to specify the exact ref to checkout

      - name: Call my Action
        uses: ./.github/actions/my-action
```

When writing custom Actions, please adhere to the style guidelines described in `.github/workflows/README.md`.

## What's a GitHub Action?

In GitHub terminology, [Actions](https://docs.github.com/en/actions/creating-actions/metadata-syntax-for-github-actions) are distinct from [Workflows](https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions). Their docs do not quite make this distinction clear, so here's a quick overview.

**Actions** are single units of work. They are distinct environments with a single set of inputs and an output. They are stateless and do not have access to GitHub context, like repository information or secrets. They must be passed any data they'll use as an input.

**Workflows** are multi-step processes made up of jobs and steps. Often, a Workflow will employ an Action as one of these jobs. A well-formed Workflow is mostly a composition of Actions, with the purpose of wiring up inputs and outputs between them. Workflows have access to GitHub context, like repository information and secrets, and can pass data from that context to jobs, steps, and Actions.

Actions and Workflows are both defined as YAML files stored in `.github/actions` and `.github/workflows`, respectively. The YAML example above shows how a workflow can call Actions with the `uses:` syntax, giving both the example of a offcial GitHub Action (`actions/checkout@v3`) as well as a custom, local action.

Workflows are very hard to run and test locally. They're parsed and evaluated based on a complex domain-specific language using names of nested YAML keys, string templating, and a GitHub-flavored subset of JavaScript. To supply data to their jobs, they rely on the GitHub-specific context that's only available in the GitHub Actions runner. If you've worked with them before, you've probably gone through the clunky commit-push-deploy-wait loop that's necessary to test your work. The awkwardness of this process has led many of us to write complex Bash or Python scripts directly inside the Workflow YAML, so at least some part of it can be tested locally.

Fortunately, Actions put a lot more control in the hands of the developer. They have a much smaller set of configuration options, and only run one process at a time. When creating an Action, you choose from three environments to run your work:

1. Node.js
2. Docker
3. Composite

The simplest of these to work with is Docker, and that's what we'll focus on for the rest of this document. Both Node.js and "Composite" start to bring in the complexity of Workflows by adding jobs and steps and lots of GitHub context that becomes impossible to reproduce locally. By selecting a Docker environment, GitHub effectively "hands off" all execution to your Docker container. You have full control over your environment and dependencies. This allows you to work with a familiar toolset while you're developing and testing, with the expectation that the environment will be the same when you deploy to GitHub.

Our goal here is to be able to comfortably write a program that can be decoupled from GitHub Actions. We want to be able to do the bulk of our work in a familiar environment, writing code that simply accepts data and returns data. Ideally, our program doesn't need to know that it's running in GitHub Actions. In the next sections, we'll configure our Action and Docker container as a GitHub-focused "wrapper" that can host any general-purpose program.

## Setting up a Docker Action

New Actions should be developed in a new subdirectory under `.github/actions`. Note that this folder isn't run automatically by the GitHub Actions runner like the files in `.github/workflows`. When you want to use one of your custom `.github/actions`, you refer to it a Workflow with the `uses:` syntax. Note that `uses:` should refer to folder _containing_ an `action.yml` file.
