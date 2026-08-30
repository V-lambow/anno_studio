#include "dlg_ultrainfer.h"
#include "ui_dlg_ultrainfer.h"

Ultrainfer::Ultrainfer(QWidget *parent)
    : QDialog(parent), ui(new Ui::Ultrainfer)
{
    ui->setupUi(this);
    connect(ui->isBatchInferButton, &QRadioButton::clicked, this, &Ultrainfer::batchInferBt);
}

Ultrainfer::~Ultrainfer()
{
    delete ui;
}

void Ultrainfer::setProjectName(const QString &name)
{
    param_.projectName = name;
}

void Ultrainfer::on_modelPath_btn_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("选择模型文件"), "",
        tr("PyTorch 模型 (*.pt);;ONNX 模型 (*.onnx);;所有文件 (*)"));
    if (!fileName.isEmpty()) {
        ui->modelPath_lineEdit->setText(fileName);
        param_.modelPath = fileName;
    }
}

void Ultrainfer::on_buttonBox_accepted()
{
    param_.modelPath = ui->modelPath_lineEdit->text();
    param_.task = ui->task_cbBox->currentText();
    param_.device = ui->device_cbBox->currentText();
    param_.optimizer = ui->optimizer_cbBox->currentText();
    param_.cos_lr = ui->cos_lr_ckBox->isChecked();
    param_.epochs = ui->epochs_spinBox->value();
    param_.batch = ui->batch_spinBox->value();
    param_.imgsz = ui->imgsz_spinBox->value();
    param_.amp = ui->amp_ckBox->isChecked();
    param_.resume = ui->resume_ckBox->isChecked();

    param_.useConf = ui->conf_ckBox->isChecked();
    param_.confThreshold = ui->conf_spinBox->value();
    param_.useArea = ui->area_ckBox->isChecked();
    param_.areaMin = ui->areaMin_spinBox->value();
    param_.areaMax = ui->areaMax_spinBox->value();

    if (param_.modelPath.isEmpty())
        return;

    emit sg_ultrainferPara(param_);
    if (ui->isBatchInferButton->isChecked()) {
        emit sg_batchInfer(ui->non_trainInfer_ckBox->isChecked(),
                           ui->trainInfer_ckBox->isChecked());
    }
}

void Ultrainfer::batchInferBt(bool checked)
{
    ui->non_trainInfer_ckBox->setEnabled(checked);
    ui->trainInfer_ckBox->setEnabled(checked);
}
