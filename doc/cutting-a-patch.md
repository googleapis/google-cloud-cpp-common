# Creating a new patch release of google-cloud-cpp-common

The intended audience of this document are developers in the
`google-cloud-cpp-common` project that need to create a new patch release. The
audience is expected to be familiar with the project itself, [git][git-docs],
[GitHub][github-guides], [semantic versioning](https://semver.org), and
with how to [cut a release](cutting-a-release.md).

## Prepare a PR for the Release Branch

We will assume that you have created a clone of your fork, as in:

```bash
git clone git@github.com:${GITHUB_USERNAME}/google-cloud-cpp-common.git
cd google-cloud-cpp-common
git remote add upstream https://github.com/googleapis/google-cloud-cpp-common.git
git fetch upstream
```

We will use some variables to represent the release branch and the patch
version:

```bash
RELEASE_BRANCH="v0.22.x"
PATCH_VERSION="v0.22.1"
```

With these variables we create a branch out of the existing release branch:

```bash
git checkout "${RELEASE_BRANCH}"
git checkout -b "prepare-${PATCH_VERSION}-patch"
```

Then cherry-pick the changes you want, without committing them, for example:

```bash
git cherry-pick -n 140c7e3d0ad06accd639e95164f5de2c4c82642e
```

Update the release notes to include these changes, and commit them, including
your updates to the `CHANGELOG.md` file:

```bash
git add CHANGELOG.md
git commit
```

You may want to use the same description as in the change to `master`.

Run the usual tests, push the changes and create a PR. You must use
"${RELEASE_BRANCH}" as the target branch of your PR.

## Creating the release

As of this writing the release script always cuts releases from `master`,
so creating the release manually using the GitHub website is probably best.
