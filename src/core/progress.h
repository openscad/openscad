#pragma once

#include <memory>

class AbstractNode;

// Reset to 0 in _prep() and increased for each Node instance in progress_prepare()
extern int progress_report_count;

extern void (*progress_report_f)(const std::shared_ptr<const AbstractNode> &, void *, int);
extern void *progress_report_userdata;

void progress_report_prep(const std::shared_ptr<AbstractNode> &root, void (*f)(const std::shared_ptr<const AbstractNode> &node, void *userdata, int mark), void *userdata);
void progress_report_fin();
void progress_update(const std::shared_ptr<const AbstractNode> &node, int mark);
// CGALUtils::applyUnion3D may process nodes out of order, so allow for an increment instead of tracking exact node
void progress_tick();

class ProgressCancelException
{
};
