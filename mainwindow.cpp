#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "utilities.h"
#include "enu.h"

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QTextCodec>
#include <QtWidgets/QFontDialog>
#include <QTextDocument>
#include <QPrinter>
#include <QPrintDialog>
#include <QDebug>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tabWidget->setFont(QFont(Utilities::labelFont, Utilities::labelFontSize));

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(documentChanged(int)));
    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseClicked(int)));

    connectDialog = new ConnectToDocument(this);
    connect(connectDialog, SIGNAL(connectToDocumentClicked(QStringList)), this, SLOT(connectToDocument(QStringList)));

    preferencesDialog = new PreferencesDialog(this);
    connect(preferencesDialog, SIGNAL(setEditorFont(QFont)), this, SLOT(setEditorFont(QFont)));
    connect(preferencesDialog, SIGNAL(setChatFont(QFont)), this, SLOT(setChatFont(QFont)));
    connect(preferencesDialog, SIGNAL(setParticipantsFont(QFont)), this, SLOT(setParticipantsFont(QFont)));

    announceDocumentDialog = new AnnounceDocumentDialog(this);
    connect(announceDocumentDialog, SIGNAL(announceDocument(QString,Qt::CheckState,Qt::CheckState)),
            this, SLOT(announceDocument(QString,Qt::CheckState,Qt::CheckState)));

    // sets the announce dialog to the state of the preferences pane
    announceDocumentDialog->setAnnounceDialogInfo(preferencesDialog->getMyName(), preferencesDialog->getAlwaysUseMyName());

    // Connects the pref dialog to the announce dialog for when information is changed in the preferences dialog
    connect(preferencesDialog, SIGNAL(setAnnounceDialogInfo(QString,bool)), announceDocumentDialog, SLOT(setAnnounceDialogInfo(QString,bool)));

    Document *document = new Document(ui->tab);
    QGridLayout *tabLayout = new QGridLayout;
    tabLayout->addWidget(document);
    tabLayout->setContentsMargins(0,0,0,0);
    ui->tabWidget->widget(0)->setLayout(tabLayout);

    document->setEditorFont(preferencesDialog->getEditorFont());
    document->setChatFont(preferencesDialog->getChatFont());
    document->setParticipantsFont(preferencesDialog->getParticipantsFont());

    tabWidgetToDocumentMap.insert(ui->tabWidget->currentWidget(), document);

    connect(document, SIGNAL(undoAvailable(bool)), this, SLOT(setUndoability(bool)));
    connect(document, SIGNAL(redoAvailable(bool)), this, SLOT(setRedoability(bool)));

    readSettings();
    openPath = QDir::homePath();

    qApp->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readSettings()
{
    QSettings settings("Kgp-Edit", "MainWindow");
    QDesktopWidget *desktop = QApplication::desktop();
    int width = static_cast<int>(desktop->width() * 0.80);
    int height = static_cast<int>(desktop->height() * 0.70);
    int screenWidth = desktop->width();
    int screenHeight = desktop->height();
    QPoint pos = settings.value("pos", QPoint((screenWidth - width) / 2, (screenHeight - height) / 2)).toPoint();
    QSize size = settings.value("size", QSize(width, height)).toSize();
    resize(size);
    move(pos);

    firstRunDialog = new FirstRunDialog(this);
    firstRunDialog->show();
    firstRunDialog->raise();
    firstRunDialog->activateWindow();

    myName = settings.value("name", "Owner").toString();
}

void MainWindow::writeSettings()
{
    QSettings settings("Kgp-Edit", "MainWindow");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("name", preferencesDialog->getMyName());
}

// Protected closeEvent
void MainWindow::closeEvent(QCloseEvent *event)
{
    bool okToQuit = true;
    for (int i = 0; i < ui->tabWidget->count() && okToQuit; i++) {
        okToQuit = maybeSave(i);
    }
    if (okToQuit) {
        writeSettings();
        event->accept();
    }
    else {
        event->ignore();
    }
}

