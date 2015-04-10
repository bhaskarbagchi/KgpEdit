#include "utilities.h"

#include <QString>

// Fonts
const QString Utilities::codeFont = getSystem() == "Windows" ? "Courier New" : (getSystem() == "Mac" ? "Monaco" : "Courier 10 Pitch");
const int Utilities::codeFontSize = getSystem() == "Mac" ? 11 : 9;
const QString Utilities::labelFont = getSystem() == "Mac" ? "Lucida Grande" : "Verdana";
const int Utilities::labelFontSize = getSystem() == "Mac" ? 11 : 8;
const QString Utilities::chatFont = getSystem() == "Mac" ? "Lucida Grande" : "Verdana";
const int Utilities::chatFontSize = getSystem() == "Mac" ? 11 : 8;

QString Utilities::getSystem() {
    return QString("Linux");
}

