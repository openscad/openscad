#pragma once

#include <string>
#include <memory.h>
#include <annotation.h>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <string>

class Location {

public:
	Location(int firstLine, int firstCol, int lastLine, int lastCol,
			std::shared_ptr<fs::path> path)
		: first_line(firstLine), first_col(firstCol), last_line(lastLine),
		last_col(lastCol), path(std::move(path)) {
	}

	std::string fileName() const { return path ? path->generic_string() : ""; }
	fs::path filePath() const { return *path; }
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
	std::shared_ptr<fs::path> path;
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

	virtual void addAnnotations(AnnotationList *annotations);
	virtual bool hasAnnotations() const;
	virtual const Annotation *annotation(const std::string &name) const;

protected:
	Location loc;
	AnnotationMap annotations;
};

std::ostream &operator<<(std::ostream &stream, const ASTNode &ast);
