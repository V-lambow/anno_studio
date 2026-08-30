#ifndef DLG_ULTRAINFER_H
#define DLG_ULTRAINFER_H

#include <QDialog>
#include <QFileDialog>

namespace Ui {
class Ultrainfer;
}

struct UltrainferPara {
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
    QString projectName;
    bool existOk{false};

    // 过滤参数
    bool useConf{false};
    double confThreshold{0.25};
    bool useArea{false};
    int areaMin{100};
    int areaMax{100000};

    // 批量推理
    bool isInfer_trainList{false};
    bool isInfer_nontrainList{false};
};

Q_DECLARE_METATYPE(UltrainferPara);

class Ultrainfer : public QDialog
{
    Q_OBJECT

public:
    explicit Ultrainfer(QWidget *parent = nullptr);
    ~Ultrainfer();

    void setProjectName(const QString &name);

private slots:
    void on_modelPath_btn_clicked();
    void on_buttonBox_accepted();

    void batchInferBt(bool checked);

private:
    Ui::Ultrainfer *ui;
    UltrainferPara param_;

signals:
    void sg_ultrainferPara(UltrainferPara);
    void sg_batchInfer(bool, bool);
};

#endif // DLG_ULTRAINFER_H
