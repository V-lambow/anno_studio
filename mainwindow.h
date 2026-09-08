#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qapplication.h>
#include <qgraphicsitem.h>
#include <QKeyEvent>
#include <QShortcut>
#include <QToolBar>
#include <QSpinBox>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTimer>
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
#include "dlg/dlg_ultrainfer.h"
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
#include "MyGraphics/roioverlayitem.h"
#include "MyGraphics/canvasadjustmentwidget.h"
#include "dlg/dlg_labelstatistics.h"








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

    void on_actioncreate_rect_triggered();

    void on_actionsaige_infer_triggered();

    void set_saigeInferPara(SaigeInferPara param);

    void on_actionultra_infer_triggered();

    void set_ultrainferTrainPara(UltrainferTrainPara param);
    void set_ultrainferInferPara(UltrainferInferPara param);
    void on_ultra_batchInfer(UltrainferBatchInferPara param);

    void on_infer_ckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_drawEffect_ckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_inferRes_ckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_actioninferPattern_xml_triggered();

    void on_actioninferPattern_hdict_triggered();

    void on_actionExportYoloHBB_triggered();
    void on_actionExportYoloSeg_triggered();
    void on_actionExportVocDet_triggered();
    void on_actionExportVocSeg_triggered();
    void on_actionExportCocoDet_triggered();
    void on_actionExportCocoSeg_triggered();

    void on_actionadd_image_triggered();

    void on_actionadd_imagefold_triggered();

    void on_actionoh_test_triggered();
    void on_actionShortcuts_triggered();
    void on_actionLabelStatistics_triggered();
    void saveLabelingData();
    void mergeOverlappingStrokes();
    void on_actionCreateRoi_triggered();

    void sl_onBrightnessContrastChanged(int brightness, int contrast);
    void sl_onAnnotationOpacityChanged(int opacity);
    void sl_showAdjustPopup();

signals:
    void sg_imageInfo_label(QString,int,int);
    void sg_penWidth_changed(int);
    void sg_penColor_changed(Qt::GlobalColor);

private:
    void setupGraphicsView();
    void enableDrawActions(bool enabled);
    void setupShortcuts();
    void navigateList(int delta);
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void showSegInferLabel();
    void showLabelingPatterns();
    bool writeSegInferXML(const char* xmlPath, LabelInfo& labelInfo);
    bool writeSegInferXMLsb(const char* xmlPath, LabelInfo& labelInfo);
    bool writeSegInferHdict(const char* HictPath, LabelInfo& labelInfo);
    int nextMultipleOf32(int n);
    void add_non_train_list(QVector<QString> appendFiles);
    std::variant<std::monostate,QString> saigeMoelInit(SaigeInferPara param);
    void saveRoi();
    void loadRoi();
    void displayRoi();
    void saveCategories();
    void loadCategories();
    void syncFilterComboBoxes();
    void applyFilter();

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
    QImage m_currentImage;

    VisibilityManager *m_visibilityMgr;
    QList<QGraphicsItem*> m_labelingItems;

    // ROI
    QRectF m_roiRect;
    RoiOverlayItem *m_roiOverlayItem = nullptr;

    // 类别筛选
    QStringList m_categories;
    QStringList m_allImageFiles;
    QListWidget *m_activeList = nullptr;

    // W/S 长按调笔触
    QTimer *m_penSizeTimer = nullptr;
    int m_penSizeDelta = 0;

    // 亮度对比度调节
    CanvasAdjustmentWidget *m_canvasAdjustment = nullptr;
    QPixmap m_originalPixmap;
    QGraphicsPixmapItem *m_pixmapItem = nullptr;
};
#endif // MAINWINDOW_H
