#pragma once

class Location {
public:
	Location(int firstLine, int firstCol, int lastLine, int lastCol)
		: first_line(firstLine), first_col(firstCol), last_line(lastLine), last_col(lastCol) {
	}

	int firstLine() const { return first_line; }
	int firstColumn() const { return first_col; }
	int lastLine() const { return last_line; }
	int lastColumn() const { return last_col; }


	static const Location NONE;
;
private:
	int first_line;
	int first_col;
	int last_line;
	int last_col;
};

class ASTNode
{
public:
  ASTNode(const Location &loc) : loc(loc) {}
	virtual ~ASTNode() {}

	const Location &location() const { return loc; }
	void setLocation(const Location &loc) { this->loc = loc; }

protected:
	Location loc;
};
