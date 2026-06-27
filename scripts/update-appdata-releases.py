#!/usr/bin/env python3
"""
Regenerate the AppStream <releases> block in pythonscad.appdata.xml.in.

The release history shown by Linux software centers (GNOME Software, KDE
Discover, Flathub, ...) is generated from CHANGELOG.md, which is the single
source of truth maintained by release-please.

Release selection policy (hybrid):

  * Versions <= BACKFILL_MINORS_THROUGH: only minor/major releases
    (patch component == 0) are listed. This collapses the early
    rapid-fire patch bursts that carry little user value.
  * Versions  > BACKFILL_MINORS_THROUGH: every release is listed,
    patches included.

Usage:
    update-appdata-releases.py [--check]
    update-appdata-releases.py --changelog CHANGELOG.md --appdata file.xml.in

With --check the script makes no changes and exits non-zero if the file is
out of date (for CI / pre-commit).
"""

import argparse
import html
import re
import sys
from pathlib import Path

# Releases at or below this version are pruned to minor/major only; newer
# releases are all listed. See module docstring.
BACKFILL_MINORS_THROUGH = (1, 0, 0)

# Maximum number of bullet points rendered per section before the list is
# truncated with a trailing "and more" entry.
MAX_ITEMS_PER_SECTION = 12

# CHANGELOG sections mapped to the human-facing label used in the release
# notes. Only these (user-facing) sections are surfaced in the metadata.
SECTIONS = [
    ("Features", "Features:"),
    ("Bug Fixes", "Bug fixes:"),
]

TAG_URL = "https://github.com/pythonscad/pythonscad/releases/tag/v{version}"

REPO_ROOT = Path(__file__).resolve().parent.parent

HEADING_RE = re.compile(r"^## \[?(\d+)\.(\d+)\.(\d+)")
DATE_RE = re.compile(r"\((\d{4}-\d{2}-\d{2})\)\s*$")
SECTION_RE = re.compile(r"^### (.+?)\s*$")
BULLET_RE = re.compile(r"^\* (.+?)\s*$")
RELEASES_BLOCK_RE = re.compile(r"^[ \t]*<releases>.*?</releases>", re.DOTALL | re.MULTILINE)

# A trailing "([text](url))" PR/commit reference group, anchored to the end.
TRAILING_REF_RE = re.compile(r"\s*\(\[[^\]]+\]\([^)]+\)\)\s*$")
# A trailing "closes/fixes [text](url)" issue reference, optionally wrapped in
# parentheses (e.g. "(closes [#648](url))") and anchored to the end.
TRAILING_CLOSES_RE = re.compile(
    r",?\s*\(?(?:closes|fixes)\s+\[[^\]]+\]\([^)]+\)\)?\s*$", re.IGNORECASE)
# An in-text markdown link "[text](url)" -> "text".
MARKDOWN_LINK_RE = re.compile(r"\[([^\]]+)\]\([^)]+\)")


def parse_changelog(text):
    """Parse CHANGELOG.md into a list of release dicts (newest first)."""
    releases = []
    current = None
    section = None
    for line in text.splitlines():
        heading = HEADING_RE.match(line)
        if heading:
            version = tuple(int(g) for g in heading.groups())
            date_match = DATE_RE.search(line)
            current = {
                "version": version,
                "date": date_match.group(1) if date_match else "",
                "sections": {},
            }
            releases.append(current)
            section = None
            continue
        if current is None:
            continue
        section_match = SECTION_RE.match(line)
        if section_match:
            section = section_match.group(1)
            continue
        bullet_match = BULLET_RE.match(line)
        if bullet_match and section is not None:
            current["sections"].setdefault(section, []).append(bullet_match.group(1))
    return releases


def selected(version):
    """Apply the hybrid release-selection policy."""
    if version > BACKFILL_MINORS_THROUGH:
        return True
    return version[2] == 0


