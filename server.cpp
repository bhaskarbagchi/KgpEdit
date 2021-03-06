#include "server.h"

#include <QTextDocumentFragment>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QDateTime>

Server::Server(CodeEditor *editor, ParticipantsPane *participantsPane, ChatPane *chatPane, QObject *parent) :
    QObject(parent)
{
    this->editor = editor;
    this->participantPane = participantsPane;
    this->chatPane = chatPane;

    server = new QTcpServer(this);

    connect(editor->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(onTextChange(int,int,int)));
    connect(chatPane, SIGNAL(returnPressed(QString)), this, SLOT(onChatSend(QString)));

    connect(participantPane, SIGNAL(memberCanNowRead(QTcpSocket*)), this, SLOT(populateDocumentForUser(QTcpSocket*)));

    connect(server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

bool Server::listen(const QHostAddress &address, quint16 port)
{
    return server->listen(address, port);
}

quint16 Server::serverPort()
{
    return server->serverPort();
}

void Server::writeToAll(QString string, QTcpSocket *exception)
{
    if (string != "") {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_6);

        // Reserve space for a 16 bit int that will contain the total size of the block we're sending
        out << (quint32)0;

        // Write the data to the stream
        out << string;

        // Move the head to the beginning and replace the reserved space at the beginning with the size of the block.
        out.device()->seek(0);
        out << (quint32)(block.size() - sizeof(quint32));

        for (int i = 0; i < participantPane->participantList.size(); i++) {
            if (participantPane->participantList.at(i)->socket != exception) {
                participantPane->participantList.at(i)->socket->write(block);
            }
        }
    }
}

void Server::writeToSocket(QString string, QTcpSocket *socket)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_6);

    // Reserve space for a 32 bit int that will contain the total size of the block we're sending
    out << (quint32)0;

    // Write the data to the stream
    out << string;

    // Move the head to the beginning and replace the reserved space at the beginning with the size of the block, minus the size of the uint.
    out.device()->seek(0);
    out << (quint32)(block.size() - sizeof(quint32));

    socket->write(block);
}

void Server::resynchronize()
{
    for (int i = 0; i < participantPane->participantList.size(); i++) {
        populateDocumentForUser(participantPane->participantList.at(i)->socket);
    }
}

void Server::setOwnerName(QString name)
{
    myName = name;
    writeToAll(QString("helo:%1").arg(myName).toAscii());
    participantPane->setOwnerName(name);
}

void Server::processData(QString data, QTcpSocket *sender)
{
    QString toSend;
    QTcpSocket *exception = 0;

    QRegExp rx;
    if (data.startsWith("doc:")) {
        toSend = data;
        data.remove(0, 4);
        // detect line number, then put text at that line.
        rx = QRegExp("(\\d+)\\s(\\d+)\\s(\\d+)\\s(.*)");
        if (data.contains(rx)) {
            int pos = rx.cap(1).toInt();
            int charsRemoved = rx.cap(2).toInt();
            int charsAdded = rx.cap(3).toInt();
            data = rx.cap(4);
            editor->collabTextChange(pos, charsRemoved, charsAdded, data);
            exception = sender;
        }
    }
    else if (data.startsWith("chat:")) {
        data.remove(0, 5);
        toSend = data;
        chatPane->appendChatMessage(toSend);
        toSend.insert(0, "chat:");
        exception = sender;
    }
    else if (data.startsWith("resync")) { // user requesting resync of the entire document
        populateDocumentForUser(sender);
    }
    else if (data.startsWith("helo:")) {
        data.remove(0, 5);
        rx = QRegExp("([a-zA-Z0-9_]*)");
        if (data.contains(rx)) {
            QString name = rx.cap(1);
            if (participantPane->addParticipant(name, sender)) { // returns false if there is a duplicate
                toSend = "join:" + participantPane->getNameAddressForSocket(sender);
                writeToSocket(QString("helo:%1").arg(myName).toAscii(), sender);
                populateDocumentForUser(sender);
            }
            else {
                disconnect(sender, SIGNAL(disconnected()), this, SLOT(disconnected()));
                sender->disconnectFromHost();
                return; // we're rejecting the particpant because he's joining with a duplicate name.
            }
        }
    }

    // Distribute data to all the other participants
    writeToAll(toSend, exception);
}

