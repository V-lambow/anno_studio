#ifndef DLG_DATASET_MANAGER_H
#define DLG_DATASET_MANAGER_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <json/json.h>

class DatasetManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DatasetManagerDialog(const QString &projectPath, QWidget *parent = nullptr);

private slots:
    void onCellChanged(int row, int column);
    void onDeleteClicked();

private:
    void loadData();
    void saveData();

    QString m_projectPath;
    Json::Value m_root;
    QTableWidget *m_table;
    bool m_blockSignals = false;
};

#endif // DLG_DATASET_MANAGER_H
