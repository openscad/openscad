#pragma once

#include <QObject>
#include "memory.h"

class CSGIF_Worker : public QObject
{
	Q_OBJECT;
public:
	CSGIF_Worker();
	virtual ~CSGIF_Worker();

public slots:
	void start(const class Tree &tree);

protected slots:
	void work();

signals:
	void done(shared_ptr<const class Geometry>);

protected:

	class QThread *thread;
	const class Tree *tree;
};
