#ifndef DLG_ULTRAINFER_H
#define DLG_ULTRAINFER_H

#include <QDialog>
#include <QFileDialog>

namespace Ui {
class Ultrainfer;
}

struct UltrainferTrainPara {
    QString modelPath;
    QString task;
    QString device;
    QString optimizer;
    bool cos_lr{false};
    int epochs{100};
    int batch{16};
    int imgsz{640};
    bool amp{true};
    bool resume{false};
};

struct UltrainferInferPara {
    QString modelPath;
    bool useConf{false};
    double confThreshold{0.25};
    bool useArea{false};
    int areaMin{100};
    int areaMax{100000};
};

Q_DECLARE_METATYPE(UltrainferTrainPara);
struct UltrainferBatchInferPara {
    bool useNonTrain{false};
    bool useTrain{false};
};

Q_DECLARE_METATYPE(UltrainferInferPara);
Q_DECLARE_METATYPE(UltrainferBatchInferPara);

class Ultrainfer : public QDialog
{
    Q_OBJECT

public:
    explicit Ultrainfer(QWidget *parent = nullptr);
    ~Ultrainfer();

    void setProjectName(const QString &name);
    void setInferModelList(const QStringList &models);

private slots:
    void on_modelPath_btn_clicked();
    void on_train_startBtn_clicked();
    void on_infer_confirmBtn_clicked();
    void on_inferModel_refreshBtn_clicked();
    void on_batchInfer_startBtn_clicked();
    void batchInferBt(bool checked);

private:
    Ui::Ultrainfer *ui;
    QString projectName_;

signals:
    void sg_train(UltrainferTrainPara);
    void sg_inferConfirm(UltrainferInferPara);
    void sg_batchInfer(UltrainferBatchInferPara);
    void sg_refreshModels(QStringList &models);
};

#endif // DLG_ULTRAINFER_H
