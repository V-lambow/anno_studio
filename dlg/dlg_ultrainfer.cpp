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
    projectName_ = name;
}

void Ultrainfer::setInferModelList(const QStringList &models)
{
    ui->inferModel_cbBox->clear();
    ui->inferModel_cbBox->addItems(models);
}

void Ultrainfer::on_modelPath_btn_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("选择模型文件"), "",
        tr("PyTorch 模型 (*.pt);;ONNX 模型 (*.onnx);;所有文件 (*)"));
    if (!fileName.isEmpty()) {
        ui->modelPath_lineEdit->setText(fileName);
    }
}

void Ultrainfer::on_train_startBtn_clicked()
{
    UltrainferTrainPara param;
    param.modelPath = ui->modelPath_lineEdit->text();
    param.task = ui->task_cbBox->currentText();
    param.device = ui->device_cbBox->currentText();
    param.optimizer = ui->optimizer_cbBox->currentText();
    param.cos_lr = ui->cos_lr_ckBox->isChecked();
    param.epochs = ui->epochs_spinBox->value();
    param.batch = ui->batch_spinBox->value();
    param.imgsz = ui->imgsz_spinBox->value();
    param.amp = ui->amp_ckBox->isChecked();
    param.resume = ui->resume_ckBox->isChecked();

    if (param.modelPath.isEmpty())
        return;

    emit sg_train(param);
}

void Ultrainfer::on_infer_confirmBtn_clicked()
{
    UltrainferInferPara param;
    param.modelPath = ui->inferModel_cbBox->currentText();
    param.useConf = ui->conf_ckBox->isChecked();
    param.confThreshold = ui->conf_spinBox->value();
    param.useArea = ui->area_ckBox->isChecked();
    param.areaMin = ui->areaMin_spinBox->value();
    param.areaMax = ui->areaMax_spinBox->value();

    if (param.modelPath.isEmpty())
        return;

    emit sg_inferConfirm(param);
}

void Ultrainfer::on_inferModel_refreshBtn_clicked()
{
    QStringList models;
    emit sg_refreshModels(models);
    setInferModelList(models);
}

void Ultrainfer::on_batchInfer_startBtn_clicked()
{
    if (!ui->isBatchInferButton->isChecked())
        return;

    UltrainferBatchInferPara param;
    param.useNonTrain = ui->non_trainInfer_ckBox->isChecked();
    param.useTrain = ui->trainInfer_ckBox->isChecked();
    emit sg_batchInfer(param);
}

void Ultrainfer::batchInferBt(bool checked)
{
    ui->non_trainInfer_ckBox->setEnabled(checked);
    ui->trainInfer_ckBox->setEnabled(checked);
}
