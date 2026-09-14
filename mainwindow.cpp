#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QToolBar>
#include <QSpinBox>
#include <QLabel>
#include <QStyledItemDelegate>
#include <QPainter>
#include "utils/include/label_exporter.h"

class ColorDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else if (option.state & QStyle::State_MouseOver)
            painter->fillRect(option.rect, option.palette.light());

        QColor color = index.data(Qt::UserRole).value<QColor>();
        if (color.isValid()) {
            QRect colorRect(option.rect.left() + 4, option.rect.top() + 3, 14, 14);
            painter->setPen(Qt::gray);
            painter->setBrush(color);
            painter->drawRect(colorRect);
        }

        QRect textRect = option.rect.adjusted(22, 0, 0, 0);
        painter->setPen(option.state & QStyle::State_Selected ? option.palette.highlightedText().color() : option.palette.text().color());
        painter->drawText(textRect, Qt::AlignVCenter, index.data(Qt::DisplayRole).toString());
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(option); Q_UNUSED(index);
        return QSize(0, 22);
    }
};
#include <QComboBox>
#include <QVariant>
#include <QDebug>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLibrary>
#include <QDialog>
#include <QTableWidget>
#include <QHeaderView>
#include "MyGraphics/inferpattern.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
// , m_scene(nullptr)
// , m_view(nullptr)
// , m_penWidthSpinBox(nullptr)
// , m_labelsComboBox(nullptr)
// , m_statusLabel(nullptr)
{
    ui->setupUi(this);

    QToolBar *penToolBar = new QToolBar("画笔工具栏", this);
    addToolBar(Qt::LeftToolBarArea, penToolBar);
    penToolBar->setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea);
    penToolBar->setFloatable(false);
    // 添加画笔粗细整数
    m_penWidthSpinBox = new QSpinBox(this);
    // 画笔粗细添加标题
    penToolBar->addWidget(new QLabel("画笔粗细:", this));
    m_penWidthSpinBox->setRange(1, 100);
    m_penWidthSpinBox->setValue(10);
    penToolBar->addWidget(m_penWidthSpinBox);
    connect(m_penWidthSpinBox, &QSpinBox::valueChanged, this, [this](int penWid)
            { ui->graphicsView->setPenWidth(penWid); });

    m_penSizeTimer = new QTimer(this);
    m_penSizeTimer->setInterval(30);
    connect(m_penSizeTimer, &QTimer::timeout, this, [this]{
        int val = m_penWidthSpinBox->value() + m_penSizeDelta;
        m_penWidthSpinBox->setValue(qBound(1, val, 100));
    });

    // 添加标签下拉栏
    m_labelsComboBox = new QComboBox(this);
    m_labelsComboBox->setItemDelegate(new ColorDelegate(m_labelsComboBox));
    m_labelsComboBox->addItem("阳极", QVariant::fromValue(QColor(Qt::red)));
    m_labelsComboBox->addItem("阴极", QVariant::fromValue(QColor(Qt::green)));
    m_labelsComboBox->addItem("其他", QVariant::fromValue(QColor(Qt::blue)));
    connect(m_labelsComboBox, &QComboBox::currentIndexChanged, this, [this](int idx)
            {
        if (idx < 0) return;
        QColor color = m_labelsComboBox->itemData(idx).value<QColor>();
        ui->graphicsView->setPenColor(color);
        ui->graphicsView->setCurrentClassId(idx); });

    m_addLabelBtn = new QPushButton("+", this);
    m_addLabelBtn->setFixedWidth(24);
    m_addLabelBtn->setToolTip("添加标签");
    connect(m_addLabelBtn, &QPushButton::clicked, this, &MainWindow::onAddLabel);

    m_removeLabelBtn = new QPushButton("-", this);
    m_removeLabelBtn->setFixedWidth(24);
    m_removeLabelBtn->setToolTip("删除当前标签");
    connect(m_removeLabelBtn, &QPushButton::clicked, this, &MainWindow::onRemoveLabel);

    QWidget *labelGroup = new QWidget(this);
    QVBoxLayout *labelGroupLayout = new QVBoxLayout(labelGroup);
    labelGroupLayout->setContentsMargins(0, 0, 0, 0);
    labelGroupLayout->setSpacing(2);
    labelGroupLayout->addWidget(m_labelsComboBox);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(2);
    btnLayout->addStretch();
    btnLayout->addWidget(m_addLabelBtn);
    btnLayout->addWidget(m_removeLabelBtn);
    labelGroupLayout->addLayout(btnLayout);
    penToolBar->addWidget(new QLabel("标签颜色:", this));
    penToolBar->addWidget(labelGroup);

    penToolBar->addSeparator();

    penToolBar->addAction(ui->actioncursor);
    penToolBar->addAction(ui->actionpen_brush);
    penToolBar->addAction(ui->actioncreate_rect);
    penToolBar->addAction(ui->actioneraser);
    penToolBar->addSeparator();

    penToolBar->addAction(ui->actionCreateRoi);
    enableDrawActions(false);

    /// CreateRoi信号连接
    connect(ui->actionCreateRoi, &QAction::triggered, this, &MainWindow::on_actionCreateRoi_triggered);
    connect(ui->graphicsView, &MyGraphicsView::sg_roiDrawn, this, [this](const QRectF &rect) {
        m_roiRect = rect;
        saveRoi();
        displayRoi();
        ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    });

    /// 初始化筛选ComboBox
    ui->labelFilterComboBox->addItem("全部");
    ui->inferFilterComboBox->addItem("全部");
    syncFilterComboBoxes();
    connect(ui->labelFilterComboBox, &QComboBox::currentIndexChanged, this, [this](int) { applyFilter(); });
    connect(ui->inferFilterComboBox, &QComboBox::currentIndexChanged, this, [this](int) { applyFilter(); });

    /// 待训练集右键菜单栏
    connect(ui->non_train_list, &QListWidget::customContextMenuRequested,
            this, &MainWindow::showNon_trainlistMenu);
    connect(ui->train_list, &QListWidget::customContextMenuRequested,
            this, &MainWindow::showtrainlistMenu);

    /// 实时显示坐标和像素值
    QLabel *pixLabel = new QLabel("(row,col)", this);
    ui->statusbar->addWidget(pixLabel);
    connect(ui->graphicsView, &MyGraphicsView::sg_mouseMoved, this, [pixLabel, this](const QPointF &pt)
            {
                int row = static_cast<int>(pt.y());
                int col = static_cast<int>(pt.x());
                if (m_currentImage.isNull() || row < 0 || row >= m_currentImage.height()
                    || col < 0 || col >= m_currentImage.width())
                {
                    pixLabel->setText(QString("(row=%1, col=%2)").arg(row).arg(col));
                    return;
                }
                QImage::Format fmt = m_currentImage.format();
                if (fmt == QImage::Format_Grayscale8)
                {
                    const auto *line = reinterpret_cast<const quint8*>(m_currentImage.constScanLine(row));
                    pixLabel->setText(QString("(%1,%2):(gray=%3)").arg(row).arg(col).arg(line[col]));
                }
                else if (fmt == QImage::Format_Grayscale16)
                {
                    const auto *line = reinterpret_cast<const quint16*>(m_currentImage.constScanLine(row));
                    pixLabel->setText(QString("(%1,%2):(gray=%3)").arg(row).arg(col).arg(line[col]));
                }
                else if (fmt == QImage::Format_RGBA32FPx4 || fmt == QImage::Format_RGBA32FPx4_Premultiplied)
                {
                    const auto *line = reinterpret_cast<const float*>(m_currentImage.constScanLine(row));
                    int off = col * 4;
                    pixLabel->setText(QString("(%1,%2):(r=%3, g=%4, b=%5)")
                                          .arg(row).arg(col)
                                          .arg(line[off], 0, 'f', 4)
                                          .arg(line[off + 1], 0, 'f', 4)
                                          .arg(line[off + 2], 0, 'f', 4));
                }
                else if (fmt == QImage::Format_RGBA64 || fmt == QImage::Format_RGBX64
                         || fmt == QImage::Format_RGBA64_Premultiplied)
                {
                    const auto *line = reinterpret_cast<const QRgba64*>(m_currentImage.constScanLine(row));
                    QRgba64 c = line[col];
                    pixLabel->setText(QString("(%1,%2):(r=%3, g=%4, b=%5)")
                                          .arg(row).arg(col).arg(c.red()).arg(c.green()).arg(c.blue()));
                }
                else
                {
                    QColor c = m_currentImage.pixelColor(col, row);
                    bool isGray = m_currentImage.allGray();
                    if (isGray)
                        pixLabel->setText(QString("(%1,%2):(gray=%3)").arg(row).arg(col).arg(c.lightness()));
                    else
                        pixLabel->setText(QString("(%1,%2):(r=%3, g=%4, b=%5)")
                                              .arg(row).arg(col).arg(c.red()).arg(c.green()).arg(c.blue()));
                }
            });

    /// 实时显示图像信息
    connect(this, &MainWindow::sg_imageInfo_label, this, [this](QString imagePath, int height, int width)
            {
        ui->image_info->setText("name:"+imagePath+"\n"+QString("size:( %1 , %2 )").arg(height).arg(width));
        m_displayedImageInfo.imagePath=imagePath;
        m_displayedImageInfo.height=height;
        m_displayedImageInfo.width=width; });

    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QColor::fromRgb(224, 224, 224));
    ui->graphicsView->setScene(m_scene);

    m_visibilityMgr = new VisibilityManager(m_scene);

    connect(ui->graphicsView, &MyGraphicsView::sg_drawingFinished,
            this, &MainWindow::mergeOverlappingStrokes);
    connect(ui->graphicsView, &MyGraphicsView::sg_drawingFinished,
            this, &MainWindow::saveLabelingData);
    connect(ui->graphicsView, &MyGraphicsView::sg_itemErasedCompletely,
            this, [this](StrokeItem *item) {
        m_labelingItems.removeOne(item);
    });

    {
        QLibrary saigeLib("SaigeVision.dll");
        m_saigeAvailable = saigeLib.load();
        if (m_saigeAvailable)
            saigeLib.unload();
    }
    if (!m_saigeAvailable) {
        ui->actionsaige_infer->setEnabled(false);
        ui->infer_ckBox->setEnabled(false);
        qDebug() << "SaigeVision.dll not found - Saige features disabled";
    }
    setupShortcuts();

    m_canvasAdjustment = new CanvasAdjustmentWidget(this);
    connect(m_canvasAdjustment, &CanvasAdjustmentWidget::sg_brightnessContrastChanged,
            this, &MainWindow::sl_onBrightnessContrastChanged);
    connect(m_canvasAdjustment, &CanvasAdjustmentWidget::sg_annotationOpacityChanged,
            this, &MainWindow::sl_onAnnotationOpacityChanged);

    QToolButton *adjustBtn = new QToolButton();
    adjustBtn->setText(QString::fromUtf8("\u2699"));
    adjustBtn->setToolTip(QString::fromUtf8("显示调节"));
    adjustBtn->setFixedSize(24, 20);
    connect(adjustBtn, &QToolButton::clicked, this, &MainWindow::sl_showAdjustPopup);
    ui->statusbar->addPermanentWidget(adjustBtn);
}

void MainWindow::enableDrawActions(bool enabled)
{
    ui->actioncursor->setEnabled(enabled);
    ui->actionpen_brush->setEnabled(enabled);
    ui->actioncreate_rect->setEnabled(enabled);
    ui->actioneraser->setEnabled(enabled);
    ui->actionCreateRoi->setEnabled(enabled);
    ui->actionadd_image->setEnabled(enabled);
    ui->actionadd_imagefold->setEnabled(enabled);
    ui->actionsaige_infer->setEnabled(enabled && m_saigeAvailable);
    ui->actionultra_infer->setEnabled(enabled);
}

void MainWindow::onPenWidthChanged(int width)
{
    m_view->setPenWidth(width);
    m_view->updateCursor();
}

void MainWindow::onLabelColorChanged(int index)
{
    if (index < 0) return;
    QColor color = m_labelsComboBox->itemData(index).value<QColor>();
    ui->graphicsView->setPenColor(color);
    ui->graphicsView->updateCursor();
}

