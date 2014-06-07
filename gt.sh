#!/usr/bin/env bash
if ["$1" == ""]; then
  git tag
else
  RAW=${PWD##*/}
  APP_LOWER=`echo $RAW | tr '[:upper:]' '[:lower:]' | tr -d '\n'`
  VERSION_ID="$1"
  TAG_NAME="$APP_LOWER-$VERSION_ID"
  git tag -a "$TAG_NAME" -m 'Version '"$VERSION_ID"
  git push origin "$TAG_NAME"
fi