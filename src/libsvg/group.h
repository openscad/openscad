#ifndef LIBSVG_GROUP_H
#define	LIBSVG_GROUP_H

#include "shape.h"

namespace libsvg {

class group : public shape {
protected:

public:
    group();
    group(const group& orig);
    virtual ~group();

    virtual bool is_container() { return true; }
    
    virtual void set_attrs(attr_map_t& attrs);
    virtual void dump();
    const std::string& get_name() const { return group::name; };
    
    static const std::string name;
};

}

#endif	/* LIBSVG_GROUP_H */

