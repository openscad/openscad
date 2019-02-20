#ifndef QWORDSEARCHFIELD_H
#define QWORDSEARCHFIELD_H

#include <QLineEdit>
#include <QLabel>

class QWordSearchField : public QLineEdit
{
    Q_OBJECT
    
public:
    QWordSearchField(QFrame *parent = nullptr);
    int findCount() const { return findcount; }
    
protected:
    void resizeEvent(QResizeEvent *) override;
    void resizeSearchField();
    
private slots:
    void updateFieldLabel();
    
public slots:
    void setFindCount(int value);
    
signals:
    void findCountChanged();
    
private:
    QLabel *fieldLabel;
    int findcount;
};

#endif /* QWORDSEARCHFIELD_H */

