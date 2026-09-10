# Continuous integration and progress publishing

Two workflows run in GitHub Actions. They are separated by one question: does
the step need the ROM?

| Workflow | Needs the ROM | Runs on | Purpose |
|---|---|---|---|
| [`report.yml`](../.github/workflows/report.yml) | No | push to `main`, manual | Publishes the progress figures to decomp.dev |
| [`build.yml`](../.github/workflows/build.yml) | The `verify` job does | push, pull request, manual | Rebuilds every matching function and checks it against the ROM |

The split matters. The ROM cannot live in this repository, and secrets are not
available to pull requests from forks. Keeping the report independent of the ROM
means progress stays public even with no secret configured, and a contributor
who has never touched the ROM still gets the consistency gates on their pull
request.

## What the report contains, and why it cannot overstate progress

`tools/gen_report.py` reads `data/functions.csv`, `data/c_sources.csv` and
`data/unmapped_regions.csv`, and writes `report.json` in the [objdiff report
schema](https://github.com/encounter/objdiff/blob/main/objdiff-core/protos/report.proto)
(version 2). It compiles nothing and reads no ROM.

That is safe because those files are not hand-maintained claims. `make check`
regenerates them by rebuilding each function and comparing it against the ROM,
and the `verify` job runs `make check` on every push. A report can therefore only
ever restate the last verified state.

`data/unmapped_regions.csv` is the third one and the reason the figure is not
optimistic. It holds the stretches of code no entry in the function map covers,
measured from the ROM by `tools/find_map_gaps.py --write`, and the report carries
them as a `rom/unmapped` unit with nothing in them matched -- so they are in the
DENOMINATOR. Without it the same progress reads about 0.25 points higher.

The file is cached rather than measured at report time for one reason: this job
must not need the ROM. `make check` runs `tools/find_map_gaps.py --check`, which
re-measures from the ROM and fails if the cached file has drifted, so it cannot
go stale. Regenerate it with `make unmapped-update` after the map changes.

Two arithmetic guards stand behind the published number. `gen_report.py` refuses
to write unless the unit sizes sum to the full ROM code size recorded in
`functions.csv`, and the workflow re-reads the file it produced and asserts the
same thing independently. Both exist for one failure: if a function were dropped
from the units, the total would shrink and the percentage would rise without a
single new function matching.

Run it locally with `make report`, or `python3 tools/gen_report.py --check` to
see the summary without writing. `make check` runs the check form.

## How decomp.dev collects it

decomp.dev is not pushed to. It polls this repository's completed workflow runs
on the default branch and downloads any artifact whose name matches
`<version>_report`, then reads the file inside whose stem is `report`.

So the contract is exactly three things:

- the artifact is named `eu_report` — `eu` becomes the version tab on the site,
  and this repository targets the European release (game code `BGTP`);
- it contains `report.json`;
- the run is a completed `push` on `main`. Reports produced only by a pull
  request are never collected.

Registration is one-time and manual: with the repository public and reports
appearing on `main`, a repository admin adds it at
<https://decomp.dev/manage/new>. Installing the [decomp.dev GitHub
app](https://github.com/apps/decomp-dev) is optional and replaces polling with
an immediate update after each run.

## Giving CI access to the ROM

Without this, the `verify` job records a notice and skips. Nothing else breaks.

The ROM is copyrighted and must not be committed, attached to a release of this
repository, or pasted into a secret. GitHub caps a secret at 48 KB, and the ROM
is 16 MB, so the secret holds a token and the ROM lives elsewhere.

1. Create a **private** repository to hold the asset, for example
   `gta-advance-ci-assets`.
2. Publish a release there with a tag such as `baserom-eu-v1` and attach the ROM
   as a zip archive. The archive layout does not matter; `tools/prepare_rom.sh`
   takes the first `.gba` entry it finds.
3. Create a fine-grained personal access token with **Contents: read-only** on
   that private repository and nothing else.
4. In this repository's settings, add:

   | Kind | Name | Example |
   |---|---|---|
   | Secret | `ROM_ASSETS_TOKEN` | the token from step 3 |
   | Variable | `ROM_ASSETS_REPO` | `ahm3texe/gta-advance-ci-assets` |
   | Variable | `ROM_ASSETS_TAG` | `baserom-eu-v1` |
   | Variable | `ROM_ASSETS_FILE` | `gta-advance-eu.zip` |

The repository name, tag and file name are variables rather than secrets because
they are not sensitive and are worth seeing in a log when a download fails. Only
the token is a secret.

`make prepare-rom` verifies the archive against `config/rom.sha1` and fails if it
does not match, so a wrong or truncated asset stops the run rather than
producing comparisons that are all silently meaningless.

## Getting listed on decomp.wiki

Separate from decomp.dev, and manual. The
[Game Boy Advance project list](https://decomp.wiki/projects/game-boy-advance)
is a plain bullet list; adding an entry is one line.

Edit it through the site: sign in at <https://decomp.wiki/login> with GitHub and
use the page editor. Do **not** open a pull request against
[`decompals/decompedia`](https://github.com/decompals/decompedia) — that
repository is a one-way mirror of the wiki, and none of the pull requests ever
opened against it have been merged.