void MainWindow::onAddLabel()
{
    struct MorandiColor { QString name; QColor color; };
    static const MorandiColor morandiColors[] = {
        {"莫兰迪蓝", QColor(139, 167, 199)},
        {"莫兰迪绿", QColor(168, 187, 168)},
        {"莫兰迪粉", QColor(212, 165, 165)},
        {"莫兰迪紫", QColor(184, 169, 201)},
        {"莫兰迪黄", QColor(212, 197, 169)},
        {"莫兰迪灰", QColor(184, 184, 184)},
        {"莫兰迪棕", QColor(196, 168, 130)},
    };

    QDialog dlg(this);
    dlg.setWindowTitle("添加标签");
    dlg.setFixedSize(300, 220);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    layout->addWidget(new QLabel("标签名称:"));
    QLineEdit *nameEdit = new QLineEdit(&dlg);
    layout->addWidget(nameEdit);

    layout->addWidget(new QLabel("标签颜色:"));
    QHBoxLayout *colorLayout = new QHBoxLayout();

    QComboBox *colorCombo = new QComboBox(&dlg);
    colorCombo->addItem("随机");
    for (const auto &mc : morandiColors)
        colorCombo->addItem(mc.name, QVariant::fromValue(mc.color));

    QPushButton *pickBtn = new QPushButton("🎨", &dlg);
    pickBtn->setFixedWidth(30);
    pickBtn->setToolTip("选择自定义颜色");

    QLabel *preview = new QLabel(&dlg);
    preview->setFixedSize(24, 24);
    preview->setAutoFillBackground(true);

    colorLayout->addWidget(colorCombo);
    colorLayout->addWidget(pickBtn);
    colorLayout->addWidget(preview);
    layout->addLayout(colorLayout);

    QDialogButtonBox *btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    btnBox->button(QDialogButtonBox::Ok)->setText("确定");
    btnBox->button(QDialogButtonBox::Cancel)->setText("取消");
    layout->addWidget(btnBox);

    QColor selectedColor;
    auto updatePreview = [preview](const QColor &c) {
        QPalette pal = preview->palette();
        pal.setColor(QPalette::Window, c);
        preview->setPalette(pal);
    };

    connect(colorCombo, &QComboBox::currentIndexChanged, &dlg, [&](int idx) {
        if (idx == 0) {
            selectedColor = QColor::fromRgb(
                QRandomGenerator::global()->bounded(256),
                QRandomGenerator::global()->bounded(256),
                QRandomGenerator::global()->bounded(256));
        } else {
            selectedColor = colorCombo->itemData(idx).value<QColor>();
        }
        updatePreview(selectedColor);
    });
    colorCombo->setCurrentIndex(0);

    connect(pickBtn, &QPushButton::clicked, &dlg, [&]() {
        QColor c = QColorDialog::getColor(selectedColor, &dlg, "选择自定义颜色");
        if (c.isValid()) {
            selectedColor = c;
            updatePreview(c);
            colorCombo->setCurrentIndex(0);
        }
    });

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted && !nameEdit->text().isEmpty()) {
        if (!selectedColor.isValid())
            selectedColor = QColor::fromRgb(
                QRandomGenerator::global()->bounded(256),
                QRandomGenerator::global()->bounded(256),
                QRandomGenerator::global()->bounded(256));
        m_labelsComboBox->addItem(nameEdit->text(), QVariant::fromValue(selectedColor));
        m_labelsComboBox->setCurrentIndex(m_labelsComboBox->count() - 1);
        syncFilterComboBoxes();
        saveCategories();
    }
}

void MainWindow::onRemoveLabel()
{
    int idx = m_labelsComboBox->currentIndex();
    if (idx >= 0 && m_labelsComboBox->count() > 1) {
        m_labelsComboBox->removeItem(idx);
        syncFilterComboBoxes();
        saveCategories();
    }
}

void MainWindow::onCursorActionTriggered()
{
    m_view->switchCursorMode(CURSOR_MODE::origin);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->updateCursor();
    m_statusLabel->setText("光标模式 - 选择并查看图形信息");
}



// void MainWindow::onItemHovered(MyGraphicsItem *item)
// {
//     if (item) {
//         QPointF center = item->getCenter();
//         double area = item->getArea();
//         QString shapeType = item->getShapeType();

//         QString info = QString("%1 - 中心: (%2, %3) 面积: %4 像素")
//                            .arg(shapeType)
//                            .arg(center.x(), 0, 'f', 1)
//                            .arg(center.y(), 0, 'f', 1)
//                            .arg(area, 0, 'f', 1);
//         m_statusLabel->setText(info);
//     }
// }

// void MainWindow::onSceneSelectionChanged()
// {
//     QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
//     if (selectedItems.isEmpty()) {
//         m_statusLabel->setText("就绪");
//         return;
//     }

//     for (QGraphicsItem* item : selectedItems) {
//         if (MyGraphicsItem* myItem = dynamic_cast<MyGraphicsItem*>(item)) {
//             QPointF center = myItem->getCenter();
//             double area = myItem->getArea();
//             QString shapeType = myItem->getShapeType();

//             QString info = QString("%1 - 中心: (%2, %3) 面积: %4 像素")
//                                .arg(shapeType)
//                                .arg(center.x(), 0, 'f', 1)
//                                .arg(center.y(), 0, 'f', 1)
//                                .arg(area, 0, 'f', 1);
//             m_statusLabel->setText(info);
//             break;
//         }
//     }
// }

MainWindow::~MainWindow()
{

    delete ui;
}

void MainWindow::on_actioncreate_newProject_triggered()
{
    // 弹出文件对话框，让用户选择路径并输入项目名称
    QString projectName = QFileDialog::getSaveFileName(
        this,
        tr("创建新项目"),
        QDir::homePath(),
        tr("项目文件夹 (*)"));

    // 如果用户取消选择或未输入名称，则不执行操作
    if (projectName.isEmpty())
    {
        return;
    }

    // 创建主项目文件夹
    QDir projectDir(projectName);
    m_curProjectPath = projectName;

    if (!projectDir.exists())
    {
        if (!projectDir.mkpath("."))
        {
            QMessageBox::critical(this, tr("错误"), tr("无法创建项目文件夹！"));
            return;
        }
    }

    // 定义需要创建的子文件夹结构
    QStringList subFolders = {
        "non_train/image",
        "non_train/label",
        "train/image",
        "train/label"};

    // 逐个创建子文件夹
    foreach (const QString &folder, subFolders)
    {
        QString fullPath = projectName + "/" + folder;
        QDir dir(fullPath);
        if (!dir.exists() && !dir.mkpath("."))
        {
            QMessageBox::warning(
                this,
                tr("警告"),
                tr("无法创建子文件夹: %1").arg(folder));
        }
    }

    enableDrawActions(true);

    QMessageBox::information(
        this,
        tr("成功"),
        tr("项目已创建: %1").arg(projectName));
}