def _xml_escape(text):
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def clean_bullet(text):
    """Turn a markdown changelog bullet into AppStream-safe inline text."""
    # Strip trailing reference noise (PR/commit links and "closes #nnn"
    # trailers). Anchored to the end and applied in a loop so several stacked
    # trailers are removed (e.g. " ([#664](url)) ([8722c7b](url))" or
    # "..., closes [#591](url)") without touching links in the middle of a
    # bullet, which are legitimate content.
    previous = None
    while previous != text:
        previous = text
        text = TRAILING_REF_RE.sub("", text)
        text = TRAILING_CLOSES_RE.sub("", text)
    # Flatten any remaining (in-text) markdown links to their visible text.
    text = MARKDOWN_LINK_RE.sub(r"\1", text)
    # Bold markers carry no meaning once flattened (used for scopes).
    text = text.replace("**", "")
    # CHANGELOG already HTML-escapes some characters; normalise to raw first.
    text = html.unescape(text)

    # Re-escape for XML, turning inline `code` spans into <code> elements.
    parts = []
    for index, segment in enumerate(text.split("`")):
        if index % 2 == 1:
            parts.append(f"<code>{_xml_escape(segment)}</code>")
        else:
            parts.append(_xml_escape(segment))
    result = "".join(parts)

    # Prevent CMake configure_file() from treating ${...} as a substitution.
    result = result.replace("${", "&#36;{")
    return re.sub(r"\s+", " ", result).strip()


def render_release(release):
    version_str = ".".join(str(n) for n in release["version"])
    lines = [f'    <release version="{version_str}" date="{release["date"]}">']
    lines.append(f'      <url type="details">{TAG_URL.format(version=version_str)}</url>')

    blocks = []
    for key, label in SECTIONS:
        bullets = release["sections"].get(key)
        if not bullets:
            continue
        seen = set()
        unique = []
        for bullet in bullets:
            cleaned = clean_bullet(bullet)
            if cleaned and cleaned not in seen:
                seen.add(cleaned)
                unique.append(cleaned)
        if not unique:
            continue
        truncated = len(unique) > MAX_ITEMS_PER_SECTION
        shown = unique[:MAX_ITEMS_PER_SECTION]
        item_lines = [f"          <li>{item}</li>" for item in shown]
        if truncated:
            item_lines.append("          <li>and more</li>")
        blocks.append((label, item_lines))

    if blocks:
        lines.append("      <description>")
        for label, item_lines in blocks:
            lines.append(f"        <p>{label}</p>")
            lines.append("        <ul>")
            lines.extend(item_lines)
            lines.append("        </ul>")
        lines.append("      </description>")

    lines.append("    </release>")
    return "\n".join(lines)


def render_releases_block(releases):
    chosen = [r for r in releases if selected(r["version"])]
    chosen.sort(key=lambda r: r["version"], reverse=True)
    body = "\n".join(render_release(r) for r in chosen)
    return f"  <releases>\n{body}\n  </releases>"


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--changelog", type=Path, default=REPO_ROOT / "CHANGELOG.md")
    parser.add_argument("--appdata", type=Path,
                        default=REPO_ROOT / "pythonscad.appdata.xml.in")
    parser.add_argument("--check", action="store_true",
                        help="exit non-zero if the file would change; make no edits")
    args = parser.parse_args()

    changelog = args.changelog.read_text(encoding="utf-8")
    appdata = args.appdata.read_text(encoding="utf-8")

    releases = parse_changelog(changelog)
    if not releases:
        print("Error: no releases parsed from changelog", file=sys.stderr)
        return 1

    new_block = render_releases_block(releases)
    if not RELEASES_BLOCK_RE.search(appdata):
        print(f"Error: no <releases> block found in {args.appdata}", file=sys.stderr)
        return 1
    updated = RELEASES_BLOCK_RE.sub(lambda _: new_block, appdata, count=1)

    if updated == appdata:
        return 0

    if args.check:
        print(f"{args.appdata} is out of date; run scripts/update-appdata-releases.py",
              file=sys.stderr)
        return 1

    args.appdata.write_text(updated, encoding="utf-8")
    print(f"Updated {args.appdata}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
