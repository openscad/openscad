#pragma once

#include <string>
#include <vector>

/*
   Running several copies of ourselves and waiting for them.

   Deliberately tiny: the only caller renders animation frames in parallel, which
   needs "start all of these, wait for all of them, tell me if any failed" and
   nothing else. No streams, no pipes, no partial waits - a worker writes its
   frames to files and reports success through its exit status.

   Hand-rolled over posix_spawn and CreateProcess rather than taking a dependency
   on boost::process, which would have to be satisfied by every platform's CI and
   by every distribution packaging OpenSCAD.
 */
namespace Subprocess {

/*!
   Starts every command concurrently, waits for all of them, and returns true only
   if every one exited zero. Each `commands[i][0]` is the executable path; the rest
   are arguments, passed through without shell interpretation. Children inherit the
   working directory, environment and standard streams of the caller.

   Waits for every child even after one fails, so none are left orphaned.
 */
bool runAllAndWait(const std::vector<std::vector<std::string>>& commands);

}  // namespace Subprocess
