#include "printutils.h"
#include <stdio.h>

QList<QString> print_messages_stack;
OutputHandlerFunc *outputhandler = NULL;
void *outputhandler_data = NULL;

void set_output_handler(OutputHandlerFunc *newhandler, void *userdata)
{
	outputhandler = newhandler;
	outputhandler_data = userdata;
}

void print_messages_push()
{
	print_messages_stack.append(QString());
}

void print_messages_pop()
{
	QString msg = print_messages_stack.takeLast();
	if (print_messages_stack.size() > 0 && !msg.isNull()) {
		if (!print_messages_stack.last().isEmpty())
			print_messages_stack.last() += "\n";
		print_messages_stack.last() += msg;
	}
}

void PRINT(const QString &msg)
{
	if (msg.isNull())
		return;
	if (print_messages_stack.size() > 0) {
		if (!print_messages_stack.last().isEmpty())
			print_messages_stack.last() += "\n";
		print_messages_stack.last() += msg;
	}
	PRINT_NOCACHE(msg);
}

void PRINT_NOCACHE(const QString &msg)
{
	if (msg.isNull())
		return;
	if (!outputhandler) {
		fprintf(stderr, "%s\n", msg.toUtf8().data());
	} else {
		outputhandler(msg, outputhandler_data);
	}
}

std::ostream &operator<<(std::ostream &os, const QFileInfo &fi) {
	os << std::hex << (fi.exists()?fi.lastModified().toTime_t():0) << "." << fi.size();
	return os; 
}
