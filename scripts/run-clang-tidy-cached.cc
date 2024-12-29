#if 0  // Invoke with /bin/sh or simply add executable bit on this file on Unix.
B=${0%%.cc}; [ "$B" -nt "$0" ] || c++ -std=c++17 -o"$B" "$0" && exec "$B" "$@";
#endif
// Copyright 2023 Henner Zeller <h.zeller@acm.org>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Location: https://github.com/hzeller/dev-tools (2024-09-19)

// Script to run clang-tidy on files in a bazel project while caching the
// results as clang-tidy can be pretty slow. The clang-tidy output messages
// are content-addressed in a hash(cc-file-content) cache file.
// Should run on any system with a shell that provides 2>/dev/null redirect.
//
// Invocation without parameters simply uses the .clang-tidy config to run on
// all *.{cc,h} files. Additional parameters passed to this script are passed
// to clang-tidy as-is. Typical use could be for instance
//   run-clang-tidy-cached.cc --checks="-*,modernize-use-override" --fix
//
// Note: useful environment variables to configure are
//  CLANG_TIDY = binary to run; default would just be clang-tidy.
//  CACHE_DIR  = where to put the cached content; default ~/.cache

// This file shall be c++17 self-contained; not using any re2 or absl niceties.
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cinttypes>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <regex>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

// These are the configuration defaults.
// Dont' change anything here, override for your project below in kConfig.
struct ConfigValues {
  // Prefix for the cache to write to. If empty, use name of toplevel directory.
  // Typical would be name of project with underscore suffix.
  std::string_view cache_prefix;

  // Directory to start recurse for sources createing a file list.
  // Some projects e.g. start at "src/".
  std::string_view start_dir;

  // Regular expression matching files that should be included in file list.
  std::string_view file_include_re = ".*";

  // Regular expxression matching files that should be excluded from file list.
  // If searching from toplevel, make sure to include at least ".git/".
  std::string_view file_exclude_re = "^(\\.git|\\.github|build)/";

  // A file in the toplevel of the project that should exist, typically
  // something used to set up the build environment, such as MODULE.bazel,
  // CMakeLists.txt or similar.
  // It is used two-fold:
  //   - As validation if this script is started in the correct directory.
  //   - To take changes there into account as that build file might provide
  //     additional dependencies which might change clang-tidy outcome.
  //     (if revisit_brokenfiles_if_build_config_newer is on).
  // (Default configuration: just .clang-tidy as this should always be there)
  std::string_view toplevel_build_file = ".clang-tidy";

  // Bazel projects have some complicated paths where generated and external
  // sources reside. Setting this to true will tell this script canonicalize
  // these in the output. Set to true if this is a bazel project.
  bool is_bazel_project = false;

  // If the compilation DB or toplevel_build_file changed in timestamp, it
  // might be worthwhile revisiting sources that previously had issues.
  // This flag enables that.
  //
  // It is good to set once the project is 'clean' and there are only a
  // few problematic sources to begin with, otherwise every update of the
  // compilation DB will re-trigger revisiting all of them.
  bool revisit_brokenfiles_if_build_config_newer = true;

  // Revisit a source file if any of its include files changed content. Say
  // foo.cc includes bar.h. Reprocess foo.cc with clang-tidy when bar.h changed,
  // even if foo.cc is unchanged. This will find issues in which foo.cc relies
  // on something bar.h provides.
  // Usually good to keep on, but it can result in situations in which a header
  // that is included by a lot of other files results in lots of reprocessing.
  bool revisit_if_any_include_changes = true;
};

// --------------[ Project-specific configuration ]--------------
static constexpr ConfigValues kConfig = {
  .cache_prefix = "OpenSCAD_",
  .start_dir = "src/",
  .file_exclude_re = "src/ext/",
  .revisit_brokenfiles_if_build_config_newer = false,  // not clean yet.
};
// --------------------------------------------------------------

// Files that look like relevant include files.
inline bool IsIncludeExtension(const std::string &extension) {
  for (const std::string_view e : {".h", ".hpp", ".hxx", ".inl"}) {
    if (extension == e) return true;
  }
  return false;
}

