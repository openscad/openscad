#ifndef QWORDSEARCHFIELD_H
#define QWORDSEARCHFIELD_H

#include <QLineEdit>
#include <QLabel>

class QWordSearchField : public QLineEdit
{
    Q_OBJECT
    
public:
    QWordSearchField(QFrame *parent = NULL);
    int findcount;
    
protected:
    void resizeEvent(QResizeEvent *);
    void resizeSearchField();
    
private slots:
    void updateFieldLabel(const QString &text);
    
private:
    QLabel *fieldLabel;
};

#endif /* QWORDSEARCHFIELD_H */
