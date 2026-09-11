<p align="center">
  <img src="assets/lish.svg" alt="LiSh" width="80">
</p>

# Releasing

How to cut a new version of LiSh. For building/testing day-to-day, see
[SETUP.md](SETUP.md) instead - this is specifically about publishing a
release.

## How Arduino's Library Manager actually finds a release

This is the one part of the process that isn't optional or a matter of
preference: Arduino's Library Manager discovers new versions by watching
this repo's **git tags**. It does the equivalent of `git checkout <tag>`
and reads `library.properties` from that state - no CI, build, or
artifact of any kind is involved on Arduino's side. The tag must match
`library.properties`' `version=` field **exactly**: a bare version number
like `0.2.0`, no `v` prefix.

Everything else in this document - the GitHub Actions workflows, test
report, coverage, board-compile matrix - is release *quality assurance*
for your own benefit and your users' confidence, layered on top of that
one required mechanism. Arduino's tooling doesn't look at any of it.

**One-time step, not part of a normal release**: for a version to be
auto-discovered by Library Manager at all, this repo has to be submitted
once to [arduino/library-registry](https://github.com/arduino/library-registry)
(a PR adding the repo URL to their index). Until that's done, tagging a
release still works (anyone can install it manually, or via
"Add .ZIP Library"), it just won't appear in the Library Manager search
until registration happens. As of this writing, this repo has **not**
been submitted yet.

## Cutting a Release

The easiest way: run **Prepare Release** from the Actions tab ("Run
workflow"), choosing `patch`, `minor`, or `major` - standard semver
(`minor` resets patch to 0; `major` resets minor and patch to 0). It bumps
`library.properties`, commits that change, creates the matching tag, and
pushes both, which is all the manual checklist below does by hand. Same
thing from the CLI:

```bash
gh workflow run prepare-release.yml -f bump=patch
```

Pushing that tag is what actually triggers the real release process (the
**Release** workflow, below) - **Prepare Release** never builds or
publishes anything itself, it just automates the bookkeeping in front of
it. If the Release workflow then fails, the version-bump commit has
already landed on your branch even though nothing was published - the
tag cleanup in step 5 below still applies either way, but you'll also
want to decide whether to keep or revert that commit before trying again.

### Manual Release Checklist

Equivalent to the above, step by step - useful if you want to bump the
version some other way, or understand exactly what **Prepare Release**
is doing on your behalf.

1. **Bump the version** in [`library.properties`](../library.properties)
   (`version=X.Y.Z`, following [semver](https://semver.org/)).
2. **Commit** that change:
   ```bash
   git add library.properties
   git commit -m "Bump version to X.Y.Z"
   ```
3. **Tag it** - bare version number, matching step 1 exactly:
   ```bash
   git tag X.Y.Z
   git push origin main X.Y.Z
   ```
4. **Watch the Release workflow** (Actions tab) run to completion. It:
   - Lints the library (`arduino-lint --compliance strict`) against
     Library Manager's own rules.
   - Builds and runs the full host-side test suite.
   - Compiles `examples/BasicShell` for every supported board preset
     (Uno, Uno R4, Uno R4 Minima, Mega).
   - If all of that passes: creates a GitHub Release for the tag, with
     auto-generated release notes and a `test-report-and-coverage.zip`
     attached (the same `test_report.html` and `coverage/` you'd get
     locally from `ctest` - see [SETUP.md](SETUP.md#test-reports)).

   That zip is attached as a **release asset**, not a workflow artifact -
   GitHub doesn't expire release assets, so it stays downloadable from
   the release page indefinitely, for every release, not just recent
   ones. (A short-lived *copy* of the same zip also exists as a workflow
   artifact purely as plumbing to get it from the test job to the release
   job within that one run - that copy does expire after a couple of
   weeks, but by then it's already been attached to the release
   permanently, so nothing is lost.)
5. **If the workflow fails**, nothing was published - fix the issue,
   commit, and move the tag:
   ```bash
   git tag -d X.Y.Z
   git push origin :refs/tags/X.Y.Z
   # fix, commit, then repeat step 3
   ```

Library Manager itself typically picks up a new matching tag within about
a day of it being pushed (once this repo is registered - see above) - no
action needed beyond pushing the tag.