bool MainWindow::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QString fileName = static_cast<QFileOpenEvent *>(event)->file(); // for the sake of mirroring code in our open slot
        if (!fileName.isEmpty()) {

            int index = ui->tabWidget->addTab(new QWidget(), QFileInfo(fileName).fileName());

            Document *document = new Document(ui->tabWidget->widget(index));

            QGridLayout *tabLayout = new QGridLayout;
            tabLayout->addWidget(document);
            tabLayout->setContentsMargins(0,0,0,0);
            ui->tabWidget->widget(index)->setLayout(tabLayout);

            tabWidgetToDocumentMap.insert(ui->tabWidget->widget(index), document);

            connect(document, SIGNAL(undoAvailable(bool)), this, SLOT(setUndoability(bool)));
            connect(document, SIGNAL(redoAvailable(bool)), this, SLOT(setRedoability(bool)));

            ui->tabWidget->setCurrentIndex(index);

            loadFile(fileName);
            openPath = QFileInfo(fileName).path();

            ui->actionTools_Announce_Document->setEnabled(true);

            ui->actionWindow_Next_Document->setEnabled(ui->tabWidget->count() > 1);
            ui->actionWindow_Previous_Document->setEnabled(ui->tabWidget->count() > 1);
        }
        event->accept();
        return true;
    }
    return false;
}

// Save methods
bool MainWindow::save(int index)
{
    if (tabWidgetToDocumentMap.value(ui->tabWidget->widget(index))->curFile.isEmpty()) {
        return on_actionFile_Save_As_triggered();
    }
    else {
        return saveFile(tabWidgetToDocumentMap.value(ui->tabWidget->widget(index))->curFile);
    }
}

bool MainWindow::maybeSave(int index)
{
    if (tabWidgetToDocumentMap.value(ui->tabWidget->widget(index))->isModified()) {
        ui->tabWidget->setCurrentIndex(index);
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, "Collab-edit",
                                   "The document has been modified.\n"
                                   "Do you want to save your changes?",
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return save(index);
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

bool MainWindow::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out.setCodec(QTextCodec::codecForName("ISO 8859-1"));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->getPlainText();
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage("File saved", 4000);
    return true;
}

void MainWindow::loadFile(const QString &fileName)
{
     QFile file(fileName);
     if (!file.open(QFile::ReadOnly | QFile::Text)) {
         QMessageBox::warning(this, tr("Application"),
                              tr("Cannot read file %1:\n%2.")
                              .arg(fileName)
                              .arg(file.errorString()));
         return;
     }

     QTextStream in(&file);
     in.setCodec(QTextCodec::codecForName("ISO 8859-1"));
     QApplication::setOverrideCursor(Qt::WaitCursor);
     tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->setPlainText(in.readAll());
     QApplication::restoreOverrideCursor();

     setCurrentFile(fileName);

     if (fileName.endsWith(".py")) {
         tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->setHighlighter(Document::Python);
     }
     else if (fileName.endsWith(".cpp") || fileName.endsWith(".c") || fileName.endsWith(".h") || fileName.endsWith(".hpp")) {
         tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->setHighlighter(Document::CPlusPlus);
     }
     statusBar()->showMessage("File loaded", 4000);
}

void MainWindow::setCurrentFile(const QString &fileName)
{
     tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->curFile = fileName;
     tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->setModified(false);

     QString shownName;
     if (tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->curFile.isEmpty())
         shownName = "untitled.txt";
     else
         shownName = strippedName(tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->curFile);

     ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), shownName);
}

QString MainWindow::strippedName(const QString &fullFileName)
{
     return QFileInfo(fullFileName).fileName();
}

// File menu items
void MainWindow::on_actionFile_New_triggered()
{
    int index = ui->tabWidget->addTab(new QWidget(), "untitled.txt");
    
    Document *document = new Document(ui->tabWidget->widget(index));
    QGridLayout *tabLayout = new QGridLayout;
    tabLayout->addWidget(document);
    tabLayout->setContentsMargins(0,0,0,0);
    ui->tabWidget->widget(index)->setLayout(tabLayout);

    document->setEditorFont(preferencesDialog->getEditorFont());
    document->setChatFont(preferencesDialog->getChatFont());
    document->setParticipantsFont(preferencesDialog->getParticipantsFont());

    tabWidgetToDocumentMap.insert(ui->tabWidget->widget(index), document);

    connect(document, SIGNAL(undoAvailable(bool)), this, SLOT(setUndoability(bool)));
    connect(document, SIGNAL(redoAvailable(bool)), this, SLOT(setRedoability(bool)));
    
    ui->tabWidget->setCurrentIndex(index);
    ui->actionTools_Announce_Document->setEnabled(true);


    ui->actionWindow_Next_Document->setEnabled(ui->tabWidget->count() > 1);
    ui->actionWindow_Previous_Document->setEnabled(ui->tabWidget->count() > 1);

}

