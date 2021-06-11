# Custom GitHub Actions

This directory contains custom GitHub Actions that have been written in order to reduce the length of and code duplication in our GitHub workflows. Each Action is represented by its own directory containing at least a file called `action.yml`.

The syntax of the `action.yml` file is documented [here](https://docs.github.com/en/actions/creating-actions/metadata-syntax-for-github-actions).

Note that in order to call a custom Action in a workflow, the repository must be checked out (using the `actions/checkout` action) first. For example:

```yaml
name: My Workflow
on: push
jobs:
  call-my-action:
    name: Call my custom GitHub Action
    runs-on: ubuntu-latest
    steps:
      - name: Checkout the repository
        uses: actions/checkout@v2
        with:
          ref: ${{ github.ref }} # Use this input to specify the exact ref to checkout

      - name: Call my Action
        uses: ./.github/actions/my-action
```

When writing custom Actions, please adhere to the style guidelines described in `.github/workflows/README.md`.