void MainWindow::on_actionopen_project_triggered()
{
    // 弹出文件夹选择对话框
    QString projectPath = QFileDialog::getExistingDirectory(
        this,
        tr("打开项目"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    // 如果用户取消选择，则不执行操作
    if (projectPath.isEmpty())
    {
        return;
    }

    // 定义需要检查的文件夹结构
    QStringList requiredFolders = {
        "non_train/image",
        "non_train/label",
        "train/image",
        "train/label"};

    // 检查文件夹结构是否完整
    bool structureValid = true;
    foreach (const QString &folder, requiredFolders)
    {
        QDir checkDir(projectPath + "/" + folder);
        if (!checkDir.exists())
        {
            structureValid = false;
            break;
        }
    }

    // 如果结构不完整，显示错误信息
    if (!structureValid)
    {
        QMessageBox::critical(
            this,
            tr("打开失败"),
            tr("所选文件夹不是有效的项目，缺少必要的子文件夹结构！"));
        return;
    }

    // 先清空列表控件中的现有内容
    ui->non_train_list->clear();
    ui->train_list->clear();
    m_curProjectPath = projectPath;
    enableDrawActions(true);
    loadRoi();
    loadCategories();
    syncFilterComboBoxes();
    m_allImageFiles.clear();

    // 定义要扫描的目录
    QString trainImagePath = projectPath + "/train/image";
    QString nonTrainImagePath = projectPath + "/non_train/image";

    // 获取train/image目录下的所有文件并添加到train_list
    QDir trainImageDir(trainImagePath);
    QStringList trainFiles = trainImageDir.entryList(QDir::Files);
    foreach (const QString &file, trainFiles)
    {
        // 过滤出图片文件 bmp jpg png tiff jpeg raw
        if ((!file.endsWith(".jpg", Qt::CaseInsensitive) &&
             !file.endsWith(".png", Qt::CaseInsensitive)) &&
            !file.endsWith(".tiff", Qt::CaseInsensitive) &&
            !file.endsWith(".jpeg", Qt::CaseInsensitive) &&
            !file.endsWith(".bmp", Qt::CaseInsensitive) &&
            !file.endsWith(".raw", Qt::CaseInsensitive) &&
            !file.endsWith(".tif", Qt::CaseInsensitive))

        {
            continue;
        }
        ui->train_list->addItem(file);
    }

    // 获取non_train/image目录下的所有文件并添加到non_train_list
    QDir nonTrainImageDir(nonTrainImagePath);
    QStringList nonTrainFiles = nonTrainImageDir.entryList(QDir::Files);
    foreach (const QString &file, nonTrainFiles)
    {

        // 过滤出图片文件 bmp jpg png tiff jpeg raw
        if (!file.endsWith(".jpg", Qt::CaseInsensitive) &&
            !file.endsWith(".png", Qt::CaseInsensitive) &&
            !file.endsWith(".tiff", Qt::CaseInsensitive) &&
            !file.endsWith(".jpeg", Qt::CaseInsensitive) &&
            !file.endsWith(".bmp", Qt::CaseInsensitive) &&
            !file.endsWith(".raw", Qt::CaseInsensitive) &&
            !file.endsWith(".tif", Qt::CaseInsensitive))

        {
            continue;
        }
        ui->non_train_list->addItem(file);
    }

    // 提示项目打开成功
    QMessageBox::information(
        this,
        tr("成功"),
        tr("项目已成功打开: %1").arg(projectPath));
}

void MainWindow::on_non_train_list_itemPressed(QListWidgetItem *item)
{
    m_activeList = ui->non_train_list;
}

void MainWindow::showNon_trainlistMenu(const QPoint &pos)
{

    // 将控件坐标转换为全局坐标
    QPoint globalPos = ui->non_train_list->mapToGlobal(pos);

    // 获取当前选中的项
    QList<QListWidgetItem *> selectedItems = ui->non_train_list->selectedItems();

    // 只有当有选中项时才显示菜单
    if (!selectedItems.isEmpty())
    {
        // 创建菜单
        QMenu contextMenu(tr("操作"), this);

        // 添加菜单项
        QAction *action1 = contextMenu.addAction(tr("加入训练集"));
        QAction *action2 = contextMenu.addAction(tr("彻底删除"));
        contextMenu.addSeparator(); // 添加分隔线
        QAction *action3 = contextMenu.addAction(tr("全选"));
        QAction *action4 = contextMenu.addAction(tr("取消选择"));

        // 连接菜单项的触发信号到对应的槽函数
        connect(action1, &QAction::triggered, this, &MainWindow::on_actionmoveinTrainlist_triggered);
        connect(action2, &QAction::triggered, this, &MainWindow::on_actiondelete_non_trainlistele_triggered);
        connect(action3, &QAction::triggered, ui->non_train_list, &QListWidget::selectAll);
        connect(action4, &QAction::triggered, ui->non_train_list, &QListWidget::clearSelection);

        // 显示菜单
        contextMenu.exec(globalPos);
    }
}

void MainWindow::showtrainlistMenu(const QPoint &pos)
{
    // 将控件坐标转换为全局坐标
    QPoint globalPos = ui->train_list->mapToGlobal(pos);

    // 获取当前选中的项
    QList<QListWidgetItem *> selectedItems = ui->train_list->selectedItems();

    // 只有当有选中项时才显示菜单
    if (!selectedItems.isEmpty())
    {
        // 创建菜单
        QMenu contextMenu(tr("操作"), this);

        // 添加菜单项
        QAction *action1 = contextMenu.addAction(tr("移出训练集"));
        QAction *action2 = contextMenu.addAction(tr("彻底删除"));
        contextMenu.addSeparator(); // 添加分隔线
        QAction *action3 = contextMenu.addAction(tr("全选"));
        QAction *action4 = contextMenu.addAction(tr("取消选择"));

        // 连接菜单项的触发信号到对应的槽函数
        connect(action1, &QAction::triggered, this, &MainWindow::on_actionremoveTrainlist_triggered);
        connect(action2, &QAction::triggered, this, &MainWindow::on_actiondelete_trainlistele_triggered);
        connect(action3, &QAction::triggered, ui->train_list, &QListWidget::selectAll);
        connect(action4, &QAction::triggered, ui->train_list, &QListWidget::clearSelection);

        // 显示菜单
        contextMenu.exec(globalPos);
    }
}

void MainWindow::on_actionmoveinTrainlist_triggered()
{

    // 1、获取选中的item信息获取图像名序列
    // 2、移动所有选中的m_curProjectPath/non_train/image图像文件到m_curProjectPath/train/image
    // 3、移动../label/同名的所有标注文件到../../train/label
    // 4、将选中项目non_train_list从删除，添加到train_list
    //  获取选中的项目
    QList<QListWidgetItem *> selectedItems = ui->non_train_list->selectedItems();
    if (selectedItems.isEmpty())
    {
        QMessageBox::information(this, "提示", "请先选择要移动的项目");
        return;
    }

    // 构建源目录和目标目录路径（使用QDir确保路径格式正确）
    QDir projectDir(m_curProjectPath);
    QString nonTrainImageDir = projectDir.filePath("non_train/image");
    QString nonTrainLabelDir = projectDir.filePath("non_train/label");
    QString trainImageDir = projectDir.filePath("train/image");
    QString trainLabelDir = projectDir.filePath("train/label");

    // 确保目标目录存在
    QDir().mkpath(trainImageDir);
    QDir().mkpath(trainLabelDir);

    // 记录移动成功的项目文本（用于重新创建项目）
    QStringList movedItemTexts;

    // 遍历所有选中的项目
    foreach (QListWidgetItem *item, selectedItems)
    {
        if (!item)
            continue;

        QString fileName = item->text();
        bool allSuccess = true;

        // 移动图像文件
        QString srcImage = QDir(nonTrainImageDir).filePath(fileName);
        QString destImage = QDir(trainImageDir).filePath(fileName);

        if (!SaigeUtils::safeMoveFile(srcImage, destImage, true))
        {
            QMessageBox::warning(this, "警告", QString("移动图像文件失败: %1").arg(fileName));
            allSuccess = false;
        }

        // 移动标注文件
        QFileInfo fileInfo(fileName);
        QString baseName = fileInfo.baseName();
        QStringList labelExts = {"xml", "json", "txt"};

        foreach (const QString &ext, labelExts)
        {
            QString srcLabel = QDir(nonTrainLabelDir).filePath(baseName + "." + ext);
            QString destLabel = QDir(trainLabelDir).filePath(baseName + "." + ext);

            if (QFile::exists(srcLabel) && !SaigeUtils::safeMoveFile(srcLabel, destLabel, true))
            {
                QMessageBox::warning(this, "警告", QString("移动标注文件失败: %1.%2").arg(baseName).arg(ext));
                allSuccess = false;
            }
        }

        // 如果所有文件都移动成功，记录项目文本
        if (allSuccess)
        {
            movedItemTexts.append(fileName);
        }
    }

    // 从非训练列表中删除并添加到训练列表
    foreach (const QString &text, movedItemTexts)
    {
        // 查找并从非训练列表中移除
        for (int i = 0; i < ui->non_train_list->count(); ++i)
        {
            QListWidgetItem *item = ui->non_train_list->item(i);
            if (item && item->text() == text)
            {
                delete ui->non_train_list->takeItem(i);
                break;
            }
        }

        // 向训练列表添加新项目（使用文本重新创建）
        QListWidgetItem *newItem = new QListWidgetItem(text);
        // 复制原项目的所有属性（如选中状态、背景色等）
        ui->train_list->addItem(newItem);
    }

    // 强制刷新列表
    applyFilter();

    if (!movedItemTexts.isEmpty())
    {
        QMessageBox::information(this, "成功",
                                 QString("已成功移动 %1 个项目到训练列表")
                                     .arg(movedItemTexts.size()));
    }
}

void MainWindow::on_actionremoveTrainlist_triggered()
{
    // 获取选中的项目（从train_list中选择）
    QList<QListWidgetItem *> selectedItems = ui->train_list->selectedItems();
    if (selectedItems.isEmpty())
    {
        QMessageBox::information(this, "提示", "请先选择要移动的项目");
        return;
    }

    // 构建源目录和目标目录路径（使用QDir确保路径格式正确）
    QDir projectDir(m_curProjectPath);
    QString trainImageDir = projectDir.filePath("train/image");
    QString trainLabelDir = projectDir.filePath("train/label");
    QString nonTrainImageDir = projectDir.filePath("non_train/image");
    QString nonTrainLabelDir = projectDir.filePath("non_train/label");

    // 确保目标目录存在
    QDir().mkpath(nonTrainImageDir);
    QDir().mkpath(nonTrainLabelDir);

    // 记录移动成功的项目文本（用于重新创建项目）
    QStringList movedItemTexts;

    // 遍历所有选中的项目
    foreach (QListWidgetItem *item, selectedItems)
    {
        if (!item)
            continue;

        QString fileName = item->text();
        bool allSuccess = true;

        // 移动图像文件（从train/image到non_train/image）
        QString srcImage = QDir(trainImageDir).filePath(fileName);
        QString destImage = QDir(nonTrainImageDir).filePath(fileName);

        if (!SaigeUtils::safeMoveFile(srcImage, destImage, true))
        {
            QMessageBox::warning(this, "警告", QString("移动图像文件失败: %1").arg(fileName));
            allSuccess = false;
        }

        // 移动标注文件（从train/label到non_train/label）
        QFileInfo fileInfo(fileName);
        QString baseName = fileInfo.baseName();
        QStringList labelExts = {"xml", "json", "txt"};

        foreach (const QString &ext, labelExts)
        {
            QString srcLabel = QDir(trainLabelDir).filePath(baseName + "." + ext);
            QString destLabel = QDir(nonTrainLabelDir).filePath(baseName + "." + ext);

            if (QFile::exists(srcLabel) && !SaigeUtils::safeMoveFile(srcLabel, destLabel, true))
            {
                QMessageBox::warning(this, "警告", QString("移动标注文件失败: %1.%2").arg(baseName).arg(ext));
                allSuccess = false;
            }
        }

        // 如果所有文件都移动成功，记录项目文本
        if (allSuccess)
        {
            movedItemTexts.append(fileName);
        }
    }

    // 从训练列表中删除并添加到非训练列表
    foreach (const QString &text, movedItemTexts)
    {
        // 查找并从训练列表中移除
        for (int i = 0; i < ui->train_list->count(); ++i)
        {
            QListWidgetItem *item = ui->train_list->item(i);
            if (item && item->text() == text)
            {
                delete ui->train_list->takeItem(i);
                break;
            }
        }

        // 向非训练列表添加新项目
        QListWidgetItem *newItem = new QListWidgetItem(text);
        ui->non_train_list->addItem(newItem);
    }

    // 强制刷新列表
    applyFilter();

    if (!movedItemTexts.isEmpty())
    {
        QMessageBox::information(this, "成功",
                                 QString("已成功移动 %1 个项目到非训练列表")
                                     .arg(movedItemTexts.size()));
    }
}

void MainWindow::on_actiondelete_non_trainlistele_triggered()
{
    // 1. 获取选中的项目
    QList<QListWidgetItem *> selectedItems = ui->non_train_list->selectedItems();
    if (selectedItems.isEmpty())
    {
        QMessageBox::information(this, "提示", "请先选择要删除的项目");
        return;
    }

    // 2. 构建文件目录路径
    QDir projectDir(m_curProjectPath);
    QString imageDir = projectDir.filePath("non_train/image");
    QString labelDir = projectDir.filePath("non_train/label");

    // 记录删除成功的项目
    QList<QListWidgetItem *> deletedItems;

    // 3. 遍历选中项，删除对应的文件
    foreach (QListWidgetItem *item, selectedItems)
    {
        if (!item)
            continue;

        QString fileName = item->text();
        bool allDeleted = true;

        // 3.1 删除图像文件
        QString imagePath = QDir(imageDir).filePath(fileName);
        if (QFile::exists(imagePath))
        {
            if (!SaigeUtils::safeDeleteFile(imagePath, false))
            { // 不进行二次确认
                QMessageBox::warning(this, "警告", QString("删除图像文件失败: %1").arg(fileName));
                allDeleted = false;
            }
        }

        // 3.2 删除标注文件
        QFileInfo fileInfo(fileName);
        QString baseName = fileInfo.baseName();
        QStringList labelExts = {"xml", "json", "txt"};

        foreach (const QString &ext, labelExts)
        {
            QString labelPath = QDir(labelDir).filePath(baseName + "." + ext);

            if (QFile::exists(labelPath))
            {
                if (!SaigeUtils::safeDeleteFile(labelPath, false))
                { // 不进行二次确认
                    QMessageBox::warning(this, "警告", QString("删除标注文件失败: %1.%2").arg(baseName).arg(ext));
                    allDeleted = false;
                }
            }
        }

        // 3.3 记录删除成功的项目
        if (allDeleted)
        {
            deletedItems.append(item);
        }
    }

    // 4. 从列表中移除删除成功的项目
    foreach (QListWidgetItem *item, deletedItems)
    {
        int row = ui->non_train_list->row(item);
        delete ui->non_train_list->takeItem(row);
    }

    // 5. 刷新列表
    applyFilter();

    // 6. 删除完成后提示结果
    if (!deletedItems.isEmpty())
    {
        QMessageBox::information(this, "成功",
                                 QString("已成功删除 %1 个项目及其文件")
                                     .arg(deletedItems.size()));
    }
}

void MainWindow::on_actiondelete_trainlistele_triggered()
{
        // 1. 获取选中的项目
    QList<QListWidgetItem *> selectedItems = ui->train_list->selectedItems();
    if (selectedItems.isEmpty())
    {
        QMessageBox::information(this, "提示", "请先选择要删除的项目");
        return;
    }

    // 2. 构建文件目录路径
    QDir projectDir(m_curProjectPath);
    QString imageDir = projectDir.filePath("train/image");
    QString labelDir = projectDir.filePath("train/label");

    // 记录删除成功的项目
    QList<QListWidgetItem *> deletedItems;

    // 3. 遍历选中项，删除对应的文件
    foreach (QListWidgetItem *item, selectedItems)
    {
        if (!item)
            continue;

        QString fileName = item->text();
        bool allDeleted = true;

        // 3.1 删除图像文件
        QString imagePath = QDir(imageDir).filePath(fileName);
        if (QFile::exists(imagePath))
        {
            if (!SaigeUtils::safeDeleteFile(imagePath, false))
            { // 不进行二次确认
                QMessageBox::warning(this, "警告", QString("删除图像文件失败: %1").arg(fileName));
                allDeleted = false;
            }
        }

        // 3.2 删除标注文件
        QFileInfo fileInfo(fileName);
        QString baseName = fileInfo.baseName();
        QStringList labelExts = {"xml", "json", "txt"};

        foreach (const QString &ext, labelExts)
        {
            QString labelPath = QDir(labelDir).filePath(baseName + "." + ext);

            if (QFile::exists(labelPath))
            {
                if (!SaigeUtils::safeDeleteFile(labelPath, false))
                { // 不进行二次确认
                    QMessageBox::warning(this, "警告", QString("删除标注文件失败: %1.%2").arg(baseName).arg(ext));
                    allDeleted = false;
                }
            }
        }

        // 3.3 记录删除成功的项目
        if (allDeleted)
        {
            deletedItems.append(item);
        }
    }

    // 4. 从列表中移除删除成功的项目
    foreach (QListWidgetItem *item, deletedItems)
    {
        int row = ui->train_list->row(item);
        delete ui->train_list->takeItem(row);
    }

    // 5. 刷新列表
    applyFilter();

    // 6. 删除完成后提示结果
    if (!deletedItems.isEmpty())
    {
        QMessageBox::information(this, "成功",
                                 QString("已成功删除 %1 个项目及其文件")
                                     .arg(deletedItems.size()));
    }

}

void MainWindow::on_non_train_list_itemDoubleClicked(QListWidgetItem *item)
{
    if (!item)
        return; // 防御性检查
    m_activeList = ui->non_train_list;

    // 1. 获取图像文件路径
    QString fileName = item->text();
    QDir projectDir(m_curProjectPath);
    QString imagePath = projectDir.filePath(QString("non_train/image/%1").arg(fileName));

    // 2. 检查文件是否存在
    if (!QFile::exists(imagePath))
    {
        QMessageBox::warning(this, "文件不存在",
                             QString("图像文件不存在：\n%1").arg(imagePath));
        return;
    }

    // 3. 加载图像
    QPixmap pixmap;
    if (!pixmap.load(imagePath))
    {
        QMessageBox::warning(this, "加载失败",
                             QString("无法加载图像文件：\n%1").arg(imagePath));
        return;
    }

    ui->graphicsView->clearStrokes();
    ui->graphicsView->clearRects();
    m_scene->clear();
    m_labelingItems.clear();
    m_displayedImageInfo.clear();
    m_roiOverlayItem = nullptr;
    // 添加图像到场景
    QGraphicsPixmapItem *pixmapItem = m_scene->addPixmap(pixmap);
    m_pixmapItem = pixmapItem;
    m_originalPixmap = pixmap;
    m_currentImage = pixmap.toImage();
    emit sg_imageInfo_label(imagePath, pixmap.height(), pixmap.width());

    // 5. 配置视图属性（可选）
    // 启用抗锯齿
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);

    // 适应窗口大小
    ui->graphicsView->fitInView(pixmapItem, Qt::KeepAspectRatio);

    // 允许缩放和平移（如果需要）
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag); // 拖动平移
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // 显示ROI
    displayRoi();

    // 进行推理

    m_curLabelInfo.clear();
    if (ui->infer_ckBox->isChecked())
    {
        if (!m_saigeAvailable) {
            QMessageBox::warning(this, "Saige 不可用",
                                 "未找到 SaigeVision.dll，推理功能无法使用");
            return;
        }
        // 1. 检查模型是否加载
        if (m_inferPara.modelPath == nullptr)
        {
            QMessageBox::warning(this, "警告", "请重新加载模型");
            return;
        }
        m_curLabelInfo.imgName = imagePath;
        QByteArray imagePathData = imagePath.toUtf8();
        const char *imagePathTmp = imagePathData.constData();

        int ret = SaigeInferFromImageFile(*m_saigeHdl.inferHdl, imagePathTmp, *m_saigeHdl.resHdl);

        if (ret != 0)
        {
            QMessageBox::warning(this, "警告", QString("推理失败：\n%1").arg(SaigeStrError(ret)));
            return;
        }
        qDebug() << "推理成功";
        // 判断是不是seg
        if (m_inferPara.task == static_cast<DLTASK>(kProjectTypeSEG))
        {
            bool ret = SaigeUtils::get_SaigeSegRes(*m_saigeHdl.resHdl, m_curLabelInfo, m_saigeHdl.modelInfo);
            if (!ret)
            {
                QMessageBox::warning(this, "警告", QString("获取分割结果失败：\n%1").arg(SaigeStrError(ret)));
                return;
            }
            // 清理当前scene中的item，而不清理图像item
            QList<QGraphicsItem *> items = m_scene->items();
            for (QGraphicsItem *item : items)
            {
                if (item != pixmapItem)
                {
                    m_scene->removeItem(item);
                    delete item;
                }
            }
            showSegInferLabel();
        }

        //判断是否是增强
        if(m_inferPara.task==static_cast<DLTASK>(kProjectTypeIEN)){
            auto resInfer =SaigeUtils::get_SaigeIENRes(*m_saigeHdl.resHdl,pixmap.width(),pixmap.height(),QImage::Format_Grayscale8);
            if(resInfer.index()){
                QMessageBox::warning(this, "警告", QString("获取增强结果失败：\n%1").arg(std::get<QString>(resInfer)));
            }
            else
            {
                //清空当前图像
                m_scene->clear();
                m_labelingItems.clear();
                m_roiOverlayItem = nullptr;
                QPixmap ienPix = QPixmap::fromImage( (std::get<QImage>(resInfer)));
                m_scene->addPixmap(ienPix);
            }
    }
    if (ui->drawEffect_ckBox->isChecked())
        showLabelingPatterns();
    return;
}
    if (ui->drawEffect_ckBox->isChecked())
        showLabelingPatterns();

    if (ui->inferRes_ckBox->checkState() == Qt::Checked)
    {
        // 查找文件路径 如果没有就不显示
        // 去掉后缀名，并添加.json
        QString jsonFile = imagePath.left(imagePath.lastIndexOf(".")) + "_infer.json";
        if (!QFile::exists(jsonFile))
        {
            return;
        }
        // jsoncpp读取json文件到m_curLabelInfo
        Json::Reader reader;
        Json::Value root;
        QFile file(jsonFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, "警告", QString("无法打开JSON文件：\n%1").arg(jsonFile));
            return;
        }
        QByteArray jsonData = file.readAll();
        file.close();
        if (!reader.parse(jsonData.toStdString(), root))
        {
            QMessageBox::warning(this, "警告", QString("解析JSON文件失败：\n%1").arg(jsonFile));
            return;
        }
        auto res = getInferLabelInfoFromJson(root);
        if (res.index() == 1)
        {
            QString errMsg = std::get<QString>(res);
            QMessageBox::warning(this, "警告", QString("解析JSON文件失败：\n%1").arg(errMsg));
        }
        else
        {
            m_curLabelInfo = std::get<LabelInfo>(res);
            showSegInferLabel();
        }
    }
}

