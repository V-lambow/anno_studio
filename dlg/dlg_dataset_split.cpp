#include "dlg_dataset_split.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>

DatasetSplitDialog::DatasetSplitDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("数据集创建");
    setMinimumWidth(400);

    auto *mainLayout = new QVBoxLayout(this);
    auto *formLayout = new QFormLayout();

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("输入分类名字");
    formLayout->addRow("分类名字:", m_nameEdit);

    m_methodCombo = new QComboBox(this);
    m_methodCombo->addItem("普通");
    m_methodCombo->addItem("聚类");
    formLayout->addRow("分类方式:", m_methodCombo);

    m_ratioSpin = new QDoubleSpinBox(this);
    m_ratioSpin->setRange(0.01, 0.99);
    m_ratioSpin->setSingleStep(0.05);
    m_ratioSpin->setDecimals(2);
    m_ratioSpin->setValue(0.8);
    formLayout->addRow("训练集比例:", m_ratioSpin);

    m_labelTypeCombo = new QComboBox(this);
    m_labelTypeCombo->addItem("yolohbb");
    m_labelTypeCombo->addItem("yoloseg");
    m_labelTypeCombo->addItem("vochbb");
    m_labelTypeCombo->addItem("vocseg");
    m_labelTypeCombo->addItem("cocohbb");
    m_labelTypeCombo->addItem("cocoseg");
    formLayout->addRow("Label类型:", m_labelTypeCombo);

    mainLayout->addLayout(formLayout);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_startBtn = new QPushButton("开始分类", this);
    m_cancelBtn = new QPushButton("取消", this);
    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_startBtn, &QPushButton::clicked, this, &DatasetSplitDialog::onStartClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QString DatasetSplitDialog::getClassName() const { return m_nameEdit->text().trimmed(); }
int DatasetSplitDialog::getMethod() const { return m_methodCombo->currentIndex(); }
double DatasetSplitDialog::getRatio() const { return m_ratioSpin->value(); }
int DatasetSplitDialog::getLabelType() const { return m_labelTypeCombo->currentIndex(); }

void DatasetSplitDialog::onStartClicked()
{
    if (getClassName().isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入分类名字");
        return;
    }
    emit sg_confirm(getClassName(), getMethod(), getRatio(), getLabelType());
    accept();
}
