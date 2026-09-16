#include "dlg_dataset_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QTextStream>

DatasetManagerDialog::DatasetManagerDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_projectPath(projectPath)
{
    setWindowTitle("数据集管理");
    setMinimumSize(480, 320);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({"名称", "操作"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QTableWidget::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_table);

    loadData();
}

void DatasetManagerDialog::loadData()
{
    QString jsonPath = m_projectPath + "/data_config.json";
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QByteArray jsonData = file.readAll();
    file.close();

    Json::Reader reader;
    if (!reader.parse(jsonData.toStdString(), m_root)) return;

    if (!m_root.isMember("data_group")) return;

    const Json::Value &arr = m_root["data_group"];
    m_blockSignals = true;
    m_table->setRowCount(arr.size());

    for (Json::ArrayIndex i = 0; i < arr.size(); ++i) {
        QString name = QString::fromStdString(arr[i]["name"].asString());

        auto *nameItem = new QTableWidgetItem(name);
        nameItem->setData(Qt::UserRole, name);
        m_table->setItem(i, 0, nameItem);

        auto *delBtn = new QPushButton("删除", this);
        connect(delBtn, &QPushButton::clicked, this, &DatasetManagerDialog::onDeleteClicked);
        m_table->setCellWidget(i, 1, delBtn);
    }
    m_blockSignals = false;

    connect(m_table, &QTableWidget::cellChanged, this, &DatasetManagerDialog::onCellChanged);
}

void DatasetManagerDialog::saveData()
{
    Json::Value arr(Json::arrayValue);
    for (int i = 0; i < m_table->rowCount(); ++i) {
        QString name = m_table->item(i, 0)->data(Qt::UserRole).toString();
        QString yamlPath = "datayaml/" + name + ".yaml";

        Json::Value groupObj;
        groupObj["name"] = name.toStdString();
        groupObj["yaml_path"] = yamlPath.toStdString();
        arr.append(groupObj);
    }
    m_root["data_group"] = arr;

    Json::FastWriter writer;
    std::string jsonStr = writer.write(m_root);

    QString jsonPath = m_projectPath + "/data_config.json";
    QFile outFile(jsonPath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&outFile);
        out << QString::fromStdString(jsonStr);
    }
}

void DatasetManagerDialog::onCellChanged(int row, int column)
{
    if (m_blockSignals || column != 0) return;

    QTableWidgetItem *item = m_table->item(row, 0);
    QString oldName = item->data(Qt::UserRole).toString();
    QString newName = item->text().trimmed();

    if (newName.isEmpty() || newName == oldName) {
        m_blockSignals = true;
        item->setText(oldName);
        m_blockSignals = false;
        return;
    }

    for (int i = 0; i < m_table->rowCount(); ++i) {
        if (i != row && m_table->item(i, 0)->text() == newName) {
            QMessageBox::warning(this, "警告", "名称已存在");
            m_blockSignals = true;
            item->setText(oldName);
            m_blockSignals = false;
            return;
        }
    }

    QString oldDatasetDir = m_projectPath + "/" + oldName;
    QString newDatasetDir = m_projectPath + "/" + newName;
    if (QDir(oldDatasetDir).exists() && !QDir(newDatasetDir).exists()) {
        QDir().rename(oldDatasetDir, newDatasetDir);
    }

    QString oldYaml = m_projectPath + "/datayaml/" + oldName + ".yaml";
    QString newYaml = m_projectPath + "/datayaml/" + newName + ".yaml";
    if (QFile::exists(oldYaml) && !QFile::exists(newYaml)) {
        QFile::rename(oldYaml, newYaml);
    }

    if (QFile::exists(newYaml)) {
        QFile yf(newYaml);
        if (yf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = yf.readAll();
            yf.close();
            QString content = QString::fromUtf8(data);
            content.replace(oldDatasetDir, newDatasetDir);
            QFile yfOut(newYaml);
            if (yfOut.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&yfOut);
                out << content;
            }
        }
    }

    m_blockSignals = true;
    item->setData(Qt::UserRole, newName);
    m_blockSignals = false;

    saveData();
}

void DatasetManagerDialog::onDeleteClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int row = -1;
    for (int i = 0; i < m_table->rowCount(); ++i) {
        if (m_table->cellWidget(i, 1) == btn) {
            row = i;
            break;
        }
    }
    if (row < 0) return;

    QString name = m_table->item(row, 0)->data(Qt::UserRole).toString();

    auto reply = QMessageBox::question(this, "确认",
        "确定要删除数据集 \"" + name + "\" 吗？\n将同时删除对应的文件夹和yaml文件。",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QString datasetDir = m_projectPath + "/" + name;
    if (QDir(datasetDir).exists()) {
        QDir(datasetDir).removeRecursively();
    }

    QString yamlPath = m_projectPath + "/datayaml/" + name + ".yaml";
    if (QFile::exists(yamlPath)) {
        QFile::remove(yamlPath);
    }

    m_blockSignals = true;
    m_table->removeRow(row);
    m_blockSignals = false;

    saveData();
}
