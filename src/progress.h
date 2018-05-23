#pragma once

// Reset to 0 in _prep() and increased for each Node instance in progress_prepare()
extern int progress_report_count;

extern void (*progress_report_f)(const class AbstractNode*, void*, int);
extern void *progress_report_userdata;

typedef void (*ProgressReportCallback)(const class AbstractNode *node, void *userdata, int mark);

void progress_report_prep(AbstractNode *root, ProgressReportCallback, void *userdata);
void progress_report_fin();
void progress_update(const AbstractNode *node, int mark);

class ProgressCancelException { };
