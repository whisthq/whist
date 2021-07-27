# Load Fractal Secrets

This GitHub Action has the job of loading any "secret" values that are needed at CI time. It's easy for secrets to get split up among providers like GitHub, Heroku, and Netlify, so there's a good amount of code needed to properly call their APIs, validate the responses, and extract the stored secrets. Because secrets are consumed in many different places throughout CI, this results in lots of duplicated code.

This Action is intended to be a one stop shop for us to retrieve secrets. The output is a map with all the secrets from all providers merged together. Consumers shouldn't need to know where they're getting the secret from, they just need to know the name of the secret key they're looking for.

Because this Action will be run in GitHub, it actually needs to be passed the GitHub secrets object, like in the example below. The output object will contain all of these secrets, plus the secrets from other providers merged in.

This Action can also take other parameters to configure how secrets are retrieved. For example, the branch name might need to be passed to Heroku to determine which config we pull in.

## Building Docker Image

To build the Docker image for this environment, set your working directory as the monorepo root and run the following command:

``` sh
docker build -t fractal/monorepo-secrets .github/actions/monorepo-secrets
```

This will build using the Dockerfile in `.github/actions/monorepo-secrets`, and tag the image as `fractal/monorepo-secrets`.

To run the container and enter a shell:

``` sh
docker run -it --rm fractal/monorepo-secrets 
```

This will place you in `/`. The source code lives in `/root`. You can `pytest /root` to run the tests.

If you're working on the code, you'll probably want to mount the repository on your host machine to the `/root` directory of the container. Use this command to run a shell in the container. Any files you change on the host will be change in the container without needed to rebuild.

``` sh
docker run -it \
           --rm \
           --volume $(pwd)/.github/actions/monorepo-secrets:/root fractal/monorepo-secrets
```

`
