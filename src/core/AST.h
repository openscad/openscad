#pragma once

#include <ostream>
#include <string>
#include <memory>
#include <filesystem>
#include <utility>
namespace fs = std::filesystem;

#include <string>

class Location
{

public:
  Location(int firstLine, int firstCol, int lastLine, int lastCol,
           std::shared_ptr<fs::path> path)
    : first_line(firstLine), first_col(firstCol), last_line(lastLine),
    last_col(lastCol), path(std::move(path)) {
  }

  [[nodiscard]] std::string fileName() const { return path ? path->generic_string() : ""; }
  [[nodiscard]] const fs::path& filePath() const { return *path; }
  [[nodiscard]] int firstLine() const { return first_line; }
  [[nodiscard]] int firstColumn() const { return first_col; }
  [[nodiscard]] int lastLine() const { return last_line; }
  [[nodiscard]] int lastColumn() const { return last_col; }
  [[nodiscard]] bool isNone() const;

  [[nodiscard]] std::string toRelativeString(const std::string& docPath) const;

  bool operator==(Location const& rhs);
  bool operator!=(Location const& rhs);

  static const Location NONE;
private:
  int first_line;
  int first_col;
  int last_line;
  int last_col;
  std::shared_ptr<fs::path> path;
};

class ASTNode
{
public:
  ASTNode(Location loc) : loc(std::move(loc)) {}
  virtual ~ASTNode() = default;

  virtual void print(std::ostream& stream, const std::string& indent) const = 0;

  [[nodiscard]] std::string dump(const std::string& indent) const;
  [[nodiscard]] const Location& location() const { return loc; }
  void setLocation(const Location& loc) { this->loc = loc; }

protected:
  Location loc;
};

std::ostream& operator<<(std::ostream& stream, const ASTNode& ast);
