#include "canvasadjustmentwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>

CanvasAdjustmentWidget::CanvasAdjustmentWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground);
    setObjectName("canvas_adjustment");

    m_bcTimer.setSingleShot(true);
    m_bcTimer.setInterval(33);
    connect(&m_bcTimer, &QTimer::timeout, this, &CanvasAdjustmentWidget::sl_emitBcChanged);

    buildUi();

    setStyleSheet(R"(
        #canvas_adjustment {
            background: rgba(245, 245, 245, 245);
            border: 1px solid #ccc;
            border-radius: 6px;
        }
        #canvas_adjustment QSlider::groove:horizontal {
            height: 4px;
            background: rgba(0, 0, 0, 55);
            border-radius: 2px;
        }
        #canvas_adjustment QSlider::handle:horizontal {
            background: #0071e3;
            width: 14px; height: 14px;
            margin: -5px 0;
            border-radius: 7px;
        }
        #canvas_adjustment QSlider::sub-page:horizontal {
            background: #0071e3;
            border-radius: 2px;
        }
        #canvas_adjustment QToolButton {
            background: transparent;
            border: none;
            color: #333;
            font-size: 12px;
        }
        #canvas_adjustment QLabel {
            color: #333;
            font-size: 11px;
        }
    )");
}

void CanvasAdjustmentWidget::buildUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 10, 12, 10);
    mainLayout->setSpacing(6);

    auto makeRow = [&](const QString &name, QSlider *&slider, QLabel *&valueLabel, auto resetSlot) {
        auto *row = new QHBoxLayout();
        auto *label = new QLabel(name);
        label->setFixedWidth(70);
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 200);
        slider->setValue(100);
        valueLabel = new QLabel("100");
        valueLabel->setFixedWidth(35);
        valueLabel->setAlignment(Qt::AlignCenter);
        auto *resetBtn = new QToolButton();
        resetBtn->setText(QString::fromUtf8("\u21BA"));
        resetBtn->setFixedSize(22, 22);
        connect(resetBtn, &QToolButton::clicked, this, resetSlot);
        row->addWidget(label);
        row->addWidget(slider, 1);
        row->addWidget(valueLabel);
        row->addWidget(resetBtn);
        mainLayout->addLayout(row);
    };

    makeRow(QString::fromUtf8("标注透明度"), m_annotationOpacitySlider, m_annotationOpacityValueLabel,
            &CanvasAdjustmentWidget::sl_resetAnnotationOpacity);
    makeRow("Brightness", m_brightnessSlider, m_brightnessValueLabel,
            &CanvasAdjustmentWidget::sl_resetBrightness);
    makeRow("Contrast", m_contrastSlider, m_contrastValueLabel,
            &CanvasAdjustmentWidget::sl_resetContrast);

    connect(m_annotationOpacitySlider, &QSlider::valueChanged,
            this, &CanvasAdjustmentWidget::sl_onAnnotationOpacityChanged);
    connect(m_brightnessSlider, &QSlider::valueChanged, this, &CanvasAdjustmentWidget::sl_onBcChanged);
    connect(m_contrastSlider, &QSlider::valueChanged, this, &CanvasAdjustmentWidget::sl_onBcChanged);
}

void CanvasAdjustmentWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    painter.setBrush(QColor(245, 245, 245, 245));
    painter.drawRoundedRect(rect(), 6, 6);
    QWidget::paintEvent(event);
}

void CanvasAdjustmentWidget::updatePosition()
{
    QWidget *btn = qobject_cast<QWidget*>(sender());
    if (!btn) {
        QList<QToolButton*> btns = parentWidget()->findChildren<QToolButton*>();
        for (auto *b : btns) {
            if (b->toolTip().contains(QString::fromUtf8("显示调节"))) {
                btn = b;
                break;
            }
        }
    }
    if (!btn) return;

    QPoint globalPos = btn->mapToGlobal(QPoint(0, 0));
    QSize sz = sizeHint();
    int x = globalPos.x() - sz.width() / 2 + btn->width() / 2;
    int y = globalPos.y() - sz.height() - 4;

    QScreen *screen = QApplication::screenAt(globalPos);
    if (screen) {
        QRect screenRect = screen->availableGeometry();
        x = qMax(screenRect.left(), qMin(x, screenRect.right() - sz.width()));
        y = qMax(screenRect.top(), y);
    }
    move(x, y);
}

void CanvasAdjustmentWidget::sl_onBcChanged()
{
    updateValueLabel(m_brightnessValueLabel, m_brightnessSlider->value());
    updateValueLabel(m_contrastValueLabel, m_contrastSlider->value());
    if (!m_bcTimer.isActive())
        m_bcTimer.start();
}

void CanvasAdjustmentWidget::sl_emitBcChanged()
{
    emit sg_brightnessContrastChanged(m_brightnessSlider->value(), m_contrastSlider->value());
}

void CanvasAdjustmentWidget::sl_onAnnotationOpacityChanged(int value)
{
    updateValueLabel(m_annotationOpacityValueLabel, value);
    emit sg_annotationOpacityChanged(value);
}

void CanvasAdjustmentWidget::sl_resetBrightness()
{
    m_brightnessSlider->blockSignals(true);
    m_brightnessSlider->setValue(100);
    m_brightnessSlider->blockSignals(false);
    updateValueLabel(m_brightnessValueLabel, 100);
    sl_emitBcChanged();
}

void CanvasAdjustmentWidget::sl_resetContrast()
{
    m_contrastSlider->blockSignals(true);
    m_contrastSlider->setValue(100);
    m_contrastSlider->blockSignals(false);
    updateValueLabel(m_contrastValueLabel, 100);
    sl_emitBcChanged();
}

void CanvasAdjustmentWidget::sl_resetAnnotationOpacity()
{
    m_annotationOpacitySlider->blockSignals(true);
    m_annotationOpacitySlider->setValue(100);
    m_annotationOpacitySlider->blockSignals(false);
    updateValueLabel(m_annotationOpacityValueLabel, 100);
    emit sg_annotationOpacityChanged(100);
}

void CanvasAdjustmentWidget::setBrightness(int value)
{
    m_brightnessSlider->blockSignals(true);
    m_brightnessSlider->setValue(value);
    m_brightnessSlider->blockSignals(false);
    updateValueLabel(m_brightnessValueLabel, value);
}

void CanvasAdjustmentWidget::setContrast(int value)
{
    m_contrastSlider->blockSignals(true);
    m_contrastSlider->setValue(value);
    m_contrastSlider->blockSignals(false);
    updateValueLabel(m_contrastValueLabel, value);
}

void CanvasAdjustmentWidget::setAnnotationOpacity(int value)
{
    m_annotationOpacitySlider->blockSignals(true);
    m_annotationOpacitySlider->setValue(value);
    m_annotationOpacitySlider->blockSignals(false);
    updateValueLabel(m_annotationOpacityValueLabel, value);
}

int CanvasAdjustmentWidget::brightness() const { return m_brightnessSlider->value(); }
int CanvasAdjustmentWidget::contrast() const { return m_contrastSlider->value(); }
int CanvasAdjustmentWidget::annotationOpacity() const { return m_annotationOpacitySlider->value(); }

void CanvasAdjustmentWidget::updateValueLabel(QLabel *label, int value)
{
    label->setText(QString::number(value));
}
