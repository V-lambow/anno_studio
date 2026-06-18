#ifndef DLG_SAIGEINFER_H
#define DLG_SAIGEINFER_H

#include <QDialog>
#include <QFileDialog>
#include <iostream>
#include "../saige/include/SaigeVisionUtil.h"

namespace Ui {
class SaigeInfer;
}
struct SaigeInferPara{
    const char* modelPath=nullptr;
    int batchSize{0};
    int inferHeight{0};
    int inferWidth{0};
    int deviceIndex{0};
    unsigned int config_flags{0};
    int saigeProjectType{0};
    bool isCalcObjectAreaAndApplyThreshold =false;
    bool isCalcObjectScoreAndApplyThreshold =false;
    int objectAreaThreshold=0;
    int objectScoreThreshold=0;
    int objectBoundaryThreshold=0;
    unsigned int option_flags{0};


    ///
    bool isInfer_trainList{false};
    bool isInfer_nontrainList{false};

};


Q_DECLARE_METATYPE(SaigeInferPara);
class SaigeInfer : public QDialog
{
    Q_OBJECT

public:
    explicit SaigeInfer(QWidget *parent = nullptr);
    ~SaigeInfer();

private slots:
    void on_pushButton_clicked();

    void on_buttonBox_accepted();

    void batchInferBt(bool bl);


    void on_modelType_cbBox_currentIndexChanged(int index);

    void on_Score_ckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_area_ckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_score_spinBox_valueChanged(int arg1);

    void on_areaDn_spinBox_valueChanged(int arg1);

    void on_boundary_spinBox_valueChanged(int arg1);

    void on_non_trainInfer_ckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_trainInfer_ckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_boundary_ckBox_checkStateChanged(const Qt::CheckState &arg1);

signals:
    void sg_saigeInferPara(SaigeInferPara);
    void sg_batchInfer(bool ,bool);

private:
    Ui::SaigeInfer *ui;
    SaigeInferPara param_;


};

#endif // DLG_SAIGEINFER_H
