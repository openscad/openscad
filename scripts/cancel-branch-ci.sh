#!/usr/bin/env bash
#
# Cancel in-progress GitHub Actions workflow runs for a branch.
#
# By default uses the current git branch. You can also pass --pr to target
# the head branch of an open pull request (defaults to the PR for the
# current branch when no number is given).
#
# Requires: gh (https://cli.github.com/), authenticated for the repo remote.
#
# Examples:
#   ./scripts/cancel-branch-ci.sh
#   ./scripts/cancel-branch-ci.sh --dry-run
#   ./scripts/cancel-branch-ci.sh --branch fix/my-feature
#   ./scripts/cancel-branch-ci.sh --pr 711
#   ./scripts/cancel-branch-ci.sh --pr

set -euo pipefail

usage() {
	cat <<'EOF'
Usage: cancel-branch-ci.sh [OPTIONS]

Cancel queued or in-progress GitHub Actions runs for a branch.

Options:
  -b, --branch NAME   Branch to cancel runs for (default: current branch)
  -p, --pr [NUMBER]   Use the PR head branch (NUMBER optional on a PR branch)
  -n, --dry-run       Print runs that would be cancelled, do not cancel
  -h, --help          Show this help

EOF
}

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

BRANCH=""
PR_MODE=0
PR_NUMBER=""
DRY_RUN=0

while [[ $# -gt 0 ]]; do
	case "$1" in
	-h | --help)
		usage
		exit 0
		;;
	-n | --dry-run)
		DRY_RUN=1
		shift
		;;
	-b | --branch)
		[[ $# -ge 2 ]] || {
			echo "error: --branch requires a value" >&2
			exit 1
		}
		BRANCH="$2"
		shift 2
		;;
	-p | --pr)
		PR_MODE=1
		if [[ $# -ge 2 && "$2" != -* ]]; then
			PR_NUMBER="$2"
			shift 2
		else
			shift
		fi
		;;
	*)
		echo "error: unknown option: $1" >&2
		usage >&2
		exit 1
		;;
	esac
done

if ! command -v gh >/dev/null 2>&1; then
	echo "error: gh CLI not found (install https://cli.github.com/)" >&2
	exit 1
fi

if ! gh auth status >/dev/null 2>&1; then
	echo "error: gh is not authenticated (run: gh auth login)" >&2
	exit 1
fi

OWNER_REPO="$(gh repo view --json nameWithOwner --jq .nameWithOwner)"

if [[ "$PR_MODE" -eq 1 ]]; then
	if [[ -n "$PR_NUMBER" ]]; then
		BRANCH="$(gh pr view "$PR_NUMBER" --repo "$OWNER_REPO" --json headRefName --jq .headRefName)"
	else
		BRANCH="$(gh pr view --repo "$OWNER_REPO" --json headRefName --jq .headRefName)"
	fi
elif [[ -z "$BRANCH" ]]; then
	if ! git rev-parse --git-dir >/dev/null 2>&1; then
		echo "error: not inside a git repository" >&2
		exit 1
	fi
	BRANCH="$(git rev-parse --abbrev-ref HEAD)"
	if [[ "$BRANCH" == "HEAD" ]]; then
		echo "error: detached HEAD; use --branch NAME or --pr" >&2
		exit 1
	fi
fi

mapfile -t RUNS < <(
	gh run list \
		--repo "$OWNER_REPO" \
		--branch "$BRANCH" \
		--limit 50 \
		--json databaseId,status,workflowName,displayTitle,headSha,createdAt \
		--jq '.[] | select(.status != "completed" and .status != "cancelled") |
			[.databaseId, .status, .workflowName, .displayTitle] | @tsv'
)

if [[ ${#RUNS[@]} -eq 0 ]]; then
	echo "No active workflow runs on branch '$BRANCH' ($OWNER_REPO)."
	exit 0
fi

echo "Branch: $BRANCH"
echo "Repo:   $OWNER_REPO"
echo "Active runs: ${#RUNS[@]}"
echo

cancelled=0
for entry in "${RUNS[@]}"; do
	IFS=$'\t' read -r run_id status workflow title <<<"$entry"
	printf '  [%s] %s — %s (run %s)\n' "$status" "$workflow" "$title" "$run_id"
	if [[ "$DRY_RUN" -eq 1 ]]; then
		continue
	fi
	if gh api --silent -X POST "/repos/${OWNER_REPO}/actions/runs/${run_id}/cancel" >/dev/null; then
		((cancelled += 1)) || true
	else
		echo "warning: failed to cancel run $run_id" >&2
	fi
done

if [[ "$DRY_RUN" -eq 1 ]]; then
	echo
	echo "Dry run: no runs were cancelled."
else
	echo
	echo "Cancelled $cancelled run(s)."
fi
