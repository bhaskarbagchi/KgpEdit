#include "chatbrowser.h"

#include <QTextBlock>
#include <QKeyEvent>
#include <QTextCharFormat>
#include <QDebug>

ChatBrowser::ChatBrowser(QWidget *parent) : QTextEdit(parent)
{
    setReadOnly(true);
}

void ChatBrowser::keyPressEvent(QKeyEvent *e)
{
    if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ShiftModifier) {
        emit keyPress(e);
    }
}

void ChatBrowser::addChatLine(QString str, QColor color)
{
    QRegExp rx("([a-zA-Z0-9_]*):\t([0-9]*):\t(.*)");
    if (str.contains(rx)) {
        str = rx.cap(2) + "      " + rx.cap(1) + ": " + rx.cap(3);
    }
    append(str);

    QStringList strings = this->toPlainText().split(QRegExp("\n|\r\n|\r"));
    strings.sort();
    this->setPlainText(strings.join('\n'));

    return;
}
