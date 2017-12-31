#ifndef LIBSVG_GROUP_H
#define	LIBSVG_GROUP_H

#include "shape.h"

namespace libsvg {

class group : public shape {
protected:

public:
    group();
    group(const group& orig);
    ~group();

    bool is_container() override { return true; }
    
    void set_attrs(attr_map_t& attrs) override;
    void dump() override;
    const std::string& get_name() const override { return group::name; };
    
    static const std::string name;
};

}

#endif	/* LIBSVG_GROUP_H */