void MainWindow::on_actionFile_Open_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(
            this,
            "Open text file",
            openPath,
            "Text files (*.*)");
    if (!fileName.isEmpty()) {
        int index = ui->tabWidget->addTab(new QWidget(), QFileInfo(fileName).fileName());

        Document *document = new Document(ui->tabWidget->widget(index));

        QGridLayout *tabLayout = new QGridLayout;
        tabLayout->addWidget(document);
        tabLayout->setContentsMargins(0,0,0,0);
        ui->tabWidget->widget(index)->setLayout(tabLayout);

        tabWidgetToDocumentMap.insert(ui->tabWidget->widget(index), document);

        connect(document, SIGNAL(undoAvailable(bool)), this, SLOT(setUndoability(bool)));
        connect(document, SIGNAL(redoAvailable(bool)), this, SLOT(setRedoability(bool)));

        ui->tabWidget->setCurrentIndex(index);

        loadFile(fileName);
        openPath = QFileInfo(fileName).path();

        ui->actionTools_Announce_Document->setEnabled(true);

        ui->actionWindow_Next_Document->setEnabled(ui->tabWidget->count() > 1);
        ui->actionWindow_Previous_Document->setEnabled(ui->tabWidget->count() > 1);
    }
}

bool MainWindow::on_actionFile_Save_triggered()
{
    if (tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->curFile.isEmpty()) {
        return on_actionFile_Save_As_triggered();
    }
    else {
        return saveFile(tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->curFile);
    }
}

bool MainWindow::on_actionFile_Save_As_triggered()
{
    qApp->processEvents(); // Redraw so we see the document we're saving along with the dialog
    // This is in case program control changes documents on a "Save All" or closeEvent
    QString fileName = QFileDialog::getSaveFileName(
            this,
            "Save As...",
            tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->curFile.isEmpty() ?
                QDir::homePath() + "/untitled.txt" :
                tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->curFile,
            "Text (*.txt)");
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}


void MainWindow::on_actionFile_Close_triggered()
{
    tabCloseClicked(ui->tabWidget->currentIndex());

    ui->actionWindow_Next_Document->setEnabled(ui->tabWidget->count() > 1);
    ui->actionWindow_Previous_Document->setEnabled(ui->tabWidget->count() > 1);
}

// Edit menu items
void MainWindow::on_actionEdit_Undo_triggered()
{
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->undo();
}

void MainWindow::on_actionEdit_Redo_triggered()
{
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->redo();
}

void MainWindow::on_actionEdit_Cut_triggered()
{
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->cut();
}

void MainWindow::on_actionEdit_Copy_triggered()
{
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->copy();
}

void MainWindow::on_actionEdit_Paste_triggered()
{
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->paste();
}

void MainWindow::on_actionView_Line_Wrap_triggered()
{
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->toggleLineWrap();
}

void MainWindow::on_actionView_Hide_Show_Participants_triggered()
{

    if (tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->isParticipantsHidden()) {
        tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->setParticipantsHidden(false);
    }
    else {
        tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->setParticipantsHidden(true);
    }
}

void MainWindow::on_actionView_Hide_Show_Chat_triggered()
{
    if (tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->isChatHidden()) {
        tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->setChatHidden(false);
    }
    else {
        tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->setChatHidden(true);
    }
}

void MainWindow::on_actionTools_Announce_Document_triggered()
{
    if (tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->docHasCollaborated()) {
        return; // this SHOULD never happen, but just in case.
    }
//    if (preferencesDialog->getAlwaysUseMyName() && preferencesDialog->getMyName() != "") {
//        tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->announceDocument(announceDocumentDialog->isBroadcastingChecked());
//        tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->setOwnerName(preferencesDialog->getMyName());
//    }
//    else {
        announceDocumentDialog->show();
 //   }
}

void MainWindow::on_actionHelp_About_KGPEdit_triggered()
{
    firstRunDialog->exec();
}

void MainWindow::on_actionTools_Connect_to_Document_triggered()
{
    // Create our dialog and show it. When they user clicks "okay", we'll emit a signal to the mainwindow, and pass that to the document.
    connectDialog->show();
    connectDialog->setName(myName);
}

void MainWindow::on_actionTools_Resynchronize_Document_triggered()
{
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->resynchronizeTriggered();
}

void MainWindow::on_actionTools_Preferences_triggered()
{
    preferencesDialog->show();
}

void MainWindow::on_actionWindow_Split_triggered()
{
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->splitEditor();
    ui->actionWindow_Split->setDisabled(true);
    ui->actionWindow_Split_Side_by_Side->setDisabled(false);
    ui->actionWindow_Remove_Split->setEnabled(true);
}

