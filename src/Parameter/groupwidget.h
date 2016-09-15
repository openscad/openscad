#ifndef GROUPWIDGET_H
#define GROUPWIDGET_H


#include <QFrame>
#include <QGridLayout>
#include <QParallelAnimationGroup>
#include <QScrollArea>
#include <QToolButton>
#include <QWidget>
#include <vector>

struct groupInst{
    std::vector<std::string> parameterVector;
    bool show;
};

class GroupWidget : public QWidget {
    Q_OBJECT
private:
    QGridLayout mainLayout;
    QToolButton toggleButton;
    QFrame headerLine;
    QParallelAnimationGroup toggleAnimation;
    QScrollArea contentArea;
    int animationDuration;
    bool *show;
public:
    groupInst groupinst;
    explicit GroupWidget(bool &show,const QString & title = "", const int animationDuration = 300, QWidget *parent = 0);
    void setContentLayout(QLayout & contentLayout);

private slots:
    void onclicked(bool);
};

#endif // GROUPWIDGET_H
