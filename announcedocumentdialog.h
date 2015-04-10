#ifndef ANNOUNCEDOCUMENTDIALOG_H
#define ANNOUNCEDOCUMENTDIALOG_H

#include <QtWidgets/QDialog>
#include <QRegExpValidator>

namespace Ui {
    class AnnounceDocumentDialog;
}

class AnnounceDocumentDialog : public QDialog {
    Q_OBJECT
public:
    AnnounceDocumentDialog(QWidget *parent = 0);
    ~AnnounceDocumentDialog();

private:
    Ui::AnnounceDocumentDialog *ui;

    QRegExpValidator *nameValidator;

public slots:
    void setAnnounceDialogInfo(QString name);

private slots:
    void dialogAccepted();

signals:
    void announceDocument(QString name);
};

#endif // ANNOUNCEDOCUMENTDIALOG_H
