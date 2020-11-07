#!/bin/bash

# apps to generate task definitions for
apps=("fractal-browsers-chrome"
      "fractal-browsers-firefox"
      "fractal-browsers-brave"
      "fractal-creative-blender"
      "fractal-creative-blockbench"
      "fractal-creative-texturelab"
      "fractal-creative-figma"
      "fractal-creative-gimp"
      "fractal-productivity-discord"
      "fractal-productivity-notion"
      "fractal-productivity-slack"
    )

# Currently, all apps have the same task definition parameters,
# so we only need to update the family tag.
#
# When there are app-specific parameters, we can update those
# as well in this for loop
for app in "${apps[@]}"
do
   : 
   cat fractal-base.json | jq '.family |= "'$app'"' > $app.json
done
