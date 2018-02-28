#pragma once

#include <string>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class Location {

public:
	Location(int firstLine, int firstCol, int lastLine, int lastCol,
			fs::path path)
		: first_line(firstLine), first_col(firstCol), last_line(lastLine),
		last_col(lastCol), path(path) {
	}

	std::string fileName() const { return path.generic_string(); }
	int firstLine() const { return first_line; }
	int firstColumn() const { return first_col; }
	int lastLine() const { return last_line; }
	int lastColumn() const { return last_col; }


	static const Location NONE;
private:
	int first_line;
	int first_col;
	int last_line;
	int last_col;
	fs::path path;
};

class ASTNode
{
public:
	ASTNode(const Location &loc) : loc(loc) {}
	virtual ~ASTNode() {}

	virtual void print(std::ostream &stream, const std::string &indent) const = 0;

	std::string dump(const std::string &indent) const;
	const Location &location() const { return loc; }
	void setLocation(const Location &loc) { this->loc = loc; }

protected:
	Location loc;
};

std::ostream &operator<<(std::ostream &stream, const ASTNode &ast);
