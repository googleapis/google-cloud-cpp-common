# Creating a new patch release of google-cloud-cpp-common

The intended audience of this document are developers in the
`google-cloud-cpp-common` project that need to create a new release. The
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

Now you can create a branch of the release branch, we will use `v0.22.x` as an
example, replace that with the right version:

```bash
git checkout v0.22.x
git checkout -b prepare-v0.22.1-patch
```

Then cherry-pick the changes you want, without committing them

```bash
git cherry-pick -n 140c7e3d0ad06accd639e95164f5de2c4c82642e
```

Update the release notes to include these changes, commit these changes. You
may want to use the same description as in the change to `master`.

```bash
git add CHANGELOG.md
git commit
```

Run the usual tests, push the changes and create a PR. You must use `v0.22.x`
as the target branch of your PR.

## Creating the release

As of this writing the release script always cuts releases from `master`,
create the release manually using the GitHub website is probably best.
