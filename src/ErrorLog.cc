#include "ErrorLog.h"
#include "printutils.h"

ErrorLog::ErrorLog(QWidget *parent) : QWidget(parent)
{
	setupUi(this);
}

ErrorLog::~ErrorLog()
{
}

void ErrorLog::toErrorLog(const Message &log_msg)
{
	std::cout<<log_msg.file<<"--"<<log_msg.line<<"--"<<log_msg.msg<<"--"<<log_msg.msg_id<<std::endl;
	// PRINTDB("File:%s, Location:%i, Message:%s, Msg-Id:%i",log_msg.file%log_msg.line%log_msg.msg%log_msg.msg_id);
}