// Filter for source files to be considered.
inline bool ConsiderExtension(const std::string &extension) {
  for (const std::string_view e : {".cc", ".cpp", ".cxx"}) {
    if (extension == e) return true;
  }
  return IsIncludeExtension(extension);
}

// Configuration of clang-tidy itself.
static constexpr std::string_view kClangConfigFile = ".clang-tidy";
static constexpr std::string_view kExtraArgs[] = {
    "-Wno-unknown-pragmas", "-Wno-unknown-warning-option"};

namespace {

namespace fs = std::filesystem;
using file_time = std::filesystem::file_time_type;
using ReIt = std::sregex_iterator;
using hash_t = uint64_t;
using filepath_contenthash_t = std::pair<fs::path, hash_t>;

// Some helpers
std::string GetContent(FILE *f) {
  std::string result;
  if (!f) return result;  // ¯\_(ツ)_/¯ best effort.
  char buf[4096];
  while (const size_t r = fread(buf, 1, sizeof(buf), f)) {
    result.append(buf, r);
  }
  fclose(f);
  return result;
}

std::string GetContent(const fs::path &f) {
  FILE *file_to_read = fopen(f.string().c_str(), "r");
  if (!file_to_read) {
    fprintf(stderr, "%s: can't open: %s\n", f.string().c_str(),
            strerror(errno));
    return {};  // ¯\_(ツ)_/¯ best effort.
  }
  return GetContent(file_to_read);
}

std::string GetCommandOutput(const std::string &prog) {
  return GetContent(popen(prog.c_str(), "r"));  // NOLINT
}

hash_t hashContent(const std::string &s) { return std::hash<std::string>()(s); }
std::string ToHex(uint64_t value, int show_lower_nibbles = 16) {
  char out[16 + 1];
  snprintf(out, sizeof(out), "%016" PRIx64, value);
  return out + (16 - show_lower_nibbles);
}

// Mapping filepath_contenthash_t to an actual location in the file system.
class ContentAddressedStore {
 public:
  explicit ContentAddressedStore(const fs::path &project_base_dir)
      : content_dir(project_base_dir / "contents") {
    fs::create_directories(content_dir);
  }

  // Given filepath contenthash, return the path to read/write from.
  fs::path PathFor(const filepath_contenthash_t &c) const {
    // Name is human readable, the content hash makes it unique.
    std::string name_with_contenthash = c.first.filename().string();
    name_with_contenthash.append("-").append(ToHex(c.second));
    return content_dir / name_with_contenthash;
  }

  std::string GetContentFor(const filepath_contenthash_t &c) const {
    return GetContent(PathFor(c));
  }

  // Check if this needs to be recreated, either because it is not there,
  // or is not empty and does not fit freshness requirements.
  bool NeedsRefresh(const filepath_contenthash_t &c,
                    file_time min_freshness) const {
    const fs::path content_hash_file = PathFor(c);
    if (!fs::exists(content_hash_file)) return true;

    // If file exists but is broken (i.e. has a non-zero size with messages),
    // consider recreating if if older than compilation db.
    const bool timestamp_trigger =
      kConfig.revisit_brokenfiles_if_build_config_newer &&
      (fs::file_size(content_hash_file) > 0 &&
       fs::last_write_time(content_hash_file) < min_freshness);
    return timestamp_trigger;
  }

 private:
  const fs::path content_dir;
};

class ClangTidyRunner {
 public:
  ClangTidyRunner(const std::string &cache_prefix, int argc, char **argv)
      : clang_tidy_(getenv("CLANG_TIDY") ?: "clang-tidy"),
        clang_tidy_args_(AssembleArgs(argc, argv)) {
    project_cache_dir_ = AssembleProjectCacheDir(cache_prefix);
  }

  const fs::path &project_cache_dir() const { return project_cache_dir_; }

