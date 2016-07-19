#include <boost/spirit/include/qi.hpp>

#include "util.h"

namespace libsvg {

double
parse_double(const std::string& number)
{
   std::string::const_iterator iter = number.begin(), end = number.end();
   
   boost::spirit::qi::real_parser<double, boost::spirit::qi::real_policies<double> > double_parser;
   
   double d = 0.0;
   
   bool result = boost::spirit::qi::parse(iter, end, double_parser, d);
   if(result && iter == end)
   {
      return d;
   }
   else
   {
      return 0;
   }
}

}