void MainWindow::on_train_list_itemDoubleClicked(QListWidgetItem *item)
{
    if (!item)
        return; // 防御性检查
    m_activeList = ui->train_list;

    // 1. 获取训练集图像文件路径
    QString fileName = item->text();
    QDir projectDir(m_curProjectPath);
    QString imagePath = projectDir.filePath(QString("train/image/%1").arg(fileName));

    // 2. 检查文件是否存在
    if (!QFile::exists(imagePath))
    {
        QMessageBox::warning(this, "文件不存在",
                             QString("图像文件不存在：\n%1").arg(imagePath));
        return;
    }

    // 3. 加载图像
    QPixmap pixmap;
    if (!pixmap.load(imagePath))
    {
        QMessageBox::warning(this, "加载失败",
                             QString("无法加载图像文件：\n%1").arg(imagePath));
        return;
    }

    // 4. 在graphicsView中显示图像
    // 获取或创建场景
    QGraphicsScene *scene = ui->graphicsView->scene();
    if (!scene)
    {
        scene = new QGraphicsScene(this);
        ui->graphicsView->setScene(scene);
    }
    else
    {
        // 清除现有内容
        ui->graphicsView->clearStrokes();
        ui->graphicsView->clearRects();
        scene->clear();
        m_labelingItems.clear();
        m_roiOverlayItem = nullptr;
    }

    // 添加图像到场景
    QGraphicsPixmapItem *pixmapItem = scene->addPixmap(pixmap);
    m_pixmapItem = pixmapItem;
    m_originalPixmap = pixmap;
    m_currentImage = pixmap.toImage();
    emit sg_imageInfo_label(imagePath, pixmap.height(), pixmap.width());

    // 5. 配置视图属性
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);
    ui->graphicsView->fitInView(pixmapItem, Qt::KeepAspectRatio);
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag); // 支持拖动
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // 显示ROI
    displayRoi();

    // 进行推理
    m_curLabelInfo.clear();
    if (ui->infer_ckBox->isChecked())
    {
        if (!m_saigeAvailable) {
            QMessageBox::warning(this, "Saige 不可用",
                                 "未找到 SaigeVision.dll，推理功能无法使用");
            return;
        }
        // 1. 检查模型是否加载
        if (m_inferPara.modelPath == nullptr)
        {
            QMessageBox::warning(this, "警告", "请重新加载模型");
            return;
        }
        m_curLabelInfo.imgName=imagePath;
        QByteArray imagePathData = imagePath.toUtf8();
        const char *imagePathTmp = imagePathData.constData();

        int ret = SaigeInferFromImageFile(*m_saigeHdl.inferHdl, imagePathTmp, *m_saigeHdl.resHdl);

        if (ret != 0)
        {
            QMessageBox::warning(this, "警告", QString("推理失败：\n%1").arg(SaigeStrError(ret)));
            return;
        }
        qDebug() << "推理成功";
        // 判断是不是seg
        if (m_inferPara.task == static_cast<DLTASK>(kProjectTypeSEG))
        {
            bool ret = SaigeUtils::get_SaigeSegRes(*m_saigeHdl.resHdl, m_curLabelInfo, m_saigeHdl.modelInfo);
            if (!ret)
            {
                QMessageBox::warning(this, "警告", QString("获取分割结果失败：\n%1").arg(SaigeStrError(ret)));
                return;
            }
            // 清理当前scene中的item，而不清理图像item
            QList<QGraphicsItem *> items = m_scene->items();
            for (QGraphicsItem *item : items)
            {
                if (item != pixmapItem)
                {
                    m_scene->removeItem(item);
                    delete item;
                }
            }
            showSegInferLabel();
        }

        //判断是否是增强
        if(m_inferPara.task==static_cast<DLTASK>(kProjectTypeIEN)){
            auto resInfer =SaigeUtils::get_SaigeIENRes(*m_saigeHdl.resHdl,pixmap.width(),pixmap.height(),QImage::Format_Grayscale8);
            if(resInfer.index()){
                QMessageBox::warning(this, "警告", QString("获取增强结果失败：\n%1").arg(std::get<QString>(resInfer)));
            }
            else
            {
                //清空当前图像
                scene->clear();
                m_labelingItems.clear();
                m_roiOverlayItem = nullptr;
                QPixmap ienPix = QPixmap::fromImage( (std::get<QImage>(resInfer)));
                scene->addPixmap(ienPix);
            }
        }
    }
    if (ui->drawEffect_ckBox->isChecked())
        showLabelingPatterns();
}

void MainWindow::on_actioneraser_triggered()
{
    ui->graphicsView->switchCursorMode(CURSOR_MODE::eraser);
}

void MainWindow::on_actioncursor_triggered()
{
    ui->graphicsView->switchCursorMode(CURSOR_MODE::origin);
}

void MainWindow::on_actionpen_brush_triggered()
{
    ui->graphicsView->switchCursorMode(CURSOR_MODE::pen_brush);
}

void MainWindow::on_actioncreate_rect_triggered()
{
    ui->graphicsView->switchCursorMode(CURSOR_MODE::create_rect);
}

void MainWindow::on_actionsaige_infer_triggered()
{
    if (!m_saigeAvailable) {
        QMessageBox::warning(this, "Saige 不可用",
                             "未找到 SaigeVision.dll，推理功能无法使用");
        return;
    }
    SaigeInfer dlg;
    connect(&dlg, &SaigeInfer::sg_saigeInferPara, this, &MainWindow::set_saigeInferPara);
    dlg.exec();
}

void MainWindow::set_saigeInferPara(SaigeInferPara param)
{
    if (!m_saigeAvailable) {
        QMessageBox::warning(this, "Saige 不可用",
                             "未找到 SaigeVision.dll，无法初始化模型");
        return;
    }
    //启动qt不定时进度条
    QProgressDialog progressDialog("模型初始化... ...", "", 0, 0, this);
    //不显示取消
    progressDialog.setCancelButton(nullptr);
    //不显示关闭按钮
    progressDialog.setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint & ~Qt::WindowMaximizeButtonHint & ~Qt::WindowMinimizeButtonHint);
    progressDialog.setWindowModality(Qt::ApplicationModal);


    QString initError="";
    //async 异步初始化模型
    std::thread initThread([this, param, &initError,&progressDialog]() {
        auto resInit = saigeMoelInit(param);
        if (resInit.index() != 0)
        {
            initError=std::get<QString>(resInit);
        }
        else
        {
            initError="";
        }
        progressDialog.close();
    });
    progressDialog.exec();
    initThread.join();
        // 关闭进度条

    if(!initError.isEmpty()){
        QMessageBox::warning(this, "模型初始化失败",
                             QString("模型初始化失败：\n%1").arg(initError));
        return;
    }
    else{
        QMessageBox::information(this, "模型初始化成功",
                                 QString("模型初始化成功：\n%1").arg(param.modelPath));
    }








