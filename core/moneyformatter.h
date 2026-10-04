/**
 * @file moneyformatter.h
 * @brief High-precision integer money formatting with Indian numbering system grouping.
 * @layer Core (Layer 1 - Foundations)
 *
 * All monetary amounts in QuickCart are strictly represented as 64-bit signed integers (qint64)
 * in the smallest currency unit (paise). Zero floating-point operations are performed.
 *
 * Indian numbering system grouping:
 * - Last 3 digits before decimal are grouped together (hundreds).
 * - Preceding digits are grouped in pairs of 2 (thousands, lakhs, crores).
 * E.g., 12345678 paise = ₹1,23,456.78
 */

#ifndef MONEYFORMATTER_H
#define MONEYFORMATTER_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtGlobal>

class MoneyFormatter {
public:
    /**
     * @brief Format integer paise with Indian numbering system grouping.
     * @param paise Monetary value in integer paise (1 Rupee = 100 Paise).
     * @param includeSymbol If true, prepends the Rupee symbol (₹).
     * @return Formatted string, e.g. "₹1,23,456.78" or "1,23,456.78".
     */
    static inline QString formatPaise(qint64 paise, bool includeSymbol = true) {
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

    /**
     * @brief Format integer paise to rupees with Indian grouping and currency symbol.
     */
    static inline QString format(qint64 paise) {
        return formatPaise(paise, true);
    }
};

#endif // MONEYFORMATTER_H
