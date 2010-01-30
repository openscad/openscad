#ifndef PRINTUTILS_H_
#define PRINTUTILS_H_

#include <QString>
#include <QList>

extern QList<QString> print_messages_stack;
void print_messages_push();
void print_messages_pop();

void PRINT(const QString &msg);
#define PRINTF(_fmt, ...) do { QString _m; _m.sprintf(_fmt, ##__VA_ARGS__); PRINT(_m); } while (0)
#define PRINTA(_fmt, ...) do { QString _m = QString(_fmt).arg(__VA_ARGS__); PRINT(_m); } while (0)

void PRINT_NOCACHE(const QString &msg);
#define PRINTF_NOCACHE(_fmt, ...) do { QString _m; _m.sprintf(_fmt, ##__VA_ARGS__); PRINT_NOCACHE(_m); } while (0)
#define PRINTA_NOCACHE(_fmt, ...) do { QString _m = QString(_fmt).arg(__VA_ARGS__); PRINT_NOCACHE(_m); } while (0)

#endif