///////////////////////////////////////////////////////////////////////////////////////////////
    // 如果勾选了批量推理，则直接进入匹配推理
    if (param.isInfer_nontrainList || param.isInfer_trainList)
    {
        QVector<QString> inferList;
        QVector<QString> listTmp;
        QString perfix;
        if (param.isInfer_nontrainList)
        {
            perfix = this->m_curProjectPath + "/non_train/image/";
            listTmp = getListWidgetItemsQStrList(ui->non_train_list, perfix);
            inferList.append(std::move(listTmp));
        }
        if (param.isInfer_trainList)
        {
            perfix = this->m_curProjectPath + "/train/image/";
            listTmp = getListWidgetItemsQStrList(ui->train_list, perfix);
            inferList.append(std::move(listTmp));
        }

        // for(auto &inferPath:inferList){
        //     qDebug()<<inferPath;
        // }

        ///计算数量开启进度条
        QProgressDialog progressDialog("批量推理进度", "", 0, inferList.size(), this);
        progressDialog.setCancelButton(nullptr);
        progressDialog.setWindowModality(Qt::ApplicationModal);
        progressDialog.show();
        /// 开始批量推理 写出json
        for(auto path:inferList)
        {
        auto resInfer = SaigeUtils::SaveJson_fromSaigeInfer(m_saigeHdl.inferHdl, m_saigeHdl.modelInfo, m_inferPara.task, path);
        progressDialog.setValue(progressDialog.value() + 1);
        if (resInfer.index() != 0)
        {
            QMessageBox::warning(this, "批量推理失败",
                                 QString("批量推理失败：\n%1").arg(QString::fromStdString(std::get<std::string>(resInfer))));
        }
        }

    }
}

#include <QTimer> // 需要添加这个头文件

void MainWindow::on_actionultra_infer_triggered()
{
    Ultrainfer dlg;
    if (!m_curProjectPath.isEmpty()) {
        dlg.setProjectName(QFileInfo(m_curProjectPath).fileName());
        QString jsonPath = m_curProjectPath + "/model_param.json";
        dlg.loadFromJson(jsonPath);
    }
    connect(&dlg, &Ultrainfer::sg_train, this, &MainWindow::set_ultrainferTrainPara);
    connect(&dlg, &Ultrainfer::sg_inferConfirm, this, &MainWindow::set_ultrainferInferPara);
    connect(&dlg, &Ultrainfer::sg_refreshModels, this, [this](QStringList &models) {
        if (m_curProjectPath.isEmpty())
            return;

        QDir projectDir(m_curProjectPath);
        QStringList searchDirs = {"ultralytics/runs", "runs"};
        for (const auto &dir : searchDirs) {
            QDir inferDir = projectDir;
            if (inferDir.cd(dir) && inferDir.cd("detect")) {
                for (const auto &entry : inferDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                    QString weightsPath = entry.filePath() + "/weights/best.pt";
                    if (QFile::exists(weightsPath)) {
                        models.append(weightsPath);
                    }
                }
            }
        }
    });
    dlg.exec();

    if (!m_curProjectPath.isEmpty()) {
        QString jsonPath = m_curProjectPath + "/model_param.json";
        dlg.saveToJson(jsonPath);
    }
}

void MainWindow::set_ultrainferTrainPara(UltrainferTrainPara param)
{
    QMessageBox::information(this, "Ultralytics 训练参数",
                             QString("模型路径: %1\n任务: %2\n设备: %3\n优化器: %4\n"
                                     "余弦退火: %5\nEpochs: %6\nBatch: %7\nImgsz: %8\n"
                                     "AMP: %9\nResume: %10")
                                 .arg(param.modelPath)
                                 .arg(param.task)
                                 .arg(param.device)
                                 .arg(param.optimizer)
                                 .arg(param.cos_lr ? "是" : "否")
                                 .arg(param.epochs)
                                 .arg(param.batch)
                                 .arg(param.imgsz)
                                 .arg(param.amp ? "是" : "否")
                                 .arg(param.resume ? "是" : "否"));
}

void MainWindow::set_ultrainferInferPara(UltrainferInferPara param)
{
    QMessageBox::information(this, "Ultralytics 推理参数",
                             QString("推理模型: %1\n置信度阈值: %2 (%3)\n"
                                     "面积阈值: %4 (%5 - %6)")
                                 .arg(param.modelPath)
                                 .arg(param.useConf ? "启用" : "禁用")
                                 .arg(param.confThreshold)
                                 .arg(param.useArea ? "启用" : "禁用")
                                 .arg(param.areaMin)
                                 .arg(param.areaMax));
}

void MainWindow::on_ultra_batchInfer(UltrainferBatchInferPara param)
{
    QMessageBox::information(this, "Ultralytics 批量推理",
                             QString("非训练集: %1\n训练集: %2")
                                 .arg(param.useNonTrain ? "是" : "否")
                                 .arg(param.useTrain ? "是" : "否"));
}

void MainWindow::on_infer_ckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked)
    {
        if (!m_saigeAvailable) {
            QMessageBox::warning(this, "Saige 不可用",
                                 "未找到 SaigeVision.dll，推理功能无法使用");
            QTimer::singleShot(0, this, [this]()
                               { ui->infer_ckBox->setChecked(false); });
            return;
        }
        if (m_saigeHdl.inferHdl == 0)
        {
            QMessageBox::warning(this, "推理句柄不存在",
                                 "推理句柄不存在，无法开启推理\n请先加载模型创建推理句柄");

            // 使用QTimer::singleShot延迟设置状态，让当前信号处理完成
            QTimer::singleShot(0, this, [this]()
                               { ui->infer_ckBox->setChecked(false); });
            return;
        }
        // 此处可以添加启动推理的代码
    }
    else
    {
        // 处理取消选中状态的逻辑，例如停止推理、释放临时资源等
    }
}

void MainWindow::on_drawEffect_ckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    bool visible = (arg1 == Qt::Checked);
    m_visibilityMgr->setPaintedItemVisible(visible);
    ui->graphicsView->setDrawEffectEnabled(visible);

    if (visible)
        showLabelingPatterns();
}

void MainWindow::on_inferRes_ckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    m_visibilityMgr->setInferPatternVisible(arg1 == Qt::Checked);
}

void MainWindow::showSegInferLabel()
{
    for (const auto &labelele : m_curLabelInfo.label)
    {
        // 根据标签名称从m_labelsComboBox中查找对应的颜色
        QColor color;
        bool colorFound = false;

        // 安全检查：确保labelele.labelName不为空
        if (labelele.labelName && strlen(labelele.labelName) > 0)
        {
            // 转换为QString以便比较
            QString labelName = QString::fromUtf8(labelele.labelName);

            // 遍历m_labelsComboBox的所有项查找匹配的标签名称
            for (int i = 0; i < m_labelsComboBox->count(); ++i)
            {
                if (m_labelsComboBox->itemText(i) == labelName)
                {
                    color = m_labelsComboBox->itemData(i).value<QColor>();
                    colorFound = true;
                    break;
                }
            }
        }

        // 如果没有找到匹配的颜色，使用默认颜色（灰色）
        if (!colorFound)
        {
            color = Qt::gray;
        }

        // 创建InferPattern对象
        InferPattern *pattern = new InferPattern(labelele, color, nullptr);

        // 添加到场景
        m_scene->addItem(pattern);
    }

    m_visibilityMgr->setInferPatternVisible(ui->inferRes_ckBox->isChecked());

    // 确保视图适配内容
    ui->graphicsView->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void MainWindow::showLabelingPatterns()
{
    if (m_displayedImageInfo.imagePath.isEmpty()) return;
    if (!m_labelingItems.isEmpty()) return;

    QFileInfo imageInfo(m_displayedImageInfo.imagePath);
    QString jsonPath = imageInfo.path() + QDir::separator()
                       + imageInfo.baseName() + "_labeling.json";

    if (!QFile::exists(jsonPath)) return;

    Json::Reader reader;
    Json::Value root;
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QByteArray jsonData = file.readAll();
    file.close();

    if (!reader.parse(jsonData.toStdString(), root)) return;
    if (!root.isArray()) return;

    for (const auto &obj : root)
    {
        if (!obj.isMember("classname") || !obj["classname"].isString()) continue;
        if (!obj.isMember("points") || !obj["points"].isArray()) continue;

        std::string className = obj["classname"].asString();
        QColor color = Qt::gray;
        int classId = -1;
        for (int i = 0; i < m_labelsComboBox->count(); ++i)
        {
            if (m_labelsComboBox->itemText(i) == QString::fromStdString(className))
            {
                color = m_labelsComboBox->itemData(i).value<QColor>();
                classId = i;
                break;
            }
        }
        if (classId < 0) continue;

        QVector<QPointF> pts;
        for (const auto &jsonPt : obj["points"])
        {
            if (jsonPt.isArray() && jsonPt.size() >= 2)
                pts.append(QPointF(jsonPt[0].asDouble(), jsonPt[1].asDouble()));
        }
        if (pts.size() < 3) continue;

        if (pts.size() == 4) {
            RectItem *item = new RectItem(color, 3, classId);
            item->setRectFromPoints(pts[0], pts[2]);
            m_scene->addItem(item);
            m_labelingItems.append(item);
        } else {
            StrokeItem *item = new StrokeItem(color, 3, classId);
            item->setPolygonFromPoints(pts);
            m_scene->addItem(item);
            m_labelingItems.append(item);
        }
    }
}

void MainWindow::on_actioninferPattern_xml_triggered()
{
    // 判断m_curLabelInfo是否为空
    if (m_curLabelInfo.label.empty())
    {
        QMessageBox::warning(this, "警告",
                             "当前标签为空，无法导出XML文件");
        return;
    }
    // 判断image_info是不是有图像信息
    if (m_displayedImageInfo.imagePath.isEmpty())
    {
        QMessageBox::warning(this, "警告",
                             "当前图像信息为空，无法导出XML文件");
        return;
    }
    // xml文件和图像文件在同一目录并添加_SEGinfer后缀
    QString xmlPath = m_displayedImageInfo.imagePath.left(m_displayedImageInfo.imagePath.lastIndexOf(".")) + "_SEGinfer.xml";
    // 导出XML文件
    if (!writeSegInferXML(xmlPath.toStdString().c_str(), m_curLabelInfo))
    {
        QMessageBox::warning(this, "警告",
                             "导出XML文件失败");
        return;
    }
    QMessageBox::information(this, "信息",
                             QString("XML文件已导出到：\n%1").arg(xmlPath));
}

bool MainWindow::writeSegInferXML(const char *xmlPath, LabelInfo &labelInfo)
{
    QFile file(xmlPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "警告",
                             QString("无法打开文件进行写入：\n%1").arg(xmlPath));
        return false;
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("Segmentation");

    // 写入每个标签
    for (const auto &labelele : labelInfo.label)
    {
        writer.writeStartElement("Object");
        writer.writeAttribute("label", labelele.labelName);

        // 写入轮廓点
        writer.writeStartElement("Contour");
        for (const auto &point : labelele.pts)
        {
            writer.writeEmptyElement("Point");
            writer.writeAttribute("x", QString::number(point.x()));
            writer.writeAttribute("y", QString::number(point.y()));
        }
        writer.writeEndElement(); // Contour
        writer.writeEndElement(); // Object
    }

    writer.writeEndElement(); // Segmentation
    writer.writeEndDocument();

    file.close();
    return true;
}

bool MainWindow::writeSegInferXMLsb(const char *xmlPath, LabelInfo &labelInfo)
{
    QFile file(xmlPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "警告",
                             QString("无法打开文件进行写入：\n%1").arg(xmlPath));
        return false;
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);         // 自动格式化使XML更易读
    writer.writeStartDocument("1.0", true); // 声明XML版本和UTF-8编码

    // 根元素DetecData及命名空间
    writer.writeStartElement("DetecData");
    writer.writeAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
    writer.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");

    // 外层列表容器
    writer.writeStartElement("LstDeteData");

    // 遍历所有标注对象
    for (const auto &labelele : labelInfo.label)
    {
        writer.writeStartElement("DetectEntity"); // 每个对象的根元素

        // 写入名称（从属性改为子元素）
        writer.writeTextElement("name", QString::fromUtf8(labelele.labelName));

        // 写入y坐标列表（lstRowPos）
        writer.writeStartElement("lstRowPos");
        for (const auto &point : labelele.pts)
        {
            writer.writeTextElement("double", QString::number(point.y()));
        }
        writer.writeEndElement(); // 结束lstRowPos

        // 写入x坐标列表（lstColPos）
        writer.writeStartElement("lstColPos");
        for (const auto &point : labelele.pts)
        {
            writer.writeTextElement("double", QString::number(point.x()));
        }
        writer.writeEndElement(); // 结束lstColPos

        writer.writeEndElement(); // 结束DetectEntity
    }

    writer.writeEndElement();  // 结束LstDeteData
    writer.writeEndElement();  // 结束DetecData
    writer.writeEndDocument(); // 结束文档

    file.close();
    return true;
}

