# Upstream Sync Process (OpenSCAD → PythonSCAD)

This repository is a long-lived fork of **OpenSCAD**. Changes from upstream
(`openscad/openscad`) are periodically synced into it using
**small, reviewable chunks**, so that:

- merge conflicts are handled by the right domain expert (build vs Python C-API),
- regressions are easier to pinpoint,
- Git/GitHub can correctly determine whether we are “behind” upstream
  (commit-graph ancestry).

> **Important:** To make GitHub stop showing “this branch is X commits behind”,
  **upstream commits themselves** must be integrated (their SHAs appear in
  PythonSCAD's history). Cherry-picking upstream commits must be avoided,
  because it creates new SHAs and can leave GitHub thinking we are still behind.

This document describes the recommended process for syncing upstream changes
into PythonSCAD.

---

## 1 Terminology

- **upstream**: the OpenSCAD repository remote (`https://github.com/openscad/openscad`).
- **origin**: PythonSCAD's GitHub repository.
- **sync branch**: a temporary branch created for each sync cycle.
- **last sync tag**: an annotated tag marking the last upstream commit we
  synced to.

---

## 2 One-time setup

### 2.1 Add the upstream remote

```bash
git remote add upstream https://github.com/openscad/openscad.git
git fetch upstream
```

### 2.2 (Optional, recommended) Enable rerere

This makes Git remember how recurring conflicts have been resolved and can
automatically reapply those resolutions in future syncs.

```bash
git config --global rerere.enabled true
```

---

## 3 “Last synced” convention (annotated tag)

The last synced upstream state is tracked by creating an **annotated tag** that
points to the upstream `master` commit that was synced up to.

### 3.1 Tag name format

Use:

- `upstream-sync/openscad-YYYY-MM-DD`

Example:

- `upstream-sync/openscad-2026-01-29`

---

## 4 Sync workflow (PR-driven, small chunks)

### Goal

- Identify which PRs were merged upstream since the last sync.
- Merge upstream changes into PythonSCAD's fork in the
  **same order upstream landed them** (first-parent order), but still keep
  PR attribution.
- Allow domain experts to resolve conflicts where appropriate.

### Requirements

- **GitHub CLI** (`gh`) installed and authenticated — `plan_openscad_sync.py` shells
  out to `gh pr list`:

  ```bash
  gh auth login
  ```

- **`upstream` remote** configured (see [section 2.1](#21-add-the-upstream-remote)).

- **Local sync tags** — the plan script only considers tags already in your local
  repo matching `upstream-sync/openscad-*`; it does not fetch them. After a fresh
  clone, or if tags are missing, fetch from PythonSCAD:

  ```bash
  git fetch --tags origin
  ```

- **First sync with no tag yet** — if no `upstream-sync/openscad-*` tag exists
  locally (bootstrap case), `plan_openscad_sync.py` exits with an error. Either
  fetch tags from `origin` as above if another maintainer already pushed them, or
  complete the first upstream integration without the script, then create and push
  the first annotated tag (see [section 3](#3-last-synced-convention-annotated-tag))
  so later runs have a baseline.

---

### 4.1 Create `sync/openscad-YYYY-MM-DD` branch

Pick one date for the whole sync cycle and reuse it for the branch name, push,
and final tag so names stay consistent (including across midnight).

```bash
SYNC_DATE=$(date +%Y-%m-%d)
git checkout -b "sync/openscad-$SYNC_DATE" origin/master

git fetch upstream master
```

---

### 4.2 Generate a sync plan (recommended)

Generate a “merge plan” using:

```bash
python3 scripts/plan_openscad_sync.py
```

### 4.3 Apply the plan (merge commits one-by-one)

For each SHA listed by the plan (in order):

```bash
git merge --no-ff <SHA>
# resolve conflicts
# run build/tests
```

### 4.4 Push the branch to origin and open a PR

Push the sync branch to origin:

```bash
git push -u origin "sync/openscad-$SYNC_DATE"
```

Open a PR into `master`.

---

### 4.5 After the sync PR merges into `master`, tag the upstream tip

Create and push the sync tag **only after** the sync PR has been merged into
`master`. Tagging earlier can imply PythonSCAD is already synced when the changes
are still only in an open PR.

Update local `master` if helpful, then use the **same** `SYNC_DATE` as the sync
branch (set it again if your shell session is new, e.g.
`SYNC_DATE=2026-04-12` matching `sync/openscad-2026-04-12`).

```bash
git fetch upstream

UP_TIP=$(git rev-parse upstream/master)

TAG="upstream-sync/openscad-$SYNC_DATE"
git tag -a "$TAG" "$UP_TIP" -m "Synced OpenSCAD up to $UP_TIP"

git push origin "$TAG"
```

---

## 5 Troubleshooting

### 5.1 If a PR’s mergeCommit is missing

Sometimes PR metadata can be odd depending on merge mode or repo settings.

Fallback options:

- Find the commit on upstream `master` that mentions the PR number:

  ```bash
  git log upstream/master --first-parent --grep "#<PR_NUMBER>" --oneline
  ```

- Or search via GitHub CLI:

  ```bash
  gh pr list -R openscad/openscad --state merged --search "<SHA>"
  ```

---