  // Given a work-queue in/out-file, process it. Using system() for portability.
  // Empties work_queue.
  void RunClangTidyOn(ContentAddressedStore &output_store,
                      std::list<filepath_contenthash_t> *work_queue) {
    if (work_queue->empty()) return;
    const int kJobs = std::thread::hardware_concurrency();
    std::cerr << work_queue->size() << " files to process...";

    const bool print_progress = isatty(STDERR_FILENO);
    if (!print_progress) std::cerr << "\n";

    std::mutex queue_access_lock;
    auto clang_tidy_runner = [&]() {
      for (;;) {
        filepath_contenthash_t work;
        {
          const std::lock_guard<std::mutex> lock(queue_access_lock);
          if (work_queue->empty()) return;
          if (print_progress) {
            fprintf(stderr, "%5d\b\b\b\b\b", (int)(work_queue->size()));
          }
          work = work_queue->front();
          work_queue->pop_front();
        }
        const fs::path final_out = output_store.PathFor(work);
        const std::string tmp_out = final_out.string() + ".tmp";
        // Putting the file to clang-tidy early in the command line so that
        // it is easy to find with `ps` or `top`.
        const std::string command = clang_tidy_ + " '" + work.first.string() +
                                    "'" + clang_tidy_args_ + "> '" + tmp_out +
                                    "' 2>/dev/null";
        const int r = system(command.c_str());
#ifdef WIFSIGNALED
        // NOLINTBEGIN
        if (WIFSIGNALED(r) &&
            (WTERMSIG(r) == SIGINT || WTERMSIG(r) == SIGQUIT)) {
          break;  // got Ctrl-C
        }
        // NOLINTEND
#endif
        RepairFilenameOccurences(tmp_out, tmp_out);
        fs::rename(tmp_out, final_out);  // atomic replacement
      }
    };

    std::vector<std::thread> workers;
    for (auto i = 0; i < kJobs; ++i) {
      workers.emplace_back(clang_tidy_runner);  // NOLINT
    }
    for (auto &t : workers) t.join();
    if (print_progress) {
      fprintf(stderr, "     \n");  // Clean out progress counter.
    }
  }

 private:
  static fs::path GetCacheBaseDir() {
    if (const char *from_env = getenv("CACHE_DIR")) return fs::path{from_env};
    if (const char *home = getenv("HOME")) {
      if (auto cdir = fs::path(home) / ".cache/"; fs::exists(cdir)) return cdir;
    }
    return fs::path{getenv("TMPDIR") ?: "/tmp"};
  }

  static std::string AssembleArgs(int argc, char **argv) {
    std::string result = " --quiet";
    result.append(" '--config-file=").append(kClangConfigFile).append("'");
    for (const std::string_view arg : kExtraArgs) {
      result.append(" --extra-arg='").append(arg).append("'");
    }
    for (int i = 1; i < argc; ++i) {
      result.append(" '").append(argv[i]).append("'");
    }
    return result;
  }

  fs::path AssembleProjectCacheDir(const std::string &cache_prefix) const {
    const fs::path cache_dir = GetCacheBaseDir() / "clang-tidy";

    // Use major version as part of name of our configuration specific dir.
    auto version = GetCommandOutput(clang_tidy_ + " --version");
    if (version.empty()) {
      std::cerr << "Could not invoke " << clang_tidy_ << "; is it in PATH ?\n";
      exit(EXIT_FAILURE);
    }
    std::smatch version_match;
    const std::string major_version =
      std::regex_search(version, version_match, std::regex{"version ([0-9]+)"})
        ? version_match[1].str()
        : "UNKNOWN";

    // Make sure directory filename depends on .clang-tidy content.
    hash_t cache_unique_id = hashContent(version + clang_tidy_args_);
    cache_unique_id ^= hashContent(GetContent(kClangConfigFile));
    return cache_dir / fs::path(cache_prefix + "v" + major_version + "_" +
                                ToHex(cache_unique_id, 8));
  }

  // Fix filename paths found in logfiles that are not emitted relative to
  // project root in the log - remove that prefix.
  // (bazel has its own, so if this is bazel, also bazel-specific fix up that).
  static void RepairFilenameOccurences(const fs::path &infile,
                                       const fs::path &outfile) {
    static const std::regex sFixPathsRe = []() {
      std::string canonicalize_expr = "(^|\\n)(";  // fix names at start of line
      if (kConfig.is_bazel_project) {
        auto bzroot = GetCommandOutput("bazel info execution_root 2>/dev/null");
        if (!bzroot.empty()) {
          bzroot.pop_back();  // remove newline.
          canonicalize_expr += bzroot + "/|";
        }
      }
      canonicalize_expr += fs::current_path().string() + "/";  // $(pwd)/
      canonicalize_expr +=
        ")?(\\./)?";  // Some start with, or have a trailing ./
      return std::regex{canonicalize_expr};
    }();

    const auto in_content = GetContent(infile);
    std::fstream out_stream(outfile, std::ios::out);
    out_stream << std::regex_replace(in_content, sFixPathsRe, "$1");
  }