void MainWindow::on_actioninferPattern_hdict_triggered()
{
    // 判断m_curLabelInfo是否为空
    if (m_curLabelInfo.label.empty())
    {
        QMessageBox::warning(this, "警告",
                             "当前标签为空，无法导出Hdict文件");
        return;
    }
    // 判断image_info是不是有图像信息
    if (m_displayedImageInfo.imagePath.isEmpty())
    {
        QMessageBox::warning(this, "警告",
                             "当前图像信息为空，无法导出Hdict文件");
        return;
    }
    // xml文件和图像文件在同一目录并添加_SEGinfer后缀
    QString hdictPath = m_displayedImageInfo.imagePath.left(m_displayedImageInfo.imagePath.lastIndexOf(".")) + "_SEGinfer.hdict";
    // 导出Hdict文件
    if (!writeSegInferHdict(hdictPath.toStdString().c_str(), m_curLabelInfo))
    {
        QMessageBox::warning(this, "警告",
                             "导出Hdict文件失败");
        return;
    }
    QMessageBox::information(this, "信息",
                             QString("Hdict文件已导出到：\n%1").arg(hdictPath));
}

bool MainWindow::writeSegInferHdict(const char *HdictPath, LabelInfo &labelInfo)
{

    try
    {
        HalconCpp::HTuple dict{};
        HalconCpp::CreateDict(&dict);
        HalconCpp::HTuple x_arr;
        HalconCpp::HTuple y_arr;
        HalconCpp::HObject regionEle{};
        HalconCpp::HTuple dictEle;
        QPointF pt;
        int targetNum = labelInfo.label.size();
        std::vector<HalconCpp::HObject> regionlist(targetNum);
        Labelele labelele{nullptr, {{0.0, 0.0}}};

        for (int i = 0; i < targetNum; i++)
        {
            labelele = labelInfo.label.at(i);
            x_arr.Clear();
            y_arr.Clear();
            regionEle.Clear();
            dictEle.Clear();
            HalconCpp::HTuple labelName = labelele.labelName;
            int cnt = labelele.pts.size();
            int max_x = 0;
            int max_y = 0;

            for (int j = 0; j < cnt; j++)
            {
                pt = labelele.pts[j];
                x_arr[j] = pt.x();
                y_arr[j] = pt.y();
                pt.x() > max_x ? max_x = pt.x() : 0;
                pt.y() > max_y ? max_y = pt.y() : 0;
            }
            x_arr[cnt] = labelele.pts[0].x();
            y_arr[cnt] = labelele.pts[0].y();
            int width = nextMultipleOf32(max_x);
            int height = nextMultipleOf32(max_y);

            HalconCpp::SetSystem("width", width);
            HalconCpp::SetSystem("height", height);

            HalconCpp::GenRegionPolygon(&regionlist[i], y_arr, x_arr);

            HalconCpp::CreateDict(&dictEle);
            HalconCpp::SetDictTuple(dictEle, "label_name", labelName);
            HalconCpp::SetDictObject(regionlist[i], dictEle, "region");

            HalconCpp::SetDictTuple(dict, i, dictEle);
        }
        HalconCpp::WriteDict(dict, HdictPath, HalconCpp::HTuple(), HalconCpp::HTuple());

        return true;
    }
    catch (HalconCpp::HException &ex)
    {
        qDebug() << ex.ErrorMessage().Text();
        return false;
    }
}

int MainWindow::nextMultipleOf32(int n)
{
    // 如果n已经是32的倍数，返回n+32确保结果更大
    // 否则计算下一个32的倍数
    if (n % 32 == 0)
    {
        return n + 32;
    }
    else
    {
        return (n / 32 + 1) * 32;
    }
}

