# util.sh

# Helpful functions that are useful for preparing the fractal application for
# deployment.

# This code was copied from
# https://devcenter.heroku.com/articles/buildpack-api#bin-compile-summary
export_env_dir() {
  env_dir=$1
  acceptlist_regex=${2:-''}
  denylist_regex=${3:-'^(PATH|GIT_DIR|CPATH|CPPATH|LD_PRELOAD|LIBRARY_PATH)$'}
  if [ -d "$env_dir" ]; then
    for e in $(ls $env_dir); do
      echo "$e" | grep -E "$acceptlist_regex" | grep -qvE "$denylist_regex" &&
      export "$e=$(cat $env_dir/$e)"
      :
    done
  fi
}
