#ifndef BRIDGE_H
#define BRIDGE_H

#include <QUdpSocket>
#include <QAbstractTableModel>

class Bridge : public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(QString iface READ iface WRITE setIface NOTIFY ifaceChanged)
public:
    explicit Bridge(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    const QString iface() const;
    void setIface(const QString &newIface);
    Q_INVOKABLE static QVariant allInterfaces();

public slots:
    void discovery();

signals:
    void ifaceChanged();

private slots:
    void readPendingDatagrams();

private:
    QString m_uuid;
    QUdpSocket *m_socket;

    QList<QString> m_list;
};

#endif // BRIDGE_H
