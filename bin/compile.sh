#!/usr/bin/env bash
# bin/compile <build-dir> <cache-dir> <env-dir>

# used for inline heroku buildpack

BUILD_DIR=${1:-}
CACHE_DIR=${2:-}
ENV_DIR=${3:-}

PROJECT_PATH=src

echo "-----> My Subdir buildpack: path is $PROJECT_PATH"
echo "       creating cache: $CACHE_DIR"
mkdir -p $CACHE_DIR
TMP_DIR=`mktemp -d $CACHE_DIR/subdirXXXXX`
echo "       created tmp dir: $TMP_DIR"

echo "       moving working dir: $PROJECT_PATH to $TMP_DIR"
cp -R $BUILD_DIR/$PROJECT_PATH/. $TMP_DIR/

echo "       cleaning build dir $BUILD_DIR"
rm -rf $BUILD_DIR
echo "       recreating $BUILD_DIR"
mkdir -p $BUILD_DIR
echo "       copying work dir from cache $TMP_DIR to build dir $BUILD_DIR"
cp -R $TMP_DIR/. $BUILD_DIR/
echo "       cleaning tmp dir $TMP_DIR"
rm -rf $TMP_DIR
exit 0

