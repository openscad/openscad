
#include "LibraryInfoDialog.h"

#include <QString>
#include <QStringRef>
#include <QTextEdit>
#include "LibraryInfo.h"

LibraryInfoDialog::LibraryInfoDialog(const QString& rendererInfo)
{
    setupUi(this);
    connect(this->okButton, SIGNAL(clicked()), this, SLOT(accept()));
    update_library_info(rendererInfo);
}

LibraryInfoDialog::~LibraryInfoDialog()
{

}

void LibraryInfoDialog::update_library_info(const QString& rendererInfo)
{
    //Get library infos
    QString info(LibraryInfo::info().c_str());
    info += rendererInfo;

    //Parse infos and make it html
    info = info.replace("\n", "<br/>");

    auto end = false;
    int startIndex = 0;
    while (!end) {
			int endIndex = info.indexOf(":", startIndex);
			if(endIndex != -1) {
				//add bold to property name
				info = info.insert(startIndex, "<b>");
				endIndex += 3;
				info = info.replace(endIndex, 1, ":</b>");
				startIndex = info.indexOf("<br/>", endIndex);
				
				//handle property with multiple lines
				auto endInd = info.indexOf(":", startIndex);
				if (endInd != -1) {
					QStringRef lines(&info, startIndex, endInd - startIndex);
					auto lastIndex = lines.lastIndexOf("<br/>");
					startIndex = lastIndex != -1 ? lastIndex+startIndex : startIndex;
				}
			}
			else {
				end = true;
			}
    }

    this->infoTextBox->setHtml(info);
}
