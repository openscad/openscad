# Upstream Sync Process (OpenSCAD → PythonSCAD)

This repository is a long-lived fork of **OpenSCAD**. We periodically sync
changes from upstream (`openscad/openscad`) into our fork in
**small, reviewable chunks**, so that:

- merge conflicts are handled by the right domain expert (build vs Python C-API),
- regressions are easier to pinpoint,
- Git/GitHub can correctly determine whether we are “behind” upstream
  (commit-graph ancestry).

> **Important:** To make GitHub stop showing “this branch is X commits behind”,
  we must integrate **upstream commits themselves** (their SHAs appear in our
  history). Avoid cherry-picking upstream commits as a general sync mechanism,
  because it creates new SHAs and can leave GitHub thinking we are still behind.

This document describes the recommended process for syncing upstream changes
into our fork.

---

## 1) Terminology

- **upstream**: the OpenSCAD repository remote (`https://github.com/openscad/openscad`).
- **origin**: our fork (PythonSCAD) GitHub repository.
- **sync branch**: a temporary branch created for each sync cycle.
- **last sync tag**: an annotated tag marking the last upstream commit we
  synced to.

---

## 2) One-time setup

### 2.1 Add the upstream remote

```bash
git remote add upstream https://github.com/openscad/openscad.git
git fetch upstream
```

### 2.2 (Optional, recommended) Enable rerere

This makes Git remember how you resolved recurring conflicts and can
automatically reapply those resolutions in future syncs.

```bash
git config --global rerere.enabled true
```

---

## 3) “Last synced” convention (annotated tag)

We track the last synced upstream state by creating an **annotated tag** that
points to the upstream `master` (or `main`) commit that we synced up to.

### 3.1 Tag name format

Use:

- `upstream-sync/openscad-YYYY-MM-DD`

Example:

- `upstream-sync/openscad-2026-01-29`

### 3.2 Creating the tag (after a successful sync)

```bash
# ensure upstream refs are current
git fetch upstream

UP_TIP=$(git rev-parse upstream/master)

# create an annotated tag pointing at the upstream tip commit
TAG="upstream-sync/openscad-$(date +%Y-%m-%d)"
git tag -a "$TAG" "$UP_TIP" -m "Synced OpenSCAD up to $UP_TIP"

# push the tag to our fork
git push origin "$TAG"
```

> If OpenSCAD uses `main` instead of `master`, replace `upstream/master` with `upstream/main`.

---

## 4) Sync workflow (PR-driven, small chunks)

### Goal

- Identify which PRs were merged upstream since the last sync.
- Merge upstream changes into our fork in the
  **same order upstream landed them** (first-parent order), but still keep
  PR attribution.
- Allow domain experts to resolve conflicts where appropriate.

### Requirements

- GitHub CLI installed and authenticated: `gh auth login`
- `upstream` remote configured

---

## 5) Generate a sync plan (recommended)

We generate a “merge plan” using:

- **PR list** from GitHub (merged PRs since the last sync), and
- the **upstream first-parent** commit sequence from the last synced upstream
commit to current upstream tip.

This supports OpenSCAD’s **mixed merge strategies** (merge commits and squash merges):

- For merge commits: the merge commit SHA is on upstream `master`.
- For squash merges: the squash result is a single commit SHA on upstream `master`.

### 5.1 Create `sync/openscad-YYYY-MM-DD` branch

```bash
git checkout -b sync/openscad-$(date +%Y-%m-%d) origin/master

git fetch upstream master
```

### 5.2 Generate plan via script

Create `scripts/plan_openscad_sync.py` with the contents below (or keep it
elsewhere if you prefer). The script:

- finds the latest `upstream-sync/openscad-*` annotated tag,
- uses the tag’s target commit as the baseline,
- queries GitHub for merged PRs since the tagger date,
- walks upstream `master` commits in first-parent order,
- prints an ordered list where each line is either:
  - `PR #NNNN: Title …` (if the commit matches a PR mergeCommit), or
  - `(direct) …` for non-PR commits on upstream master.

