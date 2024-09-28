#include "core/progress.h"

#include <memory>
#include "core/node.h"

int progress_report_count;
int progress_mark_;
void (*progress_report_f)(const std::shared_ptr<const AbstractNode> &, void *, int);
void *progress_report_userdata;

void progress_report_prep(const std::shared_ptr<AbstractNode> &root, void (*f)(const std::shared_ptr<const AbstractNode> &node, void *userdata, int mark), void *userdata)
{
  progress_report_count = 0;
  progress_report_f = f;
  progress_report_userdata = userdata;
  root->progress_prepare();
}

void progress_report_fin()
{
  progress_report_count = 0;
  progress_report_f = nullptr;
  progress_report_userdata = nullptr;
}

void progress_update(const std::shared_ptr<const AbstractNode> &node, int mark)
{
  if (progress_report_f) {
    progress_mark_ = mark;
    progress_report_f(node, progress_report_userdata, progress_mark_);
  }
}

void progress_tick()
{
  if (progress_report_f) progress_report_f(std::shared_ptr<const AbstractNode>(), progress_report_userdata, ++progress_mark_);
}
