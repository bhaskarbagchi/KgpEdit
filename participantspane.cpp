#include "participantspane.h"
#include "ui_participantspane.h"

#include "utilities.h"
#include "enu.h"

#include <QTime>
#include <QHostAddress>
#include <QTreeWidgetItem>
#include <QSettings>


ParticipantsPane::ParticipantsPane(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParticipantsPane)
{
    ui->setupUi(this);

    connect(ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

    ui->connectInfoLabel->hide();
    ui->treeWidget->resizeColumnToContents(0);
    ui->treeWidget->resizeColumnToContents(1);
    ui->treeWidget->expandAll();

    rwItem = ui->treeWidget->topLevelItem(0);
    owner = rwItem->child(0);
}

ParticipantsPane::~ParticipantsPane()
{
    delete ui;
}

void ParticipantsPane::setConnectInfo(QString address, QString port)
{
    ui->connectInfoLabel->show();
    ui->connectInfoLabel->setText("IP: " + address + "\nPort: " + port);
}

void ParticipantsPane::newParticipant(QTcpSocket *socket)
{
    // Add a participant, but don't make an item until we have a name.
    // Once we have a name, we updateName() (below) and insert it to the treeWidget
    Participant *participant = new Participant;
    participantList.append(participant);
    participantMap.insert(socket, participant);

    participant->socket = socket;
    participant->permissions = Enu::ReadWrite;
    participant->socket = socket;
    participant->address = socket->peerAddress();
    emit memberPermissionsChanged(participant->socket, "write");

    // Initializes the incoming block size to 0
    participant->blockSize = 0;
}

bool ParticipantsPane::addParticipant(QString name, QTcpSocket *socket)
{
    Participant *participant = participantMap.value(socket);

    for (int i = 0; i < participantList.size(); i++) {
        if (socket->peerAddress() == participantList.at(i)->address && participantList.at(i)->name == name) {
            // duplicate connection, reject
            participantList.removeOne(participant);
            participantMap.remove(socket);
            delete participant;
            return false;
        }
    }
    participant->name = name;
    participant->item = new QTreeWidgetItem(rwItem);
    participant->item->setText(0, name);

    participant->color = QColor::fromHsv(qrand() % 256, 190, 190);

    participant->item->setBackgroundColor(1, participant->color);
    participant->item->setToolTip(0, QString("%1@%2").arg(name).arg(participant->address.toString()));

    return true;
}

QString ParticipantsPane::getNameForSocket(QTcpSocket *socket)
{
    Participant *participant = participantMap.value(socket);
    return participant->name;
}

QString ParticipantsPane::getNameAddressForSocket(QTcpSocket *socket)
{
    if (participantMap.contains(socket)) {
        Participant *participant = participantMap.value(socket);
        return QString("%1@%2").arg(participant->name).arg(participant->address.toString());
    }
    else {
        return "NULL";
    }
}

void ParticipantsPane::newParticipant(QString name, QString address, QString permissions)
{
    // The server has given us a name to add to the treeWidget
    Participant *participant = new Participant;
    participantList.append(participant);

    participant->name = name;
    participant->address = QHostAddress(address);

    participant->color = QColor::fromHsv(qrand() % 256, 190, 190);
    participant->item = new QTreeWidgetItem(rwItem);
    participant->permissions = Enu::ReadWrite;

    participant->item->setText(0, name);
    participant->item->setBackgroundColor(1, participant->color);
    participant->item->setToolTip(0, name + "@" + address);
}

void ParticipantsPane::removeAllParticipants()
{
    for (int i = 0; i < participantList.size(); i++) {
        participantList.at(i)->item->parent()->removeChild(participantList.at(i)->item);
        delete participantList.at(i);
    }
    participantList.clear();
    participantMap.clear();
}

void ParticipantsPane::removeParticipant(QTcpSocket *socket)
{
    for (int i = 0; i < participantList.size(); i++) {
        if (participantList.at(i)->socket == socket) {
            Participant *participant = participantList.at(i);
            participant->item->parent()->removeChild(participant->item);
            participantMap.remove(socket);
            participantList.removeAt(i);
            delete participant;
            return;
        }
    }
}

void ParticipantsPane::removeParticipant(QString name, QString address)
{
    for (int i = 0; i < participantList.size(); i++) {
        if (participantList.at(i)->name == name && participantList.at(i)->address.toString() == address) {
            Participant *participant = participantList.at(i);
            participant->item->parent()->removeChild(participant->item);
            participantList.removeAt(i);
            delete participant;
        }
    }
}

void ParticipantsPane::setParticipantPermissions(QString name, QString address, QString permissions)
{
    QString fullName = name + "@" + address;
    for (int i = 0; i < participantList.size(); i++) {
        if (participantList.at(i)->item->toolTip(0) == fullName) {
            Participant *participant = participantList.at(i);
            participant->item->parent()->removeChild(participant->item);
            rwItem->addChild(participant->item);
            participant->permissions = Enu::ReadWrite;
        }
    }
}

void ParticipantsPane::setOwnerName(QString name)
{
    owner->setText(0, name);
}

bool ParticipantsPane::canWrite(QTcpSocket *socket)
{
    return participantMap.contains(socket) && participantMap.value(socket)->permissions == Enu::ReadWrite;
}

bool ParticipantsPane::canRead(QTcpSocket *socket)
{
    return participantMap.value(socket)->permissions == Enu::ReadOnly || participantMap.value(socket)->permissions == Enu::ReadWrite;
}

void ParticipantsPane::setFont(QFont font)
{
    ui->treeWidget->setFont(font);
    ui->connectInfoLabel->setFont(font);
}
