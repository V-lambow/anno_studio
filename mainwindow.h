#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qapplication.h>
#include <qgraphicsitem.h>
#include <QToolBar>
#include <QSpinBox>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include "MyGraphics/mygraphicsview.h"
#include "MyGraphics/painteditem.h"
#include "MyGraphics/visibilitymanager.h"
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <iostream>
#include "dlg/dlg_saigeinfer.h"
#include "saige/include/SaigeVisionUtil.h"
#include <QMessageBox>
#include <QRandomGenerator>
#include <QXmlStreamWriter>
#include <QProgressDialog>
#include <json/json.h>
#include "Halcon.h"
#include "halconcpp/HalconCpp.h"
#include <math.h>
#include <algorithm>
#include "base_head.hpp"
#include "saigeutils.h"
#include <QProcess>
#include <QFuture>
#include <variant>








QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private slots:
    void onPenWidthChanged(int width);
    void onLabelColorChanged(int index);
    void onAddLabel();
    void onRemoveLabel();
    void onCursorActionTriggered();
    // void onItemHovered(MyGraphicsItem *item);
    // void onSceneSelectionChanged();

    void on_actioncreate_newProject_triggered();

    void on_actionopen_project_triggered();

    ///
    void on_non_train_list_itemPressed(QListWidgetItem *item);
    void showNon_trainlistMenu(const QPoint &pos);
    void showtrainlistMenu(const QPoint &pos);


    void on_actionmoveinTrainlist_triggered();

    void on_actionremoveTrainlist_triggered();

    void on_actiondelete_non_trainlistele_triggered();

    void on_actiondelete_trainlistele_triggered();

    void on_non_train_list_itemDoubleClicked(QListWidgetItem *item);

    void on_train_list_itemDoubleClicked(QListWidgetItem *item);

    void on_actioneraser_triggered();

    void on_actioncursor_triggered();

    void on_actionpen_brush_triggered();

    void on_actionsaige_infer_triggered();

    void set_saigeInferPara(SaigeInferPara param);

    void on_infer_ckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_drawEffect_ckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_inferRes_ckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_actioninferPattern_xml_triggered();

    void on_actioninferPattern_hdict_triggered();



    void on_actionadd_image_triggered();

    void on_actionadd_imagefold_triggered();

    void on_actionoh_test_triggered();
    void saveLabelingData();
    void mergeOverlappingStrokes();

signals:
    void sg_imageInfo_label(QString,int,int);
    void sg_penWidth_changed(int);
    void sg_penColor_changed(Qt::GlobalColor);

private:
    void setupGraphicsView();
    void enableDrawActions(bool enabled);

    void showSegInferLabel();
    void showLabelingPatterns();
    bool writeSegInferXML(const char* xmlPath, LabelInfo& labelInfo);
    bool writeSegInferXMLsb(const char* xmlPath, LabelInfo& labelInfo);
    bool writeSegInferHdict(const char* HictPath, LabelInfo& labelInfo);
    int nextMultipleOf32(int n);
    void add_non_train_list(QVector<QString> appendFiles);
    std::variant<std::monostate,QString> saigeMoelInit(SaigeInferPara param);

private:
    Ui::MainWindow *ui;
    // 工具状态
    QGraphicsScene *m_scene;
    MyGraphicsView *m_view;
    
    // UI 组件指针
    QSpinBox *m_penWidthSpinBox;
    QComboBox *m_labelsComboBox;
    QPushButton *m_addLabelBtn;
    QPushButton *m_removeLabelBtn;
    
    // 状态显示
    QLabel *m_statusLabel;

    //当前项目路径
    QString m_curProjectPath;
    //推理参数
    InferPara m_inferPara;

    SaigeHandle m_saigeHdl;
    bool m_saigeAvailable = false;

    LabelInfo m_curLabelInfo;
    IMAGEINFO m_displayedImageInfo;

    VisibilityManager *m_visibilityMgr;
    QList<PaintedItem*> m_labelingItems;
};
#endif // MAINWINDOW_H
