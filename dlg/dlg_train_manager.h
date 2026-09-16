#ifndef DLG_TRAIN_MANAGER_H
#define DLG_TRAIN_MANAGER_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>
#include "train_client.h"

class TrainManagerDialog : public QDialog
{
    Q_OBJECT

public:
    struct TrainSession {
        QString name;
        int totalEpochs{0};
        int currentEpoch{0};
        double loss{0};
        double mAP50{0};
        bool paused{false};
        bool running{true};
        TrainClient *client{nullptr};
        int port{9257};
        QProcess *process{nullptr};
    };

    explicit TrainManagerDialog(QWidget *parent = nullptr);

    int addSession(const QString &name, int totalEpochs, int port,
                   TrainClient *client, QProcess *process);
    void updateMetrics(int row, const TrainMetrics &metrics);
    void setSessionFinished(int row);
    void setSessionError(int row, const QString &msg);

signals:
    void sg_pauseResume(int row, bool pause);
    void sg_stop(int row);

private slots:
    void onPauseResumeClicked();
    void onStopClicked();

private:
    QTableWidget *m_table;
    QList<TrainSession> m_sessions;

    void updateRow(int row);
};

#endif // DLG_TRAIN_MANAGER_H
