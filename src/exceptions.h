#include <exception>

class RecursionException: public std::exception {
public:
	RecursionException(const char *recursiontype, const std::string &name)
		: rectype(recursiontype), name(name) {}
	virtual ~RecursionException() throw() {}
	virtual const char *what() const throw() {
		std::stringstream out;
		out << "ERROR: Recursion detected calling " << this->rectype << " '" << this->name << "'";
		return out.str().c_str();
  }
private:
	const char *rectype;
	const std::string name;
};
