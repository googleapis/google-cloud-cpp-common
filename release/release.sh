#!/bin/bash
#
# Usage:
#   $ release.sh [-f] <organization/project-name>
#
#   Options:
#     -f     Force; actually make and push the changes
#
# Example:
#
#   # Shows what commands would be run. NO CHANGES ARE PUSHED
#   $ release.sh googleapis/google-cloud-cpp-spanner
#
#   # Shows commands AND PUSHES changes when -f is specified
#   $ release.sh -f googleapis/google-cloud-cpp-spanner
# 
# This script creates a "release" on github by doing the following:
#
#   1. Computes the next version to use
#   2. Creates and pushes the tag w/ the new version
#   3. Creates and pushes a new branch w/ the new version
#
# Before running this script the user should make sure the README.md on master
# is up-to-date with the release notes for the new release that will happen.
# Then run this script. After running this script, the user must still go
# create the Release in the GitHub web UI using the new tag name that was
# created by this script.
#
# TODO: Consider using github's `hub` command (apt install hub) to automate the
# creation of the actual Release on GitHub. This shouldn't be too difficult.

set -eu

# Extracts all the documentation at the top of this file as the usage text.
readonly USAGE="$(sed -n '3,/^$/s/^# \?//p' $0)"

FORCE_FLAG="no"
while getopts "f" opt "$@"; do
  case "$opt" in
    [f]) 
      FORCE_FLAG="yes";;
    *)
      echo "$USAGE"
      exit 1;;
  esac
done
shift $((OPTIND - 1))
declare -r FORCE_FLAG

if [[ $# -ne 1 ]]; then
  echo "$USAGE"
  exit 1;
fi

readonly PROJECT="$1"
readonly CLONE_URL="git@github.com:${PROJECT}.git"
readonly TMP_DIR="$(mktemp -d /tmp/${PROJECT//\//-}-release.XXXXXXXX)"
readonly REPO_DIR="${TMP_DIR}/repo"

function banner() {
  local color=$(tput bold; tput setaf 4; tput rev)
  local reset=$(tput sgr0)
  echo "${color}$@${reset}"
}

function run() {
  echo "[ $@ ]"
  if [[ "${FORCE_FLAG}" == "yes" ]]; then
    $@
  fi
}

function exit_handler() {
  if [[ -d "${TMP_DIR}" ]]; then
    banner "OOPS! Unclean shutdown"
    echo "Local repo at ${REPO_DIR}"
    echo 1
  fi
}
trap exit_handler EXIT

banner "Starting release for ${PROJECT} (${CLONE_URL})"
git clone "${CLONE_URL}" "${REPO_DIR}"
cd "${REPO_DIR}"

# Figures out the most recent tagged version, and computes the next version.
readonly TAG="$(git describe --tags --abbrev=0 origin/master)"
readonly CUR_TAG="$(test -n "${TAG}" && echo "${TAG}" || echo "v0.0.0")"
readonly NEW_RELEASE="$(perl -pe 's/v0.(\d+).0/"v0.${\($1+1)}"/e' <<<"${CUR_TAG}")"
readonly NEW_TAG="${NEW_RELEASE}.0"
readonly NEW_BRANCH="${NEW_RELEASE}.x"

banner "Release info for ${NEW_RELEASE}"
echo "Current tag: ${CUR_TAG}"
echo "    New tag: ${NEW_TAG}"
echo " New branch: ${NEW_BRANCH}"

banner "Creating and pushing tag ${NEW_TAG}"
run git tag "${NEW_TAG}"
run git push origin "${NEW_TAG}"

banner "Creating and pushing branch ${NEW_BRANCH}"
run git checkout -b "${NEW_BRANCH}" "${NEW_TAG}"
run git push --set-upstream origin "${NEW_BRANCH}"

banner "Success!"
if [[ "${TMP_DIR}" == /tmp/* ]]; then
  rm -rf "${TMP_DIR}"
fi
