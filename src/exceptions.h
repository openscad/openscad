#include <exception>

class RecursionException: public std::exception {
public:
	RecursionException(const char *recursiontype, const char *funcname)
		: rectype(recursiontype), funcname(funcname) {}
	virtual const char *what() const throw() {
		std::stringstream out;
		out << "ERROR: Recursion detected calling " << this->rectype << " '" << this->funcname << "'";
		return out.str().c_str();
  }
private:
	const char *rectype;
	const char *funcname;
};
