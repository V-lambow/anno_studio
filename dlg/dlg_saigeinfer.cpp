#include "dlg_saigeinfer.h"
#include "ui_dlg_saigeinfer.h"

SaigeInfer::SaigeInfer(QWidget *parent)
    : QDialog(parent), ui(new Ui::SaigeInfer)
{
    ui->setupUi(this);

    param_.batchSize = ui->batchsize_cbBox->itemText(ui->batchsize_cbBox->currentIndex()).toInt();
    param_.inferHeight = ui->height_cbBox->itemText(ui->height_cbBox->currentIndex()).toInt();
    param_.inferWidth = ui->width_cbBox->itemText(ui->width_cbBox->currentIndex()).toInt();
    param_.saigeProjectType = ui->modelType_cbBox->currentIndex();
    if (ui->batchSize_ckBox->isChecked())
    {
        param_.config_flags |= SAIGE_SET_BATCH_SIZE;
    }
    if (ui->inferSize_ckBox->isChecked())
    {
        param_.config_flags |= SAIGE_SET_INSPECTION_SIZE;
    }
    if (ui->optimize_ckBox->isChecked())
    {
        param_.config_flags |= SAIGE_OPTIMIZE_HANDLE;
    }
    connect(ui->isBatchInferButton, &QRadioButton::clicked, this, &SaigeInfer::batchInferBt);

    if (ui->modelType_cbBox->currentIndex() == 2)
    {
        ui->Seg_gpBox->setEnabled(true);
    }
    else
    {
        ui->Seg_gpBox->setEnabled(false);
    }
}

SaigeInfer::~SaigeInfer()
{
    if (param_.modelPath)
    {
        delete[] const_cast<char *>(param_.modelPath);
        param_.modelPath = nullptr;
    }
    delete ui;
}

void SaigeInfer::on_pushButton_clicked()
{
    // 打开qt文件选择对话框，并将路径显示在旁边的qlineedit上
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*);;Text Files (*.txt)"));
    if (!fileName.isEmpty())
    {
        ui->lineEdit->setText(fileName);
    }
    // auto str1 =fileName.toUtf8();
    // auto str2 =fileName.toLatin1();
    // auto str3 =fileName.toLocal8Bit();

    QByteArray byteArray = fileName.toUtf8(); // 使用Utf8更好地支持中文路径

    param_.modelPath = new char[byteArray.size() + 1]; // +1 用于存储字符串结束符 '\0'
    strcpy(const_cast<char *>(param_.modelPath), byteArray.data());
}

void SaigeInfer::on_buttonBox_accepted()
{

    param_.batchSize = ui->batchsize_cbBox->itemText(ui->batchsize_cbBox->currentIndex()).toInt();
    param_.inferHeight = ui->height_cbBox->itemText(ui->height_cbBox->currentIndex()).toInt();
    param_.inferWidth = ui->width_cbBox->itemText(ui->width_cbBox->currentIndex()).toInt();
    param_.saigeProjectType = ui->modelType_cbBox->currentIndex();
    if (ui->batchSize_ckBox->isChecked())
    {
        param_.config_flags |= SAIGE_SET_BATCH_SIZE;
    }
    if (ui->inferSize_ckBox->isChecked())
    {
        param_.config_flags |= SAIGE_SET_INSPECTION_SIZE;
    }
    if (ui->optimize_ckBox->isChecked())
    {
        param_.config_flags |= SAIGE_OPTIMIZE_HANDLE;
    }
    // 如果文件为空，不发送信号
    if (param_.modelPath == nullptr || param_.modelPath == "")
    {
        return;
    }
    emit sg_saigeInferPara(param_);
    if (ui->isBatchInferButton->isChecked())
    {
        emit sg_batchInfer(ui->non_trainInfer_ckBox->isChecked(),
                           ui->trainInfer_ckBox->isChecked());
    }
}

void SaigeInfer::batchInferBt(bool bl)
{
    ui->non_trainInfer_ckBox->setEnabled(bl);
    ui->trainInfer_ckBox->setEnabled(bl);
}

void SaigeInfer::on_modelType_cbBox_currentIndexChanged(int index)
{
    if (index == 2)
    {
        ui->Seg_gpBox->setEnabled(true);
    }
    else
    {
        ui->Seg_gpBox->setEnabled(false);
    }
}

void SaigeInfer::on_Score_ckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    arg1 == Qt::Checked ? param_.isCalcObjectScoreAndApplyThreshold = true
                        : param_.isCalcObjectScoreAndApplyThreshold = false;
    arg1 == Qt::Checked ? param_.option_flags |= SAIGE_SET_OBJECT_SCORE_THRESHOLD : param_.option_flags &= ~SAIGE_SET_OBJECT_SCORE_THRESHOLD;
}

void SaigeInfer::on_area_ckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    arg1 == Qt::Checked ? param_.isCalcObjectAreaAndApplyThreshold = true : param_.isCalcObjectAreaAndApplyThreshold = false;
    arg1 == Qt::Checked ? param_.option_flags |= SAIGE_SET_OBJECT_AREA_THRESHOLD : param_.option_flags &= ~SAIGE_SET_OBJECT_AREA_THRESHOLD;
}

void SaigeInfer::on_score_spinBox_valueChanged(int arg1)
{
    param_.objectScoreThreshold = arg1;
}

void SaigeInfer::on_areaDn_spinBox_valueChanged(int arg1)
{
    param_.objectAreaThreshold = arg1;
}


void SaigeInfer::on_boundary_spinBox_valueChanged(int arg1)
{
    param_.objectBoundaryThreshold = arg1;
}

void SaigeInfer::on_non_trainInfer_ckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    arg1 == Qt::Checked ? param_.isInfer_nontrainList = true : param_.isInfer_nontrainList = false;
}

void SaigeInfer::on_trainInfer_ckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    arg1 == Qt::Checked ? param_.isInfer_trainList = true : param_.isInfer_trainList = false;
}

void SaigeInfer::on_boundary_ckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    arg1 == Qt::Checked ? param_.option_flags |= SAIGE_SET_OBJECT_BOUNDARY_THRESHOLD : param_.option_flags &= ~SAIGE_SET_OBJECT_BOUNDARY_THRESHOLD;

}

