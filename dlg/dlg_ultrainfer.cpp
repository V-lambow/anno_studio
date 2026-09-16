#include "dlg_ultrainfer.h"
#include "ui_dlg_ultrainfer.h"
#include <QFile>
#include <QTextStream>

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

void Ultrainfer::setRoiRect(const QRectF &roi)
{
    roiRect_ = roi;
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

void Ultrainfer::on_dataYaml_btn_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("选择数据集配置文件"), "",
        tr("YAML 文件 (*.yaml *.yml);;所有文件 (*)"));
    if (!fileName.isEmpty()) {
        ui->dataYaml_lineEdit->setText(fileName);
    }
}

void Ultrainfer::on_train_startBtn_clicked()
{
    UltrainferTrainPara param;
    param.modelPath = ui->modelPath_lineEdit->text();
    param.alias = ui->alias_lineEdit->text();
    param.dataYaml = ui->dataYaml_lineEdit->text();
    param.useRoi = ui->useRoi_ckBox->isChecked();
    if (param.useRoi)
        param.roiRect = roiRect_;
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

    if (!trainParamJsonPath_.isEmpty())
        saveToJson(trainParamJsonPath_);

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

void Ultrainfer::loadFromJson(const QString &jsonPath)
{
    if (!QFile::exists(jsonPath))
        return;

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QByteArray jsonData = file.readAll();
    file.close();

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(jsonData.toStdString(), root))
        return;

    if (root.isMember("train")) {
        Json::Value trainObj = root["train"];
        if (trainObj.isMember("modelPath"))
            ui->modelPath_lineEdit->setText(QString::fromStdString(trainObj["modelPath"].asString()));
        if (trainObj.isMember("alias"))
            ui->alias_lineEdit->setText(QString::fromStdString(trainObj["alias"].asString()));
        if (trainObj.isMember("dataYaml"))
            ui->dataYaml_lineEdit->setText(QString::fromStdString(trainObj["dataYaml"].asString()));
        if (trainObj.isMember("useRoi"))
            ui->useRoi_ckBox->setChecked(trainObj["useRoi"].asBool());
        if (trainObj.isMember("roi")) {
            Json::Value roiObj = trainObj["roi"];
            roiRect_ = QRectF(roiObj["x"].asDouble(), roiObj["y"].asDouble(),
                              roiObj["width"].asDouble(), roiObj["height"].asDouble());
        }
        if (trainObj.isMember("task")) {
            int idx = ui->task_cbBox->findText(QString::fromStdString(trainObj["task"].asString()));
            if (idx >= 0) ui->task_cbBox->setCurrentIndex(idx);
        }
        if (trainObj.isMember("device")) {
            int idx = ui->device_cbBox->findText(QString::fromStdString(trainObj["device"].asString()));
            if (idx >= 0) ui->device_cbBox->setCurrentIndex(idx);
        }
        if (trainObj.isMember("optimizer")) {
            int idx = ui->optimizer_cbBox->findText(QString::fromStdString(trainObj["optimizer"].asString()));
            if (idx >= 0) ui->optimizer_cbBox->setCurrentIndex(idx);
        }
        if (trainObj.isMember("cos_lr"))
            ui->cos_lr_ckBox->setChecked(trainObj["cos_lr"].asBool());
        if (trainObj.isMember("epochs"))
            ui->epochs_spinBox->setValue(trainObj["epochs"].asInt());
        if (trainObj.isMember("batch"))
            ui->batch_spinBox->setValue(trainObj["batch"].asInt());
        if (trainObj.isMember("imgsz"))
            ui->imgsz_spinBox->setValue(trainObj["imgsz"].asInt());
        if (trainObj.isMember("amp"))
            ui->amp_ckBox->setChecked(trainObj["amp"].asBool());
        if (trainObj.isMember("resume"))
            ui->resume_ckBox->setChecked(trainObj["resume"].asBool());
    }

    if (root.isMember("infer")) {
        Json::Value inferObj = root["infer"];
        if (inferObj.isMember("conf_ckBox"))
            ui->conf_ckBox->setChecked(inferObj["conf_ckBox"].asBool());
        if (inferObj.isMember("confThreshold"))
            ui->conf_spinBox->setValue(inferObj["confThreshold"].asDouble());
        if (inferObj.isMember("area_ckBox"))
            ui->area_ckBox->setChecked(inferObj["area_ckBox"].asBool());
        if (inferObj.isMember("areaMin"))
            ui->areaMin_spinBox->setValue(inferObj["areaMin"].asInt());
        if (inferObj.isMember("areaMax"))
            ui->areaMax_spinBox->setValue(inferObj["areaMax"].asInt());
    }
}

void Ultrainfer::saveToJson(const QString &jsonPath)
{
    Json::Value root;

    Json::Value trainObj;
    trainObj["modelPath"] = ui->modelPath_lineEdit->text().toStdString();
    trainObj["alias"] = ui->alias_lineEdit->text().toStdString();
    trainObj["dataYaml"] = ui->dataYaml_lineEdit->text().toStdString();
    trainObj["useRoi"] = ui->useRoi_ckBox->isChecked();
    Json::Value roiObj;
    roiObj["x"] = roiRect_.x();
    roiObj["y"] = roiRect_.y();
    roiObj["width"] = roiRect_.width();
    roiObj["height"] = roiRect_.height();
    trainObj["roi"] = roiObj;
    trainObj["port"] = 9257;
    trainObj["task"] = ui->task_cbBox->currentText().toStdString();
    trainObj["device"] = ui->device_cbBox->currentText().toStdString();
    trainObj["optimizer"] = ui->optimizer_cbBox->currentText().toStdString();
    trainObj["cos_lr"] = ui->cos_lr_ckBox->isChecked();
    trainObj["epochs"] = ui->epochs_spinBox->value();
    trainObj["batch"] = ui->batch_spinBox->value();
    trainObj["imgsz"] = ui->imgsz_spinBox->value();
    trainObj["amp"] = ui->amp_ckBox->isChecked();
    trainObj["resume"] = ui->resume_ckBox->isChecked();
    root["train"] = trainObj;

    Json::Value inferObj;
    inferObj["conf_ckBox"] = ui->conf_ckBox->isChecked();
    inferObj["confThreshold"] = ui->conf_spinBox->value();
    inferObj["area_ckBox"] = ui->area_ckBox->isChecked();
    inferObj["areaMin"] = ui->areaMin_spinBox->value();
    inferObj["areaMax"] = ui->areaMax_spinBox->value();
    root["infer"] = inferObj;

    Json::FastWriter writer;
    std::string jsonStr = writer.write(root);

    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << QString::fromStdString(jsonStr);
    }
}
