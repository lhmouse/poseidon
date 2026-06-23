#!/bin/bash -e

if test "$1" == "" || test "$1" == --help
then
  echo "./update_submodule.sh NAME [TAG]"
  exit 1
fi

_submodule=$(basename "${1}")
_tag="${2:-master}"
_message="submodules/${_submodule}: Update to ${_tag}"

# Update to target branch
git submodule update --init -- "${_submodule}"
cd "${_submodule}"

if ! grep -Eq '^gitdir: \.\./\.git/' .git
then
  echo "${_submodule}: Not something like a submodule"
  exit 1
fi

git fetch origin "${_tag}"
git checkout -f FETCH_HEAD
cd -

# Commit if there are diffs
if ! git diff HEAD --exit-code -- "${_submodule}"
then
  git commit -sm "${_message}" -- "${_submodule}"
fi
