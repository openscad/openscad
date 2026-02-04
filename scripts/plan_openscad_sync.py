#!/usr/bin/env python3
import json
import subprocess
from datetime import date

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
    newtag = date.today().strftime("%Y-%m-%d")

    print("# OpenSCAD sync plan")
    print(f"# last sync tag: {tag}")
    print(f"# baseline upstream commit: {base}")
    print(f"# PR cutoff (tagger date): {since_iso}")
    print(f"# upstream tip: {tip}")
    print()
    print("# git checkout master")
    print("# git checkout -b \"sync/openscad-%s\""%(newtag))
    print("# merge all hashes listed below")
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

    print()
    print(f"# finally:")
    print("# git tag -a \"upstream-sync/openscad-%s\" \"%s\" -m \"Synced OpenSCAD up to %s\""%(newtag, tip, tip))
    print("# git push origin \"upstream-sync/openscad-%s\""%(newtag))

if __name__ == "__main__":
    main()
