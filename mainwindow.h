#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include "document.h"
#include "connecttodocument.h"
#include "announcedocumentdialog.h"
#include "firstrundialog.h"

#include <QSettings>

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QMap<QWidget *, Document *> tabWidgetToDocumentMap;
    QString openPath;
    ConnectToDocument *connectDialog;
    AnnounceDocumentDialog *announceDocumentDialog;
    FirstRunDialog *firstRunDialog;

    bool save(int index);
    bool maybeSave(int index);
    bool saveFile(const QString &fileName);
    void loadFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    QString strippedName(const QString &fullFileName);

    void readSettings();
    void writeSettings();

    QString myName; // global name used for connecting to documents

protected:
    void closeEvent(QCloseEvent *event);

    bool eventFilter(QObject *, QEvent *event);

private slots:
    void on_actionFile_New_triggered();
    void on_actionFile_Open_triggered();
    bool on_actionFile_Save_triggered();
    bool on_actionFile_Save_As_triggered();
    void on_actionFile_Close_triggered();

    void on_actionEdit_Undo_triggered();
    void on_actionEdit_Redo_triggered();
    void on_actionEdit_Cut_triggered();
    void on_actionEdit_Copy_triggered();
    void on_actionEdit_Paste_triggered();

    void on_actionTools_Announce_Document_triggered();
    void on_actionTools_Connect_to_Document_triggered();
    void on_actionTools_Resynchronize_Document_triggered();

    void on_actionWindow_Next_Document_triggered();
    void on_actionWindow_Previous_Document_triggered();

    void on_actionHelp_About_KGPEdit_triggered();

    void setUndoability(bool b);
    void setRedoability(bool b);

    void documentChanged(int index);
    void tabCloseClicked(int index);

    void connectToDocument(QStringList list);
    void announceDocument(QString ownerName);
};

#endif // MAINWINDOW_H
