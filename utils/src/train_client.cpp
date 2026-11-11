#include "train_client.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace {
constexpr auto kEventPrefix = "__TRAIN_SERVICE_EVENT__=";
} // namespace

TrainClient::TrainClient(QObject *parent)
    : QObject(parent)
{
}

void TrainClient::attachProcess(QProcess *process)
{
    m_process = process;
    connect(process, &QProcess::readyReadStandardOutput, this, &TrainClient::sl_readyRead);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TrainClient::sl_finished);
}

void TrainClient::sl_readyRead()
{
    if (!m_process)
        return;

    m_buffer += m_process->readAllStandardOutput();

    int idx = -1;
    while ((idx = m_buffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_buffer.left(idx);
        m_buffer.remove(0, idx + 1);
        dispatch(QString::fromUtf8(line));
    }
}

void TrainClient::sl_finished(int exitCode, QProcess::ExitStatus status)
{
    sl_readyRead();
    if (m_terminalSeen)
        return;

    if (status == QProcess::NormalExit && exitCode == 0)
        emit sg_trainEnd(m_saveDir);
    else
        emit sg_error(QStringLiteral("train_json_service exited with code %1").arg(exitCode));
}

void TrainClient::dispatch(const QString &raw)
{
    const QString line = raw.trimmed();
    if (line.isEmpty())
        return;

    const QString prefix = QString::fromLatin1(kEventPrefix);
    if (!line.startsWith(prefix))
        return;

    const QJsonObject payload =
        QJsonDocument::fromJson(line.mid(prefix.size()).toUtf8()).object();
    const QString event = payload.value(QStringLiteral("event")).toString();
    if (event.isEmpty())
        return;

    if (event == QLatin1String("training_started")) {
        m_saveDir = payload.value(QStringLiteral("save_dir")).toString();
        return;
    }

    if (event == QLatin1String("training_metrics")) {
        const int epoch = payload.value(QStringLiteral("epoch")).toInt();
        const int total = payload.value(QStringLiteral("total_epochs")).toInt();
        // ultralytics final_eval() 会在 total+1 复发一次 best 指标，进度条不能显示 101/100
        if (epoch > total)
            return;

        TrainMetrics m;
        m.epoch = epoch;
        m.totalEpochs = total;
        m.loss = payload.value(QStringLiteral("loss")).toDouble();
        m.boxLoss = payload.value(QStringLiteral("box_loss")).toDouble();
        m.clsLoss = payload.value(QStringLiteral("cls_loss")).toDouble();
        m.dflLoss = payload.value(QStringLiteral("dfl_loss")).toDouble();
        m.precision = payload.value(QStringLiteral("precision")).toDouble();
        m.recall = payload.value(QStringLiteral("recall")).toDouble();
        m.mAP50 = payload.value(QStringLiteral("mAP50")).toDouble();
        m.mAP50_95 = payload.value(QStringLiteral("mAP50_95")).toDouble();
        emit sg_metrics(m);
        return;
    }

    if (event == QLatin1String("training_completed")
        || event == QLatin1String("training_stopped")) {
        m_terminalSeen = true;
        emit sg_trainEnd(m_saveDir);
        return;
    }

    if (event == QLatin1String("training_error")
        || event == QLatin1String("protocol_error")) {
        m_terminalSeen = true;
        emit sg_error(payload.value(QStringLiteral("error")).toString());
    }
}