```python
#!/usr/bin/env python3
import json
import subprocess

UPSTREAM = "upstream"
UPSTREAM_BRANCH = "master"   # change to "main" if needed
REPO = "openscad/openscad"
TAG_PREFIX = "upstream-sync/openscad-"


def sh(*args):
    return subprocess.check_output(args, text=True).strip()


def latest_sync_tag():
    out = sh(
        "git",
        "for-each-ref",
        "--sort=-taggerdate",
        "--format=%(refname:short)",
        f"refs/tags/{TAG_PREFIX}*",
    )
    tags = [t for t in out.splitlines() if t]
    return tags[0] if tags else None


def tag_target(tag):
    # Dereference the tag object to the commit it points to
    return sh("git", "rev-parse", f"{tag}^{{}}")


def tagger_iso(tag):
    iso = sh(
        "git",
        "for-each-ref",
        "--format=%(taggerdate:iso8601)",
        f"refs/tags/{tag}",
    )
    if not iso:
        raise SystemExit(
            f"Tag {tag} has no tagger date (is it lightweight?). Use \
              annotated tags: git tag -a …"
        )
    return iso


def fetch_upstream():
    subprocess.check_call(["git", "fetch", UPSTREAM, UPSTREAM_BRANCH])


def upstream_tip():
    return sh("git", "rev-parse", f"{UPSTREAM}/{UPSTREAM_BRANCH}")


def first_parent_commits(base, tip):
    out = sh("git", "rev-list", "--first-parent", "--reverse", f"{base}..{tip}")
    return [c for c in out.splitlines() if c]


def pr_list_since(since_iso):
    # GitHub search uses date qualifiers like merged:>=YYYY-MM-DD
    since_date = since_iso[:10]
    cmd = [
        "gh",
        "pr",
        "list",
        "-R",
        REPO,
        "--state",
        "merged",
        "--search",
        f"base:{UPSTREAM_BRANCH} merged:>={since_date}",
        "--json",
        "number,title,mergedAt,mergeCommit,labels,url",
        "--limit",
        "1000",
    ]
    data = sh(*cmd)
    prs = json.loads(data) if data else []
    by_merge_commit = {}
    for pr in prs:
        mc = pr.get("mergeCommit")
        oid = mc.get("oid") if isinstance(mc, dict) else None
        if oid:
            by_merge_commit[oid] = pr
    return by_merge_commit


def main():
    tag = latest_sync_tag()
    if not tag:
        raise SystemExit(
            f"No sync tags found (expected {TAG_PREFIX}*). Create one after \
              the next successful sync."
        )

    base = tag_target(tag)
    since_iso = tagger_iso(tag)

    fetch_upstream()
    tip = upstream_tip()

    pr_by_commit = pr_list_since(since_iso)
    commits = first_parent_commits(base, tip)

    print("# OpenSCAD sync plan")
    print(f"# last sync tag: {tag}")
    print(f"# baseline upstream commit: {base}")
    print(f"# PR cutoff (tagger date): {since_iso}")
    print(f"# upstream tip: {tip}")
    print()

    for c in commits:
        pr = pr_by_commit.get(c)
        if pr:
            labels = ",".join(
                l["name"]
                for l in (pr.get("labels") or [])
                if isinstance(l, dict) and l.get("name")
            )
            print(
                f"{c}  PR #{pr['number']}: {pr['title']}  [{labels}]  {pr['url']}"
            )
        else:
            subj = sh("git", "show", "-s", "--format=%s", c)
            print(f"{c}  (direct) {subj}")


if __name__ == "__main__":
    main()
```

Run it:

```bash
python3 scripts/plan_openscad_sync.py | less
```

---

## 6) Apply the plan (merge commits one-by-one)

For each SHA listed by the plan (in order):

```bash
git merge --no-ff <SHA>
# resolve conflicts
# run build/tests
```

### 6.1 Conflict ownership rule

- **Build system / packaging / CI** conflicts: handled by the build expert.
- **Python C-API extension** conflicts: handled by the Python C-API expert.

Tip: when you hit a conflict you want the other person to resolve, push the
sync branch and hand off.

---

## 7) Open a PR in our fork

When the sync branch builds and tests pass:

1. Push the sync branch:

   ```bash
   git push -u origin sync/openscad-$(date +%Y-%m-%d)
   ```

2. Open a PR into `master`.
3. Assign reviewers based on the conflict ownership rule.

---

## 8) Finish: tag the upstream tip we synced to

After the sync PR is merged into our `master`, create the annotated sync tag
(Section 3.2).

---

## 9) Notes / Troubleshooting

### 9.1 If a PR’s mergeCommit is missing

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

### 9.2 If OpenSCAD renames `master` → `main`

Update in the script and commands:

- `UPSTREAM_BRANCH = "main"`
- `git fetch upstream main`
- `upstream/main`

---

## 10) Suggested repository files

- `UPSTREAM_SYNC.md` (this document)
- `scripts/plan_openscad_sync.py` (plan generator)

---
