#ifndef TRAIN_CLIENT_H
#define TRAIN_CLIENT_H

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QString>

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
    explicit TrainClient(QObject *parent = nullptr);

    void attachProcess(QProcess *process);

signals:
    void sg_metrics(const TrainMetrics &metrics);
    void sg_trainEnd(const QString &saveDir);
    void sg_error(const QString &message);

private slots:
    void sl_readyRead();
    void sl_finished(int exitCode, QProcess::ExitStatus status);

private:
    void dispatch(const QString &line);

    QProcess *m_process{nullptr};
    QByteArray m_buffer;
    QString m_saveDir;
    bool m_terminalSeen{false};
};

#endif // TRAIN_CLIENT_H
