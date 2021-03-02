# Usage: ./build_container_image.py [image_paths...] [-o] [--all]
# If -o is passed in, docker build will output to standard output
# If --all is passed in, all image paths in the repo will be built

import argparse
import re
import multiprocessing
import subprocess
import sys

# Argument parser
parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('-o', '--show-output', action='store_true',
                    help='This flag will have docker build output print to stdout')
parser.add_argument('--all', action='store_true',
                    help='This flag will have docker build every Dockerfile in the current directory')
parser.add_argument('image_paths', nargs='*',
                    help='List of image paths to build')
args = parser.parse_args()

# Input Variables
show_output = args.show_output
image_paths = args.image_paths
build_all = args.all

# If --all is passed, generate image_paths procedurally
if build_all:
  files_process = subprocess.Popen("./helper-scripts/find_images_in_git_repo.sh", shell=True, stdout=subprocess.PIPE)
  image_paths = files_process.communicate()[0].decode("utf-8").strip().split(" ")
  print("All files requested, will be building the following image paths: " + " ".join(image_paths))

# Get dependency path from given image path
def get_dep_from_image(image_path): # returns dep_path
  # Open Dockerfile for the image path
  with open(image_path + "/Dockerfile.20") as f:
    # Regex match the Dockerfile dependency, with a capture group on the dependency name
    regex = re.compile("^[ ]*FROM[ ]+fractal/([^:]*):current-build")
    for line in f:
      result = regex.search(line)
      if result:
        # Grab the capture group
        dependency = result.group(1)

        # Return dependency and tag
        return dependency
  # Otherwise, return None for no dependency
  return None

# Map image_path's to dependencies
dependencies = {}

# Get all dependencies for each requested image_path,
# and add all unrequested dependencies to the image_paths list.
# This ensures that every image_path is either a root node (dependency==None),
# or a child of another image_path
i = 0
while i < len(image_paths):
  image_path = image_paths[i]
  dependency = get_dep_from_image(image_path)
  dependencies[image_path] = dependency
  if dependency and dependency not in image_paths:
    image_paths.append(dependency)
  i += 1

def build_image_path(image_path):
  # Build image path
  print("Building " + image_path + "...")
  command = "docker build -f " + image_path + "/Dockerfile.20 " + image_path + " -t fractal/" + image_path + ":current-build"
  if not show_output:
    command += " >> .build_output 2>&1"
  build_process = subprocess.Popen(command, shell=True)
  build_process.wait()
  if build_process.returncode != 0:
    # If _any_ build fails, we exit with return code 1
    print("Build of " + image_path + " failed, terminating")
    sys.exit(1)
  # Notify successful build
  print("Built " + image_path + "!")

  # Take all of the image_paths that depended on this image_path,
  # and save them as the next layer of image_paths to build
  next_layer = []
  for lhs in dependencies:
    dependency = dependencies[lhs]
    if dependency and dependency == image_path:
      # Clear out dependencies that used to depend on image_path,
      # as this image_path has just finished building
      dependencies[lhs] = None
      # Save next layer image_path
      next_layer.append(lhs)

  # Build all of those next-layer image_paths asynchronously
  procs = []
  for next_image_path in next_layer:
      proc = multiprocessing.Process(target=build_image_path, args=[next_image_path])
      proc.start()
      procs.append(proc)
  # Wait for them all to finished executing before returning
  still_waiting = True
  while still_waiting:
    still_waiting = False
    # Wait for a second, to prevent this loop from being tight
    sleep(1)
    # Loop over all the processes
    for proc in procs:
      # If a process hasn't terminated yet,
      # remember that we're still waiting for those processes
      if proc.exitcode == None:
        still_waiting = True
      # Fail if any child failed, just sysexit prematurely
      elif proc.exitcode != 0:
        sys.exit(1)

# Get all image_path's with no dependencies
root_level_images = []
for image_path in dependencies:
  if dependencies[image_path] == None:
    root_level_images.append(image_path)

# Build all root_level_images
for image_path in root_level_images:
  build_image_path(image_path)

print("All images have been built successfully!")