  const std::string clang_tidy_;
  const std::string clang_tidy_args_;
  fs::path project_cache_dir_;
};

class FileGatherer {
 public:
  FileGatherer(ContentAddressedStore &store, std::string_view search_dir)
    : store_(store), root_dir_(search_dir.empty() ? "." : search_dir) {}

  // Find all the files we're interested in, and assemble a list of
  // paths that need refreshing.
  std::list<filepath_contenthash_t> BuildWorkList(file_time min_freshness) {
    // Gather all *.cc and *.h files; remember content hashes of includes.
    static const std::regex include_re(std::string{kConfig.file_include_re});
    static const std::regex exclude_re(std::string{kConfig.file_exclude_re});
    std::map<std::string, hash_t> header_hashes;
    for (const auto &dir_entry : fs::recursive_directory_iterator(root_dir_)) {
      const fs::path &p = dir_entry.path().lexically_normal();
      if (!fs::is_regular_file(p)) {
        continue;
      }
      const std::string file = p.string();
      if (!kConfig.file_include_re.empty() &&
          !std::regex_search(file, include_re)) {
        continue;
      }
      if (!kConfig.file_exclude_re.empty() &&
          std::regex_search(file, exclude_re)) {
        continue;
      }
      const auto extension = p.extension();
      if (ConsiderExtension(extension)) {
        files_of_interest_.emplace_back(p, 0);  // <- hash to be filled later.
      }
      // Remember content hash of header, so that we can make changed headers
      // influence the hash of a file including this.
      if (kConfig.revisit_if_any_include_changes &&
          IsIncludeExtension(extension)) {
        // Since the files might be included sloppily without prefix path,
        // just keep track of the basename (but since there might be collisions,
        // accomodate all of them by xor-ing the hashes).
        const std::string just_basename = p.filename();
        header_hashes[just_basename] ^= hashContent(GetContent(p));
      }
    }
    std::cerr << files_of_interest_.size() << " files of interest.\n";

    // Create content hash address for the cache and build list of work items.
    // If we want to revisit if headers changed, make hash dependent on them.
    std::list<filepath_contenthash_t> work_queue;
    const std::regex inc_re(
        R"""(#\s*include\s+"([0-9a-zA-Z_/-]+\.[a-zA-Z]+)")""");
    for (filepath_contenthash_t &work_file : files_of_interest_) {
      const auto content = GetContent(work_file.first);
      work_file.second = hashContent(content);
      if (kConfig.revisit_if_any_include_changes) {
        // Update the hash with all the hashes from all include files.
        for (ReIt it(content.begin(), content.end(), inc_re); it != ReIt();
             ++it) {
          const std::string &header_path = (*it)[1].str();
          const std::string header_basename = fs::path(header_path).filename();
          const auto found = header_hashes.find(header_basename);
          if (found != header_hashes.end()) {
            work_file.second ^= found->second;
          }
        }
      }
      // Recreate if we don't have it yet or if it contains findings but is
      // older than build environment. Maybe something got fixed: revisit file.
      if (store_.NeedsRefresh(work_file, min_freshness)) {
        work_queue.emplace_back(work_file);
      }
    }
    return work_queue;
  }

