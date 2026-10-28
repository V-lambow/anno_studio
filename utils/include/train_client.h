#ifndef TRAIN_CLIENT_H
#define TRAIN_CLIENT_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <zmq.hpp>
#include <json/json.h>

Q_DECLARE_METATYPE(Json::Value)

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

class TrainClientWorker : public QObject
{
    Q_OBJECT

public:
    explicit TrainClientWorker(int port, QObject *parent = nullptr);
    ~TrainClientWorker();

public slots:
    void start();
    void sl_sendCommand(const QString &cmd);

signals:
    void sg_commandResult(const Json::Value &result);
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

class TrainClient : public QObject
{
    Q_OBJECT

public:
    explicit TrainClient(int port = 9257, QObject *parent = nullptr);
    ~TrainClient();

    void sendCommand(const QString &cmd);

signals:
    void sg_commandResult(const Json::Value &result);
    void sg_metrics(const TrainMetrics &metrics);
    void sg_trainEnd(const QString &saveDir);
    void sg_error(const QString &message);
    void sg_sendCommand(const QString &cmd);
    void sg_startWorker();

private:
    QThread m_thread;
    TrainClientWorker *m_worker;
};

#endif // TRAIN_CLIENT_H