void MainWindow::on_actionExportYoloHBB_triggered()
{
    if (m_displayedImageInfo.imagePath.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No image loaded");
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(this, "Select Export Directory");
    if (dir.isEmpty()) return;

    QList<AnnoRect> rects;
    QList<AnnoPolygon> polygons;
    LabelExporter::collectFromScene(m_scene, m_labelsComboBox, rects, polygons);

    QStringList classes;
    for (int i = 0; i < m_labelsComboBox->count(); ++i)
        classes.append(m_labelsComboBox->itemText(i));

    bool ok = LabelExporter::exportYoloHBB(dir, m_displayedImageInfo.imagePath,
                                           m_displayedImageInfo.width, m_displayedImageInfo.height,
                                           rects, classes);
    QMessageBox::information(this, "Result", ok ? "Export succeeded" : "Export failed");
}

void MainWindow::on_actionExportYoloSeg_triggered()
{
    if (m_displayedImageInfo.imagePath.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No image loaded");
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(this, "Select Export Directory");
    if (dir.isEmpty()) return;

    QList<AnnoRect> rects;
    QList<AnnoPolygon> polygons;
    LabelExporter::collectFromScene(m_scene, m_labelsComboBox, rects, polygons);

    QStringList classes;
    for (int i = 0; i < m_labelsComboBox->count(); ++i)
        classes.append(m_labelsComboBox->itemText(i));

    bool ok = LabelExporter::exportYoloSeg(dir, m_displayedImageInfo.imagePath,
                                           m_displayedImageInfo.width, m_displayedImageInfo.height,
                                           polygons, classes);
    QMessageBox::information(this, "Result", ok ? "Export succeeded" : "Export failed");
}

void MainWindow::on_actionExportVocDet_triggered()
{
    if (m_displayedImageInfo.imagePath.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No image loaded");
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(this, "Select Export Directory");
    if (dir.isEmpty()) return;

    QList<AnnoRect> rects;
    QList<AnnoPolygon> polygons;
    LabelExporter::collectFromScene(m_scene, m_labelsComboBox, rects, polygons);

    bool ok = LabelExporter::exportVocDetection(dir, m_displayedImageInfo.imagePath,
                                                m_displayedImageInfo.width, m_displayedImageInfo.height,
                                                rects);
    QMessageBox::information(this, "Result", ok ? "Export succeeded" : "Export failed");
}

void MainWindow::on_actionExportVocSeg_triggered()
{
    if (m_displayedImageInfo.imagePath.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No image loaded");
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(this, "Select Export Directory");
    if (dir.isEmpty()) return;

    QList<AnnoRect> rects;
    QList<AnnoPolygon> polygons;
    LabelExporter::collectFromScene(m_scene, m_labelsComboBox, rects, polygons);

    bool ok = LabelExporter::exportVocSegmentation(dir, m_displayedImageInfo.imagePath,
                                                   m_displayedImageInfo.width, m_displayedImageInfo.height,
                                                   polygons);
    QMessageBox::information(this, "Result", ok ? "Export succeeded" : "Export failed");
}

void MainWindow::on_actionExportCocoDet_triggered()
{
    QString jsonPath = QFileDialog::getSaveFileName(this, "Save COCO Detection JSON",
                                                    m_curProjectPath, "JSON (*.json)");
    if (jsonPath.isEmpty()) return;

    QStringList classes;
    for (int i = 0; i < m_labelsComboBox->count(); ++i)
        classes.append(m_labelsComboBox->itemText(i));

    QStringList imgNames;
    QList<QPair<int,int>> imgSizes;
    QList<QList<AnnoRect>> allRects;

    QList<AnnoRect> rects;
    QList<AnnoPolygon> polygons;
    LabelExporter::collectFromScene(m_scene, m_labelsComboBox, rects, polygons);

    imgNames.append(m_displayedImageInfo.imagePath);
    imgSizes.append({m_displayedImageInfo.width, m_displayedImageInfo.height});
    allRects.append(rects);

    bool ok = LabelExporter::exportCocoDetection(jsonPath, imgNames, imgSizes, allRects, classes);
    QMessageBox::information(this, "Result", ok ? "Export succeeded" : "Export failed");
}

void MainWindow::on_actionExportCocoSeg_triggered()
{
    QString jsonPath = QFileDialog::getSaveFileName(this, "Save COCO Segmentation JSON",
                                                    m_curProjectPath, "JSON (*.json)");
    if (jsonPath.isEmpty()) return;

    QStringList classes;
    for (int i = 0; i < m_labelsComboBox->count(); ++i)
        classes.append(m_labelsComboBox->itemText(i));

    QStringList imgNames;
    QList<QPair<int,int>> imgSizes;
    QList<QList<AnnoPolygon>> allPolygons;

    QList<AnnoRect> rects;
    QList<AnnoPolygon> polygons;
    LabelExporter::collectFromScene(m_scene, m_labelsComboBox, rects, polygons);

    imgNames.append(m_displayedImageInfo.imagePath);
    imgSizes.append({m_displayedImageInfo.width, m_displayedImageInfo.height});
    allPolygons.append(polygons);

    bool ok = LabelExporter::exportCocoSegmentation(jsonPath, imgNames, imgSizes, allPolygons, classes);
    QMessageBox::information(this, "Result", ok ? "Export succeeded" : "Export failed");
}

void MainWindow::add_non_train_list(QVector<QString> appendFiles)
{
    for (const QString &file : appendFiles)
    {
        QString fileName = QFileInfo(file).fileName();
        m_allImageFiles.append("non_train:" + fileName);
    }
    applyFilter();
}

void MainWindow::on_actionadd_image_triggered()
{
    //判断当前项目是否为空
    if (m_curProjectPath.isEmpty())
    {
        QMessageBox::warning(this, "警告",
                             "当前项目为空，无法添加图像");
        return;
    }

    //打开文件选择框，选择图像文件
    QStringList imageFiles = QFileDialog::getOpenFileNames(this, "选择图像文件", "", "图像文件 (*.jpg *.png *.bmp *.jpeg *.tif *.tiff)");
    if (imageFiles.isEmpty())
    {
        return;
    }
    //将文件复制到non_train/image目录
    QString non_train_list_path =m_curProjectPath+"/non_train/image/";

    for (const QString &file : imageFiles)
    {
        QString newfile =non_train_list_path+QFileInfo(file).fileName();

        QFile::copy(file, newfile);
    }
    //将选择的图像文件添加到non_train_list
    add_non_train_list(imageFiles);

}


void MainWindow::on_actionadd_imagefold_triggered()
{
    //判断当前项目是否为空
    if (m_curProjectPath.isEmpty())
    {
        QMessageBox::warning(this, "警告",
                             "当前项目为空，无法添加图像文件夹");
        return;
    }
    //打开文件选择框，选择图像文件夹
    QString imageFoldPath = QFileDialog::getExistingDirectory(this, "选择图像文件夹");
    if (imageFoldPath.isEmpty())
    {
        return;
    }
    //将文件夹下的所有图像文件添加到non_train_list
    QString non_train_list_path =m_curProjectPath+"/non_train/image/";
    QDir dir(imageFoldPath);
    QStringList imageFiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    //将文件夹下的所有图像文件复制到non_train/image目录
    for (const QString &file : imageFiles)
    {
        QFile::copy(imageFoldPath + "/" + file, non_train_list_path + file);
    }
     //将文件夹下的所有图像文件添加到non_train_list
     add_non_train_list(imageFiles);

}


void MainWindow::mergeOverlappingStrokes()
{
    if (!m_scene) return;

    auto outlinePath = [](StrokeItem *s) -> QPainterPath {
        if (s->isPolygon())
            return s->path();
        QPainterPathStroker stroker;
        stroker.setWidth(s->pen().widthF());
        stroker.setCapStyle(Qt::RoundCap);
        stroker.setJoinStyle(Qt::RoundJoin);
        return stroker.createStroke(s->path());
    };

    bool anyMerged = true;
    while (anyMerged) {
        anyMerged = false;

        QList<StrokeItem*> allStrokes;
        for (auto *item : m_scene->items()) {
            auto *s = dynamic_cast<StrokeItem*>(item);
            if (s) allStrokes.append(s);
        }

        for (int i = 0; i < allStrokes.size() && !anyMerged; ++i) {
            for (int j = i + 1; j < allStrokes.size() && !anyMerged; ++j) {
                if (allStrokes[i]->classId() != allStrokes[j]->classId())
                    continue;

                if (allStrokes[i]->collidesWithItem(allStrokes[j])) {
                    auto united = outlinePath(allStrokes[i]).united(outlinePath(allStrokes[j]));

                    m_scene->removeItem(allStrokes[j]);
                    m_labelingItems.removeOne(allStrokes[j]);
                    ui->graphicsView->removeStroke(allStrokes[j]);
                    delete allStrokes[j];

                    allStrokes[i]->setStrokePath(united);
                    allStrokes[i]->setPolygonMode(true);
                    anyMerged = true;
                }
            }
        }
    }
}

void MainWindow::saveLabelingData()
{
    QString imagePath = m_displayedImageInfo.imagePath;
    if (imagePath.isEmpty()) return;

    Json::Value root;

    const auto &items = m_scene->items();
    for (QGraphicsItem *item : items) {
        StrokeItem *stroke = dynamic_cast<StrokeItem*>(item);
        if (stroke) {
            auto outlines = stroke->getOutlinePoints();
            QString className = m_labelsComboBox->itemText(stroke->classId());

            for (auto &poly : outlines) {
                if (poly.size() < 3) continue;

                Json::Value obj;
                obj["classname"] = className.toStdString();

                double area = 0.0;
                int n = poly.size();
                for (int i = 0; i < n; i++) {
                    int j = (i + 1) % n;
                    area += poly[i].x() * poly[j].y();
                    area -= poly[j].x() * poly[i].y();
                }
                obj["area"] = (unsigned int)(std::abs(area) / 2.0);

                qreal minX = poly[0].x(), maxX = poly[0].x();
                qreal minY = poly[0].y(), maxY = poly[0].y();
                for (const auto& pt : poly) {
                    minX = qMin(minX, pt.x());
                    maxX = qMax(maxX, pt.x());
                    minY = qMin(minY, pt.y());
                    maxY = qMax(maxY, pt.y());
                }

                obj["bbox"] = Json::Value(Json::arrayValue);
                obj["bbox"].append(minX);
                obj["bbox"].append(minY);
                obj["bbox"].append(maxX - minX);
                obj["bbox"].append(maxY - minY);

                obj["points"] = Json::Value(Json::arrayValue);
                for (const auto& pt : poly) {
                    Json::Value pnt;
                    pnt.append(pt.x());
                    pnt.append(pt.y());
                    obj["points"].append(pnt);
                }

                root.append(obj);
            }
        }

        RectItem *rect = dynamic_cast<RectItem*>(item);
        if (rect) {
            QString className = m_labelsComboBox->itemText(rect->classId());
            QVector<QPointF> points = rect->getCornerPoints();

            Json::Value obj;
            obj["classname"] = className.toStdString();

            QRectF r = rect->rect();
            qreal area = r.width() * r.height();
            obj["area"] = (unsigned int)(std::abs(area));

            obj["bbox"] = Json::Value(Json::arrayValue);
            obj["bbox"].append(r.x());
            obj["bbox"].append(r.y());
            obj["bbox"].append(r.width());
            obj["bbox"].append(r.height());

            obj["points"] = Json::Value(Json::arrayValue);
            for (const auto& pt : points) {
                Json::Value pnt;
                pnt.append(pt.x());
                pnt.append(pt.y());
                obj["points"].append(pnt);
            }

            root.append(obj);
        }
    }

    QFileInfo imageInfo(imagePath);
    QString jsonPath = imageInfo.path() + QDir::separator()
                       + imageInfo.baseName() + "_labeling.json";

    if (root.empty()) {
        QFile::remove(jsonPath);
        return;
    }

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "    ";
    std::string jsonStr = Json::writeString(writer, root);

    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << QString::fromStdString(jsonStr);
    }
}

void MainWindow::setupShortcuts()
{
    auto bind = [&](const QKeySequence &key, auto slot) {
        auto *sc = new QShortcut(key, this);
        connect(sc, &QShortcut::activated, this, slot);
    };

    bind(Qt::Key_A, [this]{ navigateList(-1); });
    bind(Qt::Key_D, [this]{ navigateList(1); });
    bind(Qt::Key_Q, [this]{ ui->actioncreate_rect->trigger(); });
    bind(Qt::Key_C, [this]{ ui->actioncursor->trigger(); });
    bind(Qt::Key_E, [this]{ ui->actionpen_brush->trigger(); });
    bind(Qt::Key_R, [this]{ ui->actionCreateRoi->trigger(); });
    bind(Qt::Key_X, [this]{
        if (m_labelsComboBox->count() == 0) return;
        int next = (m_labelsComboBox->currentIndex() + 1) % m_labelsComboBox->count();
        m_labelsComboBox->setCurrentIndex(next);
    });
}

void MainWindow::sl_showAdjustPopup()
{
    if (!m_canvasAdjustment) return;
    m_canvasAdjustment->updatePosition();
    m_canvasAdjustment->show();
    m_canvasAdjustment->raise();
    m_canvasAdjustment->activateWindow();
}

void MainWindow::sl_onBrightnessContrastChanged(int brightness, int contrast)
{
    if (m_originalPixmap.isNull()) return;

    QImage img = m_originalPixmap.toImage();

    double bFactor = brightness / 100.0;
    double cFactor = contrast / 100.0;

    if (bFactor != 1.0 || cFactor != 1.0) {
        QImage result = img.copy();
        int w = result.width();
        int h = result.height();

        double cIntercept = 127.5 * (1.0 - cFactor);

        for (int y = 0; y < h; ++y) {
            uchar *line = result.scanLine(y);
            int depth = img.depth() / 8;
            for (int x = 0; x < w; ++x) {
                uchar *pixel = line + x * depth;
                for (int ch = 0; ch < qMin(depth, 3); ++ch) {
                    double v = pixel[ch];
                    v = v * bFactor;
                    v = v * cFactor + cIntercept;
                    pixel[ch] = qBound(0, static_cast<int>(v), 255);
                }
            }
        }
        if (m_pixmapItem) {
            m_pixmapItem->setPixmap(QPixmap::fromImage(result));
        }
    } else {
        if (m_pixmapItem) {
            m_pixmapItem->setPixmap(m_originalPixmap);
        }
    }
}

void MainWindow::sl_onAnnotationOpacityChanged(int opacity)
{
    qreal opacityF = opacity / 100.0;
    for (QGraphicsItem *item : m_scene->items()) {
        if (dynamic_cast<StrokeItem*>(item) || dynamic_cast<RectItem*>(item)) {
            item->setOpacity(opacityF);
        }
    }
}

void MainWindow::navigateList(int delta)
{
    QListWidget *list = m_activeList;
    if (!list || list->count() == 0) return;

    int row = list->currentRow();
    int newRow = row + delta;
    if (newRow < 0 || newRow >= list->count()) return;

    saveLabelingData();
    list->setCurrentRow(newRow);
    emit list->itemDoubleClicked(list->item(newRow));
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat()) {
        if (event->key() == Qt::Key_W) {
            m_penSizeDelta = 1;
            m_penSizeTimer->start();
            return;
        }
        if (event->key() == Qt::Key_S) {
            m_penSizeDelta = -1;
            m_penSizeTimer->start();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat()) {
        if (event->key() == Qt::Key_W || event->key() == Qt::Key_S) {
            m_penSizeTimer->stop();
            return;
        }
    }
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::on_actionShortcuts_triggered()
{
    QDialog dlg(this);
    dlg.setWindowTitle("快捷键说明");
    dlg.setMinimumSize(420, 340);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QTableWidget *table = new QTableWidget(&dlg);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"快捷键", "功能"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);

    struct ShortcutInfo {
        const char *key;
        const char *desc;
    };

    ShortcutInfo infos[] = {
        {"A",            "上一张图片"},
        {"D",            "下一张图片"},
        {"Q",            "矩形标注模式"},
        {"C",            "光标模式"},
        {"E",            "画笔模式"},
        {"R",            "创建 ROI 区域"},
        {"X",            "循环切换标签"},
        {"W (长按)",     "增大画笔粗细"},
        {"S (长按)",     "减小画笔粗细"},
    };

    table->setRowCount(sizeof(infos) / sizeof(infos[0]));
    for (int i = 0; i < table->rowCount(); ++i) {
        QTableWidgetItem *keyItem = new QTableWidgetItem(infos[i].key);
        keyItem->setTextAlignment(Qt::AlignCenter);
        QFont font = keyItem->font();
        font.setBold(true);
        keyItem->setFont(font);
        table->setItem(i, 0, keyItem);
        table->setItem(i, 1, new QTableWidgetItem(infos[i].desc));
    }

    layout->addWidget(table);

    QPushButton *closeBtn = new QPushButton("关闭", &dlg);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg.exec();
}

void MainWindow::on_actionLabelStatistics_triggered()
{
    QMap<int, QString> classNames;
    QMap<int, QColor> classColors;
    for (int i = 0; i < m_labelsComboBox->count(); ++i) {
        classNames[i] = m_labelsComboBox->itemText(i);
        classColors[i] = m_labelsComboBox->itemData(i).value<QColor>();
    }

    auto collectFromDir = [&](const QString &subDir) -> QPair<DataSet, QSize> {
        DataSet ds;
        ds.classNames = classNames;
        ds.classColors = classColors;
        QSize imgSize;
        if (m_curProjectPath.isEmpty()) return {ds, imgSize};

        QDir dir(m_curProjectPath + "/" + subDir);
        if (!dir.exists()) return {ds, imgSize};

        QStringList filters;
        filters << "*_labeling.json";
        QStringList files = dir.entryList(filters, QDir::Files);

        for (const QString &file : files) {
            QString filePath = dir.filePath(file);
            QFile f(filePath);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

            QByteArray data = f.readAll();
            f.close();

            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(data.toStdString(), root)) continue;
            if (!root.isArray()) continue;

            QString baseName = file;
            baseName.chop(strlen("_labeling.json"));
            QString imgPath = dir.filePath(baseName);
            if (imgSize.isNull() && QFile::exists(imgPath)) {
                QImage img(imgPath);
                if (!img.isNull())
                    imgSize = img.size();
            }

            for (const auto &obj : root) {
                std::string className = obj["classname"].asString();
                int classId = -1;
                for (int i = 0; i < m_labelsComboBox->count(); ++i) {
                    if (m_labelsComboBox->itemText(i).toStdString() == className) {
                        classId = i;
                        break;
                    }
                }
                if (classId < 0) continue;

                qreal area = 0;
                if (obj.isMember("area")) {
                    area = obj["area"].asUInt();
                } else if (obj.isMember("bbox") && obj["bbox"].isArray() && obj["bbox"].size() == 4) {
                    area = obj["bbox"][2].asDouble() * obj["bbox"][3].asDouble();
                }

                qreal cRow = 0, cCol = 0;
                if (obj.isMember("points") && obj["points"].isArray() && obj["points"].size() >= 3) {
                    qreal sumRow = 0, sumCol = 0;
                    int ptCount = 0;
                    for (const auto &pt : obj["points"]) {
                        if (pt.isArray() && pt.size() >= 2) {
                            sumCol += pt[0].asDouble();
                            sumRow += pt[1].asDouble();
                            ptCount++;
                        }
                    }
                    if (ptCount > 0) {
                        cRow = sumRow / ptCount;
                        cCol = sumCol / ptCount;
                    }
                }

                ds.labelData.append({classId, area, cRow, cCol});
            }
        }
        return {ds, imgSize};
    };

    auto [trainDs, trainImgSize] = collectFromDir("train/image");
    auto [nonTrainDs, nonTrainImgSize] = collectFromDir("non_train/image");

    int imgW = qMax(trainImgSize.width(), nonTrainImgSize.width());
    int imgH = qMax(trainImgSize.height(), nonTrainImgSize.height());

    LabelStatisticsDialog dlg(trainDs, nonTrainDs, imgW, imgH, this);
    dlg.exec();
}

std::variant<std::monostate,QString> MainWindow::saigeMoelInit(SaigeInferPara param){

    ui->infer_ckBox->setChecked(false);
    this->m_inferPara.modelPath = param.modelPath;
    m_inferPara.task = static_cast<DLTASK>(param.saigeProjectType);
    m_saigeHdl.configHdl.batchSize = param.batchSize;
    m_saigeHdl.configHdl.inspectionSizeHeight = param.inferHeight;
    m_saigeHdl.configHdl.inspectionSizeWidth = param.inferWidth;
    m_saigeHdl.inferHdl = new SaigeInferenceHandle();


    // 项目类型=SEG，设备=GPU 0
    {
        unsigned int flags = param.config_flags;
        int ret = SaigeCreateInferenceHandle(m_saigeHdl.inferHdl,
                                             param.saigeProjectType,
                                             m_inferPara.modelPath,
                                             0,
                                             &m_saigeHdl.configHdl,
                                             flags);

        if (ret < 0)
        {
            return QString("创建句柄失败：%1").arg(SaigeStrError(ret));
        }
    }

    m_saigeHdl.resHdl = new SaigeInferenceResult();

    {
        int ret = SaigeCreateInferenceResult(m_saigeHdl.resHdl);

        if (ret < 0)
        {
            return QString("创建结果句柄失败：%1").arg(SaigeStrError(ret));
        }
    }

    // 配置推理选项
    if (param.option_flags != 0)
    {

        m_saigeHdl.optHdl.calcHeatmap = false;
        m_saigeHdl.optHdl.calcObjectScoreAndApplyThreshold = param.isCalcObjectScoreAndApplyThreshold;
        m_saigeHdl.optHdl.calcObjectAreaAndApplyThreshold = param.isCalcObjectAreaAndApplyThreshold;
        m_saigeHdl.optHdl.objectAreaThreshold = param.objectAreaThreshold;
        m_saigeHdl.optHdl.objectScoreThreshold = param.objectScoreThreshold;
        m_saigeHdl.optHdl.objectBoundaryThreshold = param.objectBoundaryThreshold;


        unsigned int flags = param.option_flags;
        int ret = SaigeSetInferenceOption(*m_saigeHdl.inferHdl,
                                          &m_saigeHdl.optHdl,
                                          flags);
        if (ret < 0)
        {
            return QString("设置推理选项失败：%1").arg(SaigeStrError(ret));
        }
    }

    {
        // 获取模型信息
        int ret = SaigeGetModelInfo(*m_saigeHdl.inferHdl, &m_saigeHdl.modelInfo);
        if (ret < 0)
        {
            return QString("获取模型信息失败：%1").arg(SaigeStrError(ret));
        }
    }

    // 模型成功加载
    return std::monostate();



}

void MainWindow::on_actionCreateRoi_triggered()
{
    if (m_curProjectPath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先打开项目");
        return;
    }
    if (m_currentImage.isNull()) {
        QMessageBox::warning(this, "警告", "请先加载图像");
        return;
    }
    ui->graphicsView->switchCursorMode(CURSOR_MODE::create_roi);
    ui->graphicsView->setDragMode(QGraphicsView::NoDrag);
    ui->graphicsView->updateCursor();
}

void MainWindow::saveRoi()
{
    if (m_curProjectPath.isEmpty())
        return;

    Json::Value root;
    Json::Value roiObj;
    roiObj["x"] = m_roiRect.x();
    roiObj["y"] = m_roiRect.y();
    roiObj["width"] = m_roiRect.width();
    roiObj["height"] = m_roiRect.height();
    root["roi"] = roiObj;

    Json::FastWriter writer;
    std::string jsonStr = writer.write(root);

    QString jsonPath = m_curProjectPath + "/data_config.json";
    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << QString::fromStdString(jsonStr);
    }
}

void MainWindow::loadRoi()
{
    if (m_curProjectPath.isEmpty())
        return;

    QString jsonPath = m_curProjectPath + "/data_config.json";
    if (!QFile::exists(jsonPath)) {
        m_roiRect = QRectF();
        return;
    }

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QByteArray jsonData = file.readAll();
    file.close();

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(jsonData.toStdString(), root))
        return;

    if (root.isMember("roi")) {
        Json::Value roiObj = root["roi"];
        m_roiRect = QRectF(
            roiObj["x"].asDouble(),
            roiObj["y"].asDouble(),
            roiObj["width"].asDouble(),
            roiObj["height"].asDouble());
    } else {
        m_roiRect = QRectF();
    }
}