void Server::onTextChange(int pos, int charsRemoved, int charsAdded)
{
    QString toSend;
    QString data;

    if (charsRemoved > 0 && charsAdded == 0) {
        data = "";
    }
    else if (charsAdded > 0) {
        QTextCursor cursor = QTextCursor(editor->document());
        cursor.setPosition(pos, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, charsAdded);
        data = cursor.selection().toPlainText();
    }

    toSend = QString("doc:%1 %2 %3 %4").arg(pos).arg(charsRemoved).arg(charsAdded).arg(data);
    writeToAll(toSend);
}

void Server::onChatSend(QString str)
{
    QString timeStamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString toSend = "chat:" + QString("%1:\t%2:\t%3").arg(myName).arg(timeStamp).arg(str);

    writeToAll(toSend);
    chatPane->appendChatMessage(QString("%1:\t%2:\t%3").arg(myName).arg(timeStamp).arg(str));
}

void Server::onIncomingData()
{
    // disconnect the signal that fires when the contents of the editor change so we don't echo
    disconnect(editor->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(onTextChange(int,int,int)));

    // cast the sender() of this signal to a QTcpSocket
    QTcpSocket *sock = qobject_cast<QTcpSocket *>(sender());
    Participant *participant = participantPane->participantMap.value(sock);

    while (sock->bytesAvailable() > 0) {
        QString data;
        QDataStream in(sock);
        in.setVersion(QDataStream::Qt_4_6);

        if (participant->blockSize == 0) { // blockSize is 0, so we don't know how big the next packet is yet
            // We check if we have at least the size of quint32 to read, if not, return and wait for the next readyRead
            if (sock->bytesAvailable() < (int)sizeof(quint32))
                return;

            // blockSize is the first chunk, so fetch that to find out how big this packet is going to be
            in >> participant->blockSize;
        }

        // If we don't yet have the full size of the packet, return and wait for the rest
        if (sock->bytesAvailable() < participant->blockSize) {
            return;
        }

        in >> data;

        processData(data, sock);

        participant->blockSize = 0; // reset blockSize to 0 for the next packet
    }

    // reconnect the signal that fires when the contents of the editor change so we continue to send new text
    connect(editor->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(onTextChange(int,int,int)));
}

void Server::onNewConnection()
{
    QTcpSocket *sock = server->nextPendingConnection();
    connect(sock, SIGNAL(readyRead()), this, SLOT(onIncomingData()));
    connect(sock, SIGNAL(disconnected()), this, SLOT(disconnected()));
    participantPane->newParticipant(sock);
    sock->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
}

void Server::populateDocumentForUser(QTcpSocket *socket)
{
    // Send entire document
    QString toSend = QString("sync:%1").arg(editor->toPlainText());
    writeToSocket(toSend, socket);

    // Send participants
    toSend.clear();
    QString name;
    QString address;
    for (int i = 0; i < participantPane->participantList.size(); i++) {
        if (participantPane->participantMap.value(socket)==participantPane->participantList.at(i))
                continue;
        name = participantPane->participantList.at(i)->name;
        address = participantPane->participantList.at(i)->address.toString();
        toSend = QString("join:%1@%2").arg(name).arg(address);
        writeToSocket(toSend, socket);
    }
}

void Server::disconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    QString toSend = QString("leave:%1").arg(participantPane->getNameAddressForSocket(socket));
    participantPane->removeParticipant(socket);
    writeToAll(toSend);
}

void Server::displayError(QAbstractSocket::SocketError socketError)
{    
    QTcpSocket *sock = qobject_cast<QTcpSocket *>(sender());

    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    default:
        QMessageBox::information(editor, tr("Kgp-Edit"),
                                 tr("The following error occurred: %1.")
                                 .arg(sock->errorString()));
    }
}