  // Tally up findings for files of interest and assemble in one file.
  // (BuildWorkList() needs to be called first).
  size_t CreateReport(const fs::path &cache_dir,
                      std::string_view symlink_detail,
                      std::string_view symlink_summary) const {
    // Make it possible to keep independent reports for different invocation
    // locations (e.g. two checkouts of the same project) using the same cache.
    const std::string suffix = ToHex(hashContent(fs::current_path().string()));
    const fs::path tidy_outfile = cache_dir / ("tidy.out-" + suffix);
    const fs::path tidy_summary = cache_dir / ("tidy-summary.out-" + suffix);

    // Assemble the separate outputs into a single file. Tally up per-check
    const std::regex check_re("(\\[[a-zA-Z.-]+\\])\n");
    std::map<std::string, int> checks_seen;
    std::ofstream tidy_collect(tidy_outfile);
    for (const filepath_contenthash_t &f : files_of_interest_) {
      const auto tidy = store_.GetContentFor(f);
      if (!tidy.empty()) tidy_collect << f.first.string() << ":\n" << tidy;
      for (ReIt it(tidy.begin(), tidy.end(), check_re); it != ReIt(); ++it) {
        checks_seen[(*it)[1].str()]++;
      }
    }
    tidy_collect.close();
    std::error_code ignored_error;
    fs::remove(symlink_detail, ignored_error);
    fs::create_symlink(tidy_outfile, symlink_detail, ignored_error);

    // Report headline.
    if (checks_seen.empty()) {
      std::cerr << "No clang-tidy complaints. 😎\n";
    } else {
      std::cerr << "Details: " << symlink_detail << "\n"
                << "Summary: " << symlink_summary << "\n";
      std::cerr << "---- Summary ----\n";
    }

    // Produce a ordered-by-count report.
    using check_count_t = std::pair<std::string, int>;
    std::vector<check_count_t> by_count(checks_seen.begin(), checks_seen.end());
    std::stable_sort(by_count.begin(), by_count.end(),
                     [](const check_count_t &a, const check_count_t &b) {
                       return b.second < a.second;  // reverse count
                     });

    FILE *summary_file = fopen(tidy_summary.c_str(), "wb");
    for (const auto &counts : by_count) {
      fprintf(stdout, "%5d %s\n", counts.second, counts.first.c_str());
      fprintf(summary_file, "%5d %s\n", counts.second, counts.first.c_str());
    }
    fclose(summary_file);

    fs::remove(symlink_summary, ignored_error);
    fs::create_symlink(tidy_summary, symlink_summary, ignored_error);

    return checks_seen.size();
  }

 private:
  ContentAddressedStore &store_;
  const std::string root_dir_;
  std::vector<filepath_contenthash_t> files_of_interest_;
};
}  // namespace

int main(int argc, char *argv[]) {
  // Test that key files exist and remember their last change.
  if (!fs::exists(kClangConfigFile)) {
    std::cerr << "Need a " << kClangConfigFile << " config file.\n";
    return EXIT_FAILURE;
  }

  std::error_code ec;
  const auto toplevel_build_ts =
      fs::last_write_time(kConfig.toplevel_build_file, ec);
  if (ec.value() != 0) {
    std::cerr << "Script needs to be executed in toplevel project dir.\n";
    return EXIT_FAILURE;
  }

  const auto compdb_ts = fs::last_write_time("compile_commands.json", ec);
  if (ec.value() != 0) {
    std::cerr << "No compilation db compile_commands.json found; "
              << "create that first. For cmake projects, often simply\n"
              << "\tln -s build/compile_commands.json .\n";
    return EXIT_FAILURE;
  }

  const auto build_env_latest_change = std::max(toplevel_build_ts, compdb_ts);

  std::string cache_prefix{kConfig.cache_prefix};
  if (cache_prefix.empty()) {
    // Cache prefix not set, choose name of directory
    cache_prefix = fs::current_path().filename().string() + "_";
  }
  ClangTidyRunner runner(cache_prefix, argc, argv);
  ContentAddressedStore store(runner.project_cache_dir());
  std::cerr << "Cache dir " << runner.project_cache_dir() << "\n";

  FileGatherer cc_file_gatherer(store, kConfig.start_dir);
  auto work_list = cc_file_gatherer.BuildWorkList(build_env_latest_change);

  // Now the expensive part...
  runner.RunClangTidyOn(store, &work_list);

  const std::string detailed_report = cache_prefix + "clang-tidy.out";
  const std::string summary = cache_prefix + "clang-tidy.summary";
  const size_t tidy_count = cc_file_gatherer.CreateReport(
    runner.project_cache_dir(), detailed_report, summary);

  return tidy_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