void MainWindow::displayRoi()
{
    // 移除旧的overlay
    if (m_roiOverlayItem) {
        m_scene->removeItem(m_roiOverlayItem);
        delete m_roiOverlayItem;
        m_roiOverlayItem = nullptr;
    }

    if (m_currentImage.isNull())
        return;

    QRectF imageRect(0, 0, m_currentImage.width(), m_currentImage.height());

    if (m_roiRect.isNull()) {
        ui->graphicsView->setDrawBounds(imageRect);
        return;
    }

    // 裁剪ROI到图像范围内
    QRectF clampedRoi = m_roiRect.intersected(imageRect);
    ui->graphicsView->setDrawBounds(clampedRoi);

    // 创建overlay
    m_roiOverlayItem = new RoiOverlayItem();
    m_roiOverlayItem->setImageRect(imageRect);
    m_roiOverlayItem->setRoiRect(clampedRoi);
    m_roiOverlayItem->setZValue(1000);
    m_scene->addItem(m_roiOverlayItem);
}

void MainWindow::saveCategories()
{
    if (m_curProjectPath.isEmpty())
        return;

    // 读取现有JSON
    QString jsonPath = m_curProjectPath + "/data_config.json";
    Json::Value root;
    Json::Reader reader;
    QFile file(jsonPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray jsonData = file.readAll();
        file.close();
        reader.parse(jsonData.toStdString(), root);
    }

    // 保存类别
    Json::Value categoriesArr;
    for (const QString &cat : m_categories) {
        categoriesArr.append(cat.toStdString());
    }
    root["categories"] = categoriesArr;

    // 写入文件
    Json::FastWriter writer;
    std::string jsonStr = writer.write(root);
    QFile outFile(jsonPath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&outFile);
        out << QString::fromStdString(jsonStr);
    }
}

void MainWindow::loadCategories()
{
    if (m_curProjectPath.isEmpty())
        return;

    QString jsonPath = m_curProjectPath + "/data_config.json";
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

    if (root.isMember("categories")) {
        m_categories.clear();
        const Json::Value &arr = root["categories"];
        for (Json::ArrayIndex i = 0; i < arr.size(); ++i) {
            m_categories.append(QString::fromStdString(arr[i].asString()));
        }

        // 更新左侧标签ComboBox
        m_labelsComboBox->clear();
        // 默认颜色
        QList<QColor> defaultColors = {Qt::red, Qt::green, Qt::blue, Qt::yellow, Qt::cyan, Qt::magenta};
        for (int i = 0; i < m_categories.size(); ++i) {
            QColor color = (i < defaultColors.size()) ? defaultColors[i] : QColor::fromHsv((i * 40) % 360, 200, 200);
            m_labelsComboBox->addItem(m_categories[i], QVariant::fromValue(color));
        }
        if (m_labelsComboBox->count() > 0) {
            m_labelsComboBox->setCurrentIndex(0);
            QColor firstColor = m_labelsComboBox->itemData(0).value<QColor>();
            ui->graphicsView->setPenColor(firstColor);
            ui->graphicsView->setCurrentClassId(0);
        }
    }
}

void MainWindow::syncFilterComboBoxes()
{
    QString labelText = ui->labelFilterComboBox->currentText();
    QString inferText = ui->inferFilterComboBox->currentText();

    ui->labelFilterComboBox->blockSignals(true);
    ui->inferFilterComboBox->blockSignals(true);

    ui->labelFilterComboBox->clear();
    ui->inferFilterComboBox->clear();

    ui->labelFilterComboBox->addItem("全部");
    ui->inferFilterComboBox->addItem("全部");

    // 从m_labelsComboBox同步类别
    m_categories.clear();
    for (int i = 0; i < m_labelsComboBox->count(); ++i) {
        QString name = m_labelsComboBox->itemText(i);
        m_categories.append(name);
        ui->labelFilterComboBox->addItem(name);
        ui->inferFilterComboBox->addItem(name);
    }

    // 恢复之前的选择
    int labelIdx = ui->labelFilterComboBox->findText(labelText);
    ui->labelFilterComboBox->setCurrentIndex(labelIdx >= 0 ? labelIdx : 0);

    int inferIdx = ui->inferFilterComboBox->findText(inferText);
    ui->inferFilterComboBox->setCurrentIndex(inferIdx >= 0 ? inferIdx : 0);

    ui->labelFilterComboBox->blockSignals(false);
    ui->inferFilterComboBox->blockSignals(false);
}

void MainWindow::applyFilter()
{
    // 始终从磁盘重建文件列表
    m_allImageFiles.clear();
    if (!m_curProjectPath.isEmpty()) {
        QDir nonTrainDir(m_curProjectPath + "/non_train/image");
        QDir trainDir(m_curProjectPath + "/train/image");
        QStringList filters = {"*.jpg", "*.png", "*.tiff", "*.jpeg", "*.bmp", "*.raw", "*.tif"};

        QStringList nonTrainFiles = nonTrainDir.entryList(filters, QDir::Files);
        for (const QString &f : nonTrainFiles)
            m_allImageFiles.append("non_train:" + f);

        QStringList trainFiles = trainDir.entryList(filters, QDir::Files);
        for (const QString &f : trainFiles)
            m_allImageFiles.append("train:" + f);
    }

    QString labelText = ui->labelFilterComboBox->currentText();
    QString inferText = ui->inferFilterComboBox->currentText();
    bool labelAll = (labelText == "全部");
    bool inferAll = (inferText == "全部");

    // 筛选non_train_list
    ui->non_train_list->clear();
    // 筛选train_list
    ui->train_list->clear();

    for (const QString &entry : m_allImageFiles) {
        int colonPos = entry.indexOf(':');
        QString folder = entry.left(colonPos);
        QString fileName = entry.mid(colonPos + 1);
        QString baseName = QFileInfo(fileName).baseName();

        bool passLabel = labelAll;
        bool passInfer = inferAll;

        // 检查标注文件
        if (!passLabel) {
            QString labelDir = m_curProjectPath + "/" + folder + "/label";
            QStringList labelExts = {"json", "xml", "txt"};
            for (const QString &ext : labelExts) {
                // 尝试 _labeling.json 格式
                QString labelPath = QDir(labelDir).filePath(baseName + "_labeling." + ext);
                if (!QFile::exists(labelPath)) {
                    // 尝试 baseName.ext 格式
                    labelPath = QDir(labelDir).filePath(baseName + "." + ext);
                }
                if (QFile::exists(labelPath) && ext == "json") {
                    QFile file(labelPath);
                    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QByteArray data = file.readAll();
                        file.close();
                        Json::Value root;
                        Json::Reader reader;
                        if (reader.parse(data.toStdString(), root) && root.isArray()) {
                            for (Json::ArrayIndex i = 0; i < root.size(); ++i) {
                                if (root[i].isMember("classname") &&
                                    QString::fromStdString(root[i]["classname"].asString()) == labelText) {
                                    passLabel = true;
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }

        // 检查推理文件
        if (!passInfer) {
            QString inferPath = m_curProjectPath + "/" + folder + "/image/" + baseName + "_infer.json";
            if (QFile::exists(inferPath)) {
                QFile file(inferPath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QByteArray data = file.readAll();
                    file.close();
                    Json::Value root;
                    Json::Reader reader;
                    if (reader.parse(data.toStdString(), root) && root.isArray()) {
                        for (Json::ArrayIndex i = 0; i < root.size(); ++i) {
                            if (root[i].isMember("classname") &&
                                QString::fromStdString(root[i]["classname"].asString()) == inferText) {
                                passInfer = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // AND逻辑：两个条件都要满足
        if (passLabel && passInfer) {
            if (folder == "non_train")
                ui->non_train_list->addItem(fileName);
            else
                ui->train_list->addItem(fileName);
        }
    }
}
