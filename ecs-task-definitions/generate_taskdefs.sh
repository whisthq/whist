#!/bin/bash

# apps to generate task definitions for
apps=("fractal-browsers-chrome"
      "fractal-browsers-firefox"
      "fractal-browsers-brave"
      "fractal-creative-blender"
      "fractal-creative-blockbench"
      "fractal-creative-figma"
      "fractal-creative-gimp"
      "fractal-productivity-discord"
      "fractal-productivity-notion"
      "fractal-productivity-slack"
    )





for app in "${apps[@]}"
do
   : 
   cat fractal-base.json | jq '.family |= "'$app'"' > $app.json
done



