#include "progress.h"
#include "node.h"

int progress_report_count;
void (*progress_report_f)(const class AbstractNode *, void *, int);
void *progress_report_userdata;

void progress_report_prep(AbstractNode *root, void (*f)(const class AbstractNode *node, void *userdata, int mark), void *userdata)
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

void progress_update(const AbstractNode *node, int mark)
{
  if (progress_report_f) progress_report_f(node, progress_report_userdata, mark);
}

