#include "utilities.h"

#include <QString>

// Fonts
const QString Utilities::codeFont = "Courier 10 Pitch";
const int Utilities::codeFontSize = 9;
const QString Utilities::labelFont = "Verdana";
const int Utilities::labelFontSize = 8;
const QString Utilities::chatFont = "Verdana";
const int Utilities::chatFontSize = 8;

QString Utilities::getSystem() {
    return QString("Linux");
}

