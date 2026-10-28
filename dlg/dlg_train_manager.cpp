#include "dlg_train_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>

TrainManagerDialog::TrainManagerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("训练管理");
    setMinimumSize(1100, 350);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(10);
    m_table->setHorizontalHeaderLabels({
        "训练名", "进度", "Loss", "BoxLoss", "ClsLoss", "DFLLoss",
        "Precision", "Recall", "mAP50", "操作"
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i < 9; ++i)
        m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(9, 180);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QTableWidget::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_table);
}

int TrainManagerDialog::addSession(const QString &name, int totalEpochs, int port,
                                   TrainClient *client, QProcess *process)
{
    int row = m_table->rowCount();
    m_table->insertRow(row);

    TrainSession session;
    session.name = name;
    session.totalEpochs = totalEpochs;
    session.port = port;
    session.client = client;
    session.process = process;
    m_sessions.append(session);

    auto *nameItem = new QTableWidgetItem(name);
    nameItem->setData(Qt::UserRole, row);
    m_table->setItem(row, 0, nameItem);

    auto *progressBar = new QProgressBar(this);
    progressBar->setRange(0, totalEpochs);
    progressBar->setValue(0);
    progressBar->setFormat(QString("0/%1").arg(totalEpochs));
    m_table->setCellWidget(row, 1, progressBar);

    auto *lossLabel = new QLabel("--", this);
    lossLabel->setAlignment(Qt::AlignCenter);
    m_table->setCellWidget(row, 2, lossLabel);

    auto *boxLossLabel = new QLabel("--", this);
    boxLossLabel->setAlignment(Qt::AlignCenter);
    m_table->setCellWidget(row, 3, boxLossLabel);

    auto *clsLossLabel = new QLabel("--", this);
    clsLossLabel->setAlignment(Qt::AlignCenter);
    m_table->setCellWidget(row, 4, clsLossLabel);

    auto *dflLossLabel = new QLabel("--", this);
    dflLossLabel->setAlignment(Qt::AlignCenter);
    m_table->setCellWidget(row, 5, dflLossLabel);

    auto *precLabel = new QLabel("--", this);
    precLabel->setAlignment(Qt::AlignCenter);
    m_table->setCellWidget(row, 6, precLabel);

    auto *recallLabel = new QLabel("--", this);
    recallLabel->setAlignment(Qt::AlignCenter);
    m_table->setCellWidget(row, 7, recallLabel);

    auto *mapLabel = new QLabel("--", this);
    mapLabel->setAlignment(Qt::AlignCenter);
    m_table->setCellWidget(row, 8, mapLabel);

    auto *opWidget = new QWidget(this);
    auto *opLayout = new QHBoxLayout(opWidget);
    opLayout->setContentsMargins(4, 2, 4, 2);
    opLayout->setSpacing(6);

    auto *pauseBtn = new QPushButton("暂停", this);
    pauseBtn->setObjectName("pauseBtn");
    pauseBtn->setFixedWidth(70);
    auto *stopBtn = new QPushButton("停止", this);
    stopBtn->setObjectName("stopBtn");
    stopBtn->setFixedWidth(70);

    opLayout->addWidget(pauseBtn);
    opLayout->addWidget(stopBtn);
    m_table->setCellWidget(row, 9, opWidget);

    connect(pauseBtn, &QPushButton::clicked, this, &TrainManagerDialog::onPauseResumeClicked);
    connect(stopBtn, &QPushButton::clicked, this, &TrainManagerDialog::onStopClicked);

    return row;
}

