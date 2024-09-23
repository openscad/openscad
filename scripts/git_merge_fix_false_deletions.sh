#!/usr/bin/env bash

# This script is meant to help with updating old PRs.
# Mainly any OpenSCAD branch which has not been updated since 2022-02-06
# It is intended to be run on a git repo where a merge (merging master into the current, outdated branch)
# would result in conflicts which show as "deleted by them:", or "(modified/deleted)".
#
# Of course this script will not exist in older branches, where it will need to be used.
# So it should be manually downloaded and put onto the path or into /scripts/
#
# Due to how git DOES NOT track file moves/renames, in addition to how it determines file "similarity" (by # of exactly matched lines in a file),
# it makes it very difficult to merge master into branches created before 2 significant events in the OpenSCAD repo:
#   1) A large code style reformatting (PR #4095 on 2022-02-06), and
#   2) A large restructuring of the source tree (PRs #4159 on 2022-03-06, #4161 on 2022-03-12)
#
# When attempting to merge master into such PR branches, the files are wrongly detected by Git as (modified/deleted).
# That is, modified locally (fine), but deleted in the remote master (WRONG).

#### THE PROBLEM ####
# Git doesn't track the renaming of files.
# Instead it attempts to detect them after the fact, through countless semi-functional hacks.
# This detection fails in our particular situation, making a proper 3-way merge extremely difficult.
#
#### THE SOLUTION ####
# This script goes through the list of conflicting files, determines which ones were presumed "deleted by them",
# properly follows the renames to determine the final destination path, and then rewrites the git index
# in such a way that "git mergetool" will detect them as "both modified".
#
#### CAVEATS and SHORTCOMINGS ####
# - This script doesn't work the other way around (for files renamed/moved in the current branch but only modified in master).
# - Only designed for merge, not sure about rebase
# - There is not currently any error checking or backup mechanism built into the script, so please use with caution
#

# defaults
BASE=origin/master
BEAUTIFY="yes"
ASK=1

for PARAM in "$@"
do
  KEY="${PARAM%%=*}"
  if [[ "$PARAM" == *"="* ]]; then
    VALUE="${PARAM#*=}"
  else
    VALUE=""
  fi
  if [ "$KEY" == "--beautify" ]; then
    BEAUTIFY="${VALUE:-yes}"
  elif [ "$KEY" == "--base=" ]; then
    BASE="$VALUE"
    exit
  elif [ "$PARAM" == "-Y" ]; then
    ASK=0
  elif [ "$PARAM" == "-h" ]; then
    SCRIPT=$(basename "$0")
    echo "Usage: $SCRIPT [--base=<remote>/<branch>]  [--beautify=yes|no] [-Y]"
    echo
    echo "  --base        which branch will be merged into our current branch.  If omitted, defaults to \"origin/master\""
    echo "  --beautify    whether or not to run scripts/beautify.sh prior to merge.  This should make conflict resolution easier, as it will match indentation style etc. with the base branch, prior to merge.  Defaults to \"yes\""
    echo "  -Y            Don't ask for confirmation to continue"
    exit
  fi
done

echo -n "This script is about to "
if [ "$BEAUTIFY" == "yes" ]; then
  echo -n "call scripts/beautify.sh, commit those changes, and then "
fi
echo "begin a git merge of $BASE."
echo
if (($ASK)); then
  echo "Before you continue make sure:"
  echo "  - The merge has not be started yet.  If one has been started, it would need to be aborted."
  echo "  - The repo has no uncommitted changes."
  echo "  - This script was called from the top level directory of the local git repo"
  echo
  read -p "Are you sure you want to continue? Y/N" -n 1 -r
  echo
  if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborted"
    exit 1
  fi
fi


if [ "$BEAUTIFY" == "yes" ]; then
  # Get the uncrusify settings and script from the branch being merged
  git checkout ${BASE} -- scripts/beautify.sh .uncrustify.cfg
  scripts/beautify.sh --diffbase=${BASE}
  # restore original files
  git reset scripts/beautify.sh .uncrustify.cfg && git checkout scripts/beautify.sh .uncrustify.cfg
  git add src
  git commit -m "Automated uncrustify run by $SCRIPT before merge, using settings from $BASE"
fi

# initiate merge
git merge $BASE

ANCESTOR=$(git merge-base HEAD "$BASE")
echo -n "Using common ancestor: "
git -P show -s --pretty='format:%C(auto)%H (%an, %as) %s%n    %b%n' $ANCESTOR

FILES=$(git diff --name-only --diff-filter=U) # get a clean list of just the files which are (U)nmerged, (because they have conflicts)

echo
echo "Files modified between current branch and $BASE:"
while IFS= read -r FILE; do
  SRC="${FILE}"
  echo -n "  ${SRC}"
  DELETED_IN=$(git -P log $BASE -M --follow --diff-filter=D --pretty=format:"%H" -- $SRC)
  # follow deletes (possibly a chain of them) until latest path is reached
  while [[ $DELETED_IN ]]; do
    #echo -n "deleted in "
    #git -P show -s --pretty='format:%C(auto)%H (%an, %as) %s%n' $DELETED_IN
    STAT_LINE=$(git -P show --name-status $DELETED_IN | grep -F $SRC)
    REGEX="^[[:space:]]*R[0-9]+[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]*$"
    if [[ $STAT_LINE =~ $REGEX ]]; then
      SRC="${BASH_REMATCH[1]}"
      DST="${BASH_REMATCH[2]}"
      echo -n "  ==>  ${DST}"
    else
      echo -n "  ==>  (actually deleted?)"
      DST=""
      break
    fi
    SRC="${DST}"
    DELETED_IN=$(git -P log $BASE -M --follow --diff-filter=D --pretty=format:"%H" -- $SRC)
  done
  echo
  if [[ $DST ]]; then
    mkdir -p $(dirname $DST)
    # have to clear index entry before updating, with 0 file mode, and all 0's SHA1
    echo "0 0000000000000000000000000000000000000000	${DST}" > .index-info.tmp
    git ls-files --stage ${FILE} | sed "s^${FILE}^${DST}^g" >> .index-info.tmp
    git ls-files --stage ${DST} | sed "s^ 0\t^ 3\t^g" >> .index-info.tmp
    git update-index --index-info < .index-info.tmp # Repair the conflict
    git rm ${FILE} # remove local old-named file
    DST="" # clear DST for next file
  fi
done <<< "$FILES"
rm .index-info.tmp
echo
