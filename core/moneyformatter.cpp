/**
 * @file moneyformatter.cpp
 * @brief Implementation of Indian numbering system money formatting.
 * @layer Core (Layer 1 - Foundations)
 */

#include "moneyformatter.h"
#include <QtCore/QStringList>
#include <climits>

QString MoneyFormatter::formatPaise(qint64 paise, bool includeSymbol) {
    bool isNegative = (paise < 0);
    quint64 absPaise = isNegative ? static_cast<quint64>(-(paise + 1)) + 1ULL : static_cast<quint64>(paise);

    quint64 rupees = absPaise / 100ULL;
    quint64 paiseFraction = absPaise % 100ULL;

    QString fractionStr = QString::asprintf("%02llu", static_cast<unsigned long long>(paiseFraction));

    QString rupeesRaw = QString::number(rupees);
    QString rupeesFormatted;

    if (rupeesRaw.length() <= 3) {
        rupeesFormatted = rupeesRaw;
    } else {
        int len = rupeesRaw.length();
        QString lastThree = rupeesRaw.right(3);
        QString remaining = rupeesRaw.left(len - 3);

        QStringList groups;
        while (remaining.length() > 2) {
            groups.prepend(remaining.right(2));
            remaining.chop(2);
        }
        if (!remaining.isEmpty()) {
            groups.prepend(remaining);
        }
        groups.append(lastThree);
        rupeesFormatted = groups.join(QLatin1Char(','));
    }

    QString result;
    if (isNegative) {
        result += QLatin1Char('-');
    }
    if (includeSymbol) {
        result += QStringLiteral("₹");
    }
    result += rupeesFormatted;
    result += QLatin1Char('.');
    result += fractionStr;

    return result;
}
