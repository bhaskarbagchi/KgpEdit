#ifndef PARTICIPANTSPANE_H
#define PARTICIPANTSPANE_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QHostAddress>
#include <QTcpSocket>

namespace Ui {
    class ParticipantsPane;
}

struct Participant {
    // Used by owner and particiapnts:
    QString name;
    QHostAddress address;
    QColor color;
    QTreeWidgetItem *item;

    // Only used by the owner:
    QTcpSocket *socket;
    // Used to store the incoming packet's size
    quint32 blockSize;
};

class ParticipantsPane : public QWidget {
    Q_OBJECT
public:
    ParticipantsPane(QWidget *parent = 0);
    ~ParticipantsPane();

    void setConnectInfo(QString address, QString port);

    // Participants are first new'd (prior to greetings)
    void newParticipant(QTcpSocket *socket);
    // ...and later added when they send their greetings
    bool addParticipant(QString name, QTcpSocket *socket);
    // getNameForSocket allows us to set their name in the chat pane correctly
    QString getNameForSocket(QTcpSocket *socket);
    // getNameAddressForSocket allows us to get name@address as the owner
    QString getNameAddressForSocket(QTcpSocket *socket);

    // Client participant pane add participant functions

    void newParticipant(QString name, QString address);

    // For when we disconnect
    void removeAllParticipants();
    // This is a function to be used by the owner
    void removeParticipant(QTcpSocket *socket);
    // This is a function to be used by the participants in removing participants via control messages.
    void removeParticipant(QString name, QString address);

    void setOwnerName(QString name);

    void setFont(QFont font);

    QList<Participant*> participantList;
    QMap<QTcpSocket *, Participant *> participantMap;

private:
    Ui::ParticipantsPane *ui;

    QTreeWidgetItem *rwItem;
    QTreeWidgetItem *roItem;
    QTreeWidgetItem *waitItem;
    QTreeWidgetItem *owner;

private slots:

signals:
    void memberCanNowRead(QTcpSocket *member);
};

#endif // PARTICIPANTSPANE_H