void TrainManagerDialog::updateMetrics(int row, const TrainMetrics &metrics)
{
    if (row < 0 || row >= m_sessions.size()) return;

    auto &s = m_sessions[row];
    s.currentEpoch = metrics.epoch;
    s.loss = metrics.loss;
    s.boxLoss = metrics.boxLoss;
    s.clsLoss = metrics.clsLoss;
    s.dflLoss = metrics.dflLoss;
    s.precision = metrics.precision;
    s.recall = metrics.recall;
    s.mAP50 = metrics.mAP50;
    s.mAP50_95 = metrics.mAP50_95;

    auto *progressBar = qobject_cast<QProgressBar*>(m_table->cellWidget(row, 1));
    if (progressBar) {
        progressBar->setValue(metrics.epoch);
        progressBar->setFormat(QString("%1/%2").arg(metrics.epoch).arg(metrics.totalEpochs));
    }

    auto *lossLabel = qobject_cast<QLabel*>(m_table->cellWidget(row, 2));
    if (lossLabel)
        lossLabel->setText(QString::number(metrics.loss, 'f', 4));

    auto *boxLossLabel = qobject_cast<QLabel*>(m_table->cellWidget(row, 3));
    if (boxLossLabel)
        boxLossLabel->setText(QString::number(metrics.boxLoss, 'f', 4));

    auto *clsLossLabel = qobject_cast<QLabel*>(m_table->cellWidget(row, 4));
    if (clsLossLabel)
        clsLossLabel->setText(QString::number(metrics.clsLoss, 'f', 4));

    auto *dflLossLabel = qobject_cast<QLabel*>(m_table->cellWidget(row, 5));
    if (dflLossLabel)
        dflLossLabel->setText(QString::number(metrics.dflLoss, 'f', 4));

    auto *precLabel = qobject_cast<QLabel*>(m_table->cellWidget(row, 6));
    if (precLabel)
        precLabel->setText(QString::number(metrics.precision, 'f', 4));

    auto *recallLabel = qobject_cast<QLabel*>(m_table->cellWidget(row, 7));
    if (recallLabel)
        recallLabel->setText(QString::number(metrics.recall, 'f', 4));

    auto *mapLabel = qobject_cast<QLabel*>(m_table->cellWidget(row, 8));
    if (mapLabel)
        mapLabel->setText(QString::number(metrics.mAP50, 'f', 4));
}

void TrainManagerDialog::setSessionFinished(int row)
{
    if (row < 0 || row >= m_sessions.size()) return;
    m_sessions[row].running = false;

    auto *progressBar = qobject_cast<QProgressBar*>(m_table->cellWidget(row, 1));
    if (progressBar) {
        progressBar->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
    }

    auto *opWidget = m_table->cellWidget(row, 9);
    if (opWidget) {
        auto *pauseBtn = opWidget->findChild<QPushButton*>("pauseBtn");
        auto *stopBtn = opWidget->findChild<QPushButton*>("stopBtn");
        if (pauseBtn) pauseBtn->setEnabled(false);
        if (stopBtn) stopBtn->setText("已删除");
    }
}

void TrainManagerDialog::setSessionError(int row, const QString &msg)
{
    if (row < 0 || row >= m_sessions.size()) return;
    m_sessions[row].running = false;

    auto *progressBar = qobject_cast<QProgressBar*>(m_table->cellWidget(row, 1));
    if (progressBar) {
        progressBar->setStyleSheet("QProgressBar::chunk { background-color: #f44336; }");
    }

    auto *lossLabel = qobject_cast<QLabel*>(m_table->cellWidget(row, 2));
    if (lossLabel)
        lossLabel->setText("ERROR");

    auto *opWidget = m_table->cellWidget(row, 9);
    if (opWidget) {
        auto *pauseBtn = opWidget->findChild<QPushButton*>("pauseBtn");
        auto *stopBtn = opWidget->findChild<QPushButton*>("stopBtn");
        if (pauseBtn) pauseBtn->setEnabled(false);
        if (stopBtn) stopBtn->setEnabled(false);
    }
}

void TrainManagerDialog::onPauseResumeClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int row = -1;
    for (int i = 0; i < m_table->rowCount(); ++i) {
        auto *opWidget = m_table->cellWidget(i, 9);
        if (opWidget && opWidget->findChild<QPushButton*>("pauseBtn") == btn) {
            row = i;
            break;
        }
    }
    if (row < 0 || row >= m_sessions.size()) return;

    auto &s = m_sessions[row];
    if (!s.running) return;

    s.paused = !s.paused;
    btn->setText(s.paused ? "继续" : "暂停");
    emit sg_pauseResume(row, s.paused);
}

void TrainManagerDialog::onStopClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int row = -1;
    for (int i = 0; i < m_table->rowCount(); ++i) {
        auto *opWidget = m_table->cellWidget(i, 9);
        if (opWidget && opWidget->findChild<QPushButton*>("stopBtn") == btn) {
            row = i;
            break;
        }
    }
    if (row < 0 || row >= m_sessions.size()) return;

    auto &s = m_sessions[row];
    if (!s.running) return;

    auto reply = QMessageBox::question(this, "确认",
        "确定要停止训练 \"" + s.name + "\" 吗？\n将删除训练输出目录。",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    emit sg_stop(row);
}
