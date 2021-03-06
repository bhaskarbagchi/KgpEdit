#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QtWidgets/QWidget>
#include "codeeditor.h"
#include "participantspane.h"
#include "chatpane.h"
#include "enu.h"

#include "client.h"
#include "server.h"

#include <QtNetwork>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSettings>

namespace Ui {
    class Document;
}

class Document : public QWidget {
    Q_OBJECT
public:
    Document(QWidget *parent = 0);
    ~Document();

    void announceDocument();

    void connectToDocument(QStringList list);

    void setEditorFont(QFont font);
    void setChatFont(QFont font);
    void setParticipantsFont(QFont font);

    void undo();
    void redo();
    void cut();
    void copy();
    void paste();

    // shows/hides the participants pane
    void setParticipantsHidden(bool b);
    // shows/hides the chat pane
    void setChatHidden(bool b);

    // User wants to resynchronize the document with the owner
    void resynchronizeTriggered();

    // returns if the editor is undable
    bool isUndoable();
    // returns if the editor is redoable
    bool isRedoable();
    // returns if the editor is modified (unsaved since last edit)
    bool isModified();
    // returns if the chat pane is hidden, used for determining which actions to enable/disable by the main window
    bool isChatHidden();
    // returns if the participants pane is hidden, used for determining which actions to enable/disable in the MW
    bool isParticipantsHidden();

    QString getPlainText();
    void setPlainText(QString text);

    // Used for setting the document's modified attribute to b (for the modified-since-last-save attribute)
    void setModified(bool b);

    bool docHasCollaborated();

    void setOwnerName(QString name);

    QString curFile;
    QString myName;

private:
    Ui::Document *ui;

    Client *client;
    Server *server;

    CodeEditor *editor;
    CodeEditor *bottomEditor;
    bool startedCollaborating;

    ParticipantsPane *participantPane;
    ChatPane *chatPane;

signals:
    void redoAvailable(bool);
    void undoAvailable(bool);
    void notFound();
};

#endif // DOCUMENT_H
