#include "AST.h"
#include <sstream>
#include "boost-utils.h"

const Location Location::NONE(0, 0, 0, 0, std::make_shared<fs::path>(fs::path{}));

Location::Location(int firstLine, int firstCol, int lastLine, int lastCol,
			std::shared_ptr<fs::path> path)
		: first_line(firstLine), first_col(firstCol), last_line(lastLine),
		last_col(lastCol), path(std::move(path))
{
}


bool operator==(Location const& lhs, Location const& rhs){
	return
		lhs.firstLine()   == rhs.firstLine() &&
		lhs.firstColumn() == rhs.firstColumn() &&
		lhs.lastLine()    == rhs.lastLine() &&
		lhs.lastColumn()  == rhs.lastColumn() &&
		lhs.filePath()    == rhs.filePath();
}

bool operator != (Location const& lhs, Location const& rhs)
{
  return ! (lhs==rhs);
}

bool Location::isNone() const{
	return ((*this)==Location::NONE);
}

std::string Location::toRelativeString(const std::string &docPath) const{
	if(this->isNone()) return "location unknown";
	return "in file "+boostfs_uncomplete((*path), docPath).generic_string()+ ", "+"line " + std::to_string(this->firstLine());
}

bool Location::overlap(const Location &other) const {
    // None paths never overlap
    if (!this->path || !other.path) {
        return false;
    }
    if (isNone() || other.isNone()) {
        return false;
    }
    // different files never overlap
    if (this->filePath() != other.filePath()) {
        return false;
    }
    // is either of these fulle before / after?
    if (this->lastLine() < other.firstLine() || this->firstLine() > other.lastLine()) {
        return false;
    }
    // The lines _do_ overlap, check if the columns overlap
    if ((this->firstLine() == other.firstLine() && this->firstColumn() > other.lastColumn())
        ||(this->lastLine() == other.lastLine() && this->lastColumn() < other.firstColumn())) {
        return false;
    }
    // They have to overlap.
    return true;
}

std::ostream &operator<<(std::ostream &stream, const ASTNode &ast)
{
	ast.print(stream, "");
	return stream;
}

std::string ASTNode::dump(const std::string &indent) const
{
	std::ostringstream stream;
	print(stream, indent);
	return stream.str();
}
