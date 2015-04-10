#ifndef CONNECTTODOCUMENT_H
#define CONNECTTODOCUMENT_H

#include <QtWidgets/QDialog>
#include <QRegExpValidator>
#include <QUdpSocket>

#include <QtWidgets/QListWidget>

namespace Ui {
    class ConnectToDocument;
}

class ConnectToDocument : public QDialog {
    Q_OBJECT
public:
    ConnectToDocument(QWidget *parent = 0);
    ~ConnectToDocument();

    void setName(QString name);

private:
    Ui::ConnectToDocument *ui;

    QRegExpValidator* nameValidator;
    QRegExpValidator* addressValidator;
    QRegExpValidator* portValidator;

private slots:
    void dialogAccepted();

signals:
    void connectToDocumentClicked(QStringList list);

};

#endif // CONNECTTODOCUMENT_H
