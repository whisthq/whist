# Fractal ECS Task Definitions

This repository contains the JSON task definitions for each of the applications we stream via containers on AWS Elastic Container Service. Each application we stream has its dedicated JSON task definition. 

You can retrieve the JSON of a task definition directly in the AWS console when manually testing and developing a new task ahead of production release, and then add it here for version-tracking.

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

## Adding New Task Definitions

To add a new task definition, first ensure that suppor was added for the specific application on the [container-images](https://github.com/fractalcomputers/container-images) repository.

Once that done, create a new `.json` file named `fractal-[folder-name-on-container-images]-[application-name].json`, and copy-paste the content of a a pre-existing task definition JSON, replacing the `"family"` tag with `fractal-[folder-name-on-container-images]-[application-name]`.

Lastly, add your newly supported application to the `app` list in `.github/workflows/render-and-deploy.yml` as `fracta/[folder-name-on-container-images]/[application-name]` and you'll be all set to auto-deploy this new task definition on AWS!