void MainWindow::on_actionWindow_Split_Side_by_Side_triggered()
{
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->splitEditorSideBySide();
    ui->actionWindow_Split->setDisabled(false);
    ui->actionWindow_Split_Side_by_Side->setDisabled(true);
    ui->actionWindow_Remove_Split->setEnabled(true);

}

void MainWindow::on_actionWindow_Remove_Split_triggered()
{
    ui->actionWindow_Split->setDisabled(false);
    ui->actionWindow_Split_Side_by_Side->setDisabled(false);
    ui->actionWindow_Remove_Split->setEnabled(false);
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->unSplitEditor();
}

void MainWindow::on_actionWindow_Next_Document_triggered()
{
    int numDocs = ui->tabWidget->count();
    int curTab = ui->tabWidget->currentIndex();
    if (numDocs == 1) {
        return;
    }
    ui->tabWidget->setCurrentIndex((curTab + 1) % numDocs);
}

void MainWindow::on_actionWindow_Previous_Document_triggered()
{
    int numDocs = ui->tabWidget->count();
    int curTab = ui->tabWidget->currentIndex();
    if (numDocs == 1) {
        return;
    }
    if (curTab == 0) {
        ui->tabWidget->setCurrentIndex(ui->tabWidget->count() - 1);
    }
    else {
        ui->tabWidget->setCurrentIndex(curTab - 1);
    }
}

void MainWindow::setUndoability(bool b)
{
    if (sender() == tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())) {
        ui->actionEdit_Undo->setEnabled(b);
    }
}

void MainWindow::setRedoability(bool b)
{
    if (sender() == tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())) {
        ui->actionEdit_Redo->setEnabled(b);
    }
}

void MainWindow::documentChanged(int index)
{
    Document *document = tabWidgetToDocumentMap.value(ui->tabWidget->widget(index));
    ui->actionEdit_Undo->setEnabled(document->isUndoable());
    ui->actionEdit_Redo->setEnabled(document->isRedoable());
    ui->actionWindow_Split->setDisabled(document->isEditorSplit() && !document->isEditorSplitSideBySide());
    ui->actionWindow_Split_Side_by_Side->setDisabled(document->isEditorSplit() && document->isEditorSplitSideBySide());

    ui->actionWindow_Remove_Split->setEnabled(document->isEditorSplit());
    ui->actionTools_Announce_Document->setDisabled(document->docHasCollaborated());
}

void MainWindow::tabCloseClicked(int index)
{
    if (maybeSave(index)) {
        if (ui->tabWidget->count() == 1) {
            on_actionFile_New_triggered();
        }
        ui->tabWidget->widget(index)->deleteLater();
        tabWidgetToDocumentMap.remove(ui->tabWidget->widget(index));
        ui->tabWidget->removeTab(index);
    }
}

void MainWindow::connectToDocument(QStringList list)
{
    on_actionFile_New_triggered();
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->connectToDocument(list);;
    if (tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->myName != "") {
        myName = tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->myName;
    }
}

void MainWindow::announceDocument(QString ownerName, Qt::CheckState broadcastCheckState, Qt::CheckState alwaysUserNameCheckState)
{
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->announceDocument(broadcastCheckState == Qt::Checked);
    tabWidgetToDocumentMap.value(ui->tabWidget->currentWidget())->setOwnerName(ownerName);
    ui->actionTools_Announce_Document->setEnabled(false);

    if (alwaysUserNameCheckState == Qt::Checked) {
        preferencesDialog->setAlwaysUseMyName(true);
        preferencesDialog->setMyName(ownerName);
    }
    else {
        preferencesDialog->setAlwaysUseMyName(true);
    }
}

void MainWindow::setEditorFont(QFont font)
{
    for (int i = 0; i < tabWidgetToDocumentMap.size(); i++) {
        tabWidgetToDocumentMap.value(ui->tabWidget->widget(i))->setEditorFont(font);
    }
}

void MainWindow::setChatFont(QFont font)
{
    for (int i = 0; i < tabWidgetToDocumentMap.size(); i++) {
        tabWidgetToDocumentMap.value(ui->tabWidget->widget(i))->setChatFont(font);
    }
}

void MainWindow::setParticipantsFont(QFont font)
{
    for (int i = 0; i < tabWidgetToDocumentMap.size(); i++) {
        tabWidgetToDocumentMap.value(ui->tabWidget->widget(i))->setParticipantsFont(font);
    }
}
