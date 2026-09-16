#ifndef TRAIN_CLIENT_H
#define TRAIN_CLIENT_H

#include <QObject>
#include <QTimer>
#include <zmq.hpp>
#include <json/json.h>

struct TrainMetrics {
    int epoch{0};
    int totalEpochs{0};
    double loss{0};
    double boxLoss{0};
    double clsLoss{0};
    double dflLoss{0};
    double precision{0};
    double recall{0};
    double mAP50{0};
    double mAP50_95{0};
};

class TrainClient : public QObject
{
    Q_OBJECT

public:
    explicit TrainClient(int port = 9257, QObject *parent = nullptr);
    ~TrainClient();

    Json::Value sendCommand(const QString &cmd);

signals:
    void sg_metrics(const TrainMetrics &metrics);
    void sg_trainEnd(const QString &saveDir);
    void sg_error(const QString &message);

private slots:
    void pollSubscription();

private:
    int m_port;
    zmq::context_t m_ctx;
    zmq::socket_t m_req;
    zmq::socket_t m_sub;
    QTimer m_pollTimer;
};

#endif // TRAIN_CLIENT_H
