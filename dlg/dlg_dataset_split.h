#ifndef DLG_DATASET_SPLIT_H
#define DLG_DATASET_SPLIT_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>

class DatasetSplitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DatasetSplitDialog(QWidget *parent = nullptr);

    QString getClassName() const;
    int getMethod() const;
    double getRatio() const;
    int getLabelType() const;

signals:
    void sg_confirm(QString name, int method, double ratio, int labelType);

private slots:
    void onStartClicked();

private:
    QLineEdit *m_nameEdit;
    QComboBox *m_methodCombo;
    QDoubleSpinBox *m_ratioSpin;
    QComboBox *m_labelTypeCombo;
    QPushButton *m_startBtn;
    QPushButton *m_cancelBtn;
};

#endif // DLG_DATASET_SPLIT_H
