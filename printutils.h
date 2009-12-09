#ifndef PRINTUTILS_H_
#define PRINTUTILS_H_

#include <QString>

void PRINT(const QString &msg);
#define PRINTF(_fmt, ...) do { QString _m; _m.sprintf(_fmt, ##__VA_ARGS__); PRINT(_m); } while (0)
#define PRINTA(_fmt, ...) do { QString _m = QString(_fmt).arg(__VA_ARGS__); PRINT(_m); } while (0)

#endif
