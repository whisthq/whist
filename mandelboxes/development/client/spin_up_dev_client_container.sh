#!/bin/bash

# This script spins up the Whist protocol dev client container


# TODO: 1. Download any necessary user configs onto the container

# TODO: 2. Apply port bindings/forwarding if needed

# TODO: 3. Pass config variables such as FRACTAL_AES_KEY, which will be saved to file by the startup/entrypoint.sh script, in order for the container to be able to access them later and exported them as environment variables by the `run-fractal-client.sh` script. These config variables will have to be passed as parameters to the FractalClient executable, which will run in non-root mode in the container (username = fractal).

# TODO: 4. Create the Docker container, and start it

# TODO: 5. Decrypt user configs within the docker container, if needed

# TODO: 6. Write the config.json file if we want to test JSON transport related features

# TODO: 7. Write the .ready file to trigger `base/startup/fractal-startup.sh`
