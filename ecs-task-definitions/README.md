# Fractal ECS Task Definitions

This repository contains the JSON task definitions for each of the applications we stream via containers on AWS Elastic Container Service. Each application we stream has its dedicated JSON task definition.

You can retrieve the JSON of a task definition directly in the AWS console when manually testing and developing a new task ahead of production release, and then add it here for version-tracking or simply base it on the existing task definitions.

### Currently supported applications

All of the following applications are based off of the **Ubuntu 20.04 Base Image**.

| Browsers         | Creative   | Productivity |
| ---------------- | ---------- | ------------ |
| Google Chrome    | Blender    | Slack        |
| Mozilla Firefox  | Blockbench | Notion       |
| Brave Browser    | Figma      | Discord      |
| Sidekick Browser | TextureLab |              |
|                  | Gimp       |              |

## Generating Task Definitions

You can generate the task definitions from `fractal-base.json` by running `./generate_taskdefs.sh`.

## Adding New Task Definitions

To add a new task definition, first ensure that support was added for the specific application on the [`container-images`](https://github.com/fractal/container-images) repository.

Once that done, add the application to the `apps` array in `generate_taskdefs.sh` as `fractal-[folder-name-on-container-images]-[application-name]`, alongside with any app-specific tags that need to be modified from the `fractal-base.json` template, in the for loop. Lastly, add your newly-supported application to the `app` list in `.github/workflows/render-and-deploy.yml` as `fractal/[folder-name-on-container-images]/[application-name]`, i.e. `fractal/browsers/chrome`, and you'll be all set to auto-deploy this new task definition on AWS via GitHub Actions! Note that before your new task definition is ready to go into production, you need to also edit the database with the app's logo, terms of service link, description, task definition link, etc.

## Publishing & Continous Integration

For every push to `main`, all the task definitions specified in `.github/workflows/render-and-deploy.yml`, which should be all task definition JSONs in this repository, will be automatically rendered and deployed to all supported AWS regions listed under `aws-regions` in `.github/workflows/render-and-deploy.yml`.

On top of that, whenever there is a push to `master` on the [`container-images`](https://github.com/fractal/container-images) repository, all task definitions specified in `.github/workflows/render-and-deploy.yml` will be rendered and deployed automatically to update the task definition tags to point to the newly-deployed container images.
