#include <exception>

class RecursionException: public std::runtime_error {
public:
	static RecursionException create(const char *recursiontype, const std::string &name) {
		std::stringstream out;
		out << "ERROR: Recursion detected calling " << recursiontype << " '" << name << "'";
		return RecursionException(out.str());
	}
	virtual ~RecursionException() throw() {}

private:
	RecursionException(const std::string &what_arg) : std::runtime_error(what_arg) {}
};
