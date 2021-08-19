#include <QUuid>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QNetworkDatagram>
#include <QNetworkInterface>

#include "bridge.h"

#define MULTICAST_ADDRESS    "239.255.255.250"
#define MULTICAST_PORT       3702

Bridge::Bridge(QObject *parent) : QAbstractTableModel(parent)
{
    m_uuid = "uuid:" + QUuid::createUuid().toString(QUuid::WithoutBraces);

    m_socket = new QUdpSocket(this);
    m_socket->setSocketOption(QUdpSocket::MulticastTtlOption, 1);
    m_socket->setSocketOption(QUdpSocket::MulticastLoopbackOption, 0);

    m_socket->bind(QHostAddress(QHostAddress::AnyIPv4));
    m_socket->joinMulticastGroup(QHostAddress(MULTICAST_ADDRESS));

    connect(m_socket, &QUdpSocket::readyRead, this, &Bridge::readPendingDatagrams);

    for (int var = 0; var < 42; ++var) {
        m_list.append(QString::number(var));
    }
}
QVariant Bridge::allInterfaces()
{
    QJsonArray json;

    for (auto &iface : QNetworkInterface::allInterfaces()) {
        if (iface.type() == QNetworkInterface::Loopback)
            continue;

        QJsonObject obj = {
            {"name", iface.humanReadableName()},
            {"value", iface.name()}
        };
        json.append(obj);
    }

    return json;
}

const QString Bridge::iface() const
{
    auto _iface = m_socket->multicastInterface();

    return _iface.name();
}

void Bridge::setIface(const QString &newIface)
{
    auto name = m_socket->multicastInterface().name();
    if (name == newIface)
        return;

    auto _iface = QNetworkInterface::interfaceFromName(newIface);
    m_socket->setMulticastInterface(_iface);

    emit ifaceChanged();
}

void Bridge::discovery()
{
    removeRows(0, m_list.size());

    QByteArray data;
    QXmlStreamWriter stream(&data);
    stream.setAutoFormatting(true);
    stream.writeStartDocument(R"(xml version="1.0" encoding="utf-8")");
    stream.writeStartElement("Envelope");

    stream.writeAttribute("xmlns:tds", "http://www.onvif.org/ver10/device/wsdl");
    stream.writeAttribute("xmlns", "http://www.w3.org/2003/05/soap-envelope");
    stream.writeStartElement("Header");

    stream.writeStartElement("wsa:MessageID");
    stream.writeAttribute("xmlns:wsa", "http://schemas.xmlsoap.org/ws/2004/08/addressing");
    stream.writeCharacters(m_uuid);
    stream.writeEndElement();

    stream.writeStartElement("wsa:To");
    stream.writeAttribute("xmlns:wsa", "http://schemas.xmlsoap.org/ws/2004/08/addressing");
    stream.writeCharacters("urn:schemas-xmlsoap-org:ws:2005:04:discovery");
    stream.writeEndElement();

    stream.writeStartElement("wsa:Action");
    stream.writeAttribute("xmlns:wsa", "http://schemas.xmlsoap.org/ws/2004/08/addressing");
    stream.writeCharacters("http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe");
    stream.writeEndElement();

    stream.writeEndElement();
    stream.writeStartElement("Body");

    stream.writeStartElement("Probe");
    stream.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    stream.writeAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
    stream.writeAttribute("xmlns", "http://schemas.xmlsoap.org/ws/2005/04/discovery");
    stream.writeTextElement("Types", "tds:Device");
    stream.writeEmptyElement("Scopes");
    stream.writeEndElement();

    stream.writeEndElement();

    stream.writeEndElement();
    stream.writeEndDocument();

    m_socket->writeDatagram(data, QHostAddress(MULTICAST_ADDRESS), MULTICAST_PORT);
}

void Bridge::readPendingDatagrams()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();

        QXmlStreamReader xml(datagram.data());
        auto ip = datagram.senderAddress().toString();

        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()) {
                if (xml.name() == QLatin1String("RelatesTo")) {
                    if (xml.readElementText() != m_uuid)
                        break;
                } else if (xml.name() == QLatin1String("XAddrs")) {
                    auto url = xml.readElementText();
                    if (url.contains(ip)) {
                        setData(QModelIndex(), ip);
                    }
                }
            }
        }
    }
}

int Bridge::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_list.size();
}

int Bridge::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 1;
}

bool Bridge::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent)

    beginRemoveRows(parent, row, row + count - 1);
    for (int var = 0; var < count; ++var) {
        if (row >= m_list.size())
            break;

        m_list.removeAt(row);
    }
    endRemoveRows();

    return true;
}

QVariant Bridge::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_list.size())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    return m_list.at(index.row());
}

bool Bridge::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return QAbstractTableModel::setData(index, value, role);

    if (index.isValid() && index.row() < m_list.size()) {
        m_list[index.row()] = value.toString();
        emit dataChanged(index, index);
    } else {
        beginInsertRows(QModelIndex(), m_list.size(), m_list.size());
        m_list.append(value.toString());
        endInsertRows();
    }

    return true;
}
