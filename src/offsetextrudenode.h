#pragma once

#include "node.h"
#include "value.h"
#include "clipper-utils.h"

class OffsetExtrudeNode : public AbstractPolyNode
{
public:
    VISITABLE();
    OffsetExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
        convexity = 0;
        fn = fs = fa = 0;
        delta = height = 1;
        slices = 1;
        center = chamfer = false;
        miter_limit = 1000000.0;
        join_type = ClipperLib::jtRound;
    }
    virtual std::string toString() const;
    virtual std::string name() const { return "offset_extrude"; }

    bool chamfer, center;
    double delta, height, fn, fs, fa;
    int slices, convexity;
    double miter_limit; // currently fixed high value to disable chamfers with jtMiter
    ClipperLib::JoinType join_type;
};
