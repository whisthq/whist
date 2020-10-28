# Fractal ECS Task Definitions

This repository contains the JSON task definitions for each of the applications we stream via containers on AWS Elastic Container Service. Each application we stream has its dedicated JSON task definition. 

You can retrieve the JSON of a task definition directly in the AWS console when manually testing and developing a new task ahead of production release, and then add it here for version-tracking or simply base it on the existing task definitions.

**Currently supported applications**

- Base (Ubuntu 20.04)
- Google Chrome
- Mozilla Firefox
- Brave Browser
- Blender
- Blockbench
- Slack
- Discord
- Notion
- Figma

## Adding New Task Definitions

To add a new task definition, first ensure that support was added for the specific application on the [container-images](https://github.com/fractalcomputers/container-images) repository.

Once that done, create a new `.json` file named `fractal-[folder-name-on-container-images]-[application-name].json`, and copy-paste the content of a a pre-existing task definition JSON, replacing the `"family"` tag with `fractal-[folder-name-on-container-images]-[application-name]`. For instance, Google Chrome is named `chrome` under the folder `browsers`, so the family tag is `fractal-browsers-chrome`.

Lastly, add your newly-supported application to the `app` list in `.github/workflows/render-and-deploy.yml` as `fractal/[folder-name-on-container-images]/[application-name]`, i.e. `fractal/browsers/chrome`, and you'll be all set to auto-deploy this new task definition on AWS!

## Continous Integration

For every push to `main`, all the task definitions specified in `.github/workflows/render-and-deploy.yml`, which should be all task definition JSONs in this repository, will be automatically rendered and deployed to all supported AWS regions listed under `aws-regions` in `.github/workflows/render-and-deploy.yml`. 

On top of that, whenever a there is a push to `master` on the [container-images](https://github.com/fractalcomputers/container-images) repository, all task definitions specified in `.github/workflows/render-and-deploy.yml` will be rendered and deployed automatically, to update the task definition tags to point to the newly-deployed container images.
