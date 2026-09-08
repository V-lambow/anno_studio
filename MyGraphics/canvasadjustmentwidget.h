#ifndef CANVASADJUSTMENTWIDGET_H
#define CANVASADJUSTMENTWIDGET_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QToolButton>
#include <QTimer>

class CanvasAdjustmentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CanvasAdjustmentWidget(QWidget *parent = nullptr);

    void setBrightness(int value);
    void setContrast(int value);
    void setAnnotationOpacity(int value);

    int brightness() const;
    int contrast() const;
    int annotationOpacity() const;

    void updatePosition();

signals:
    void sg_brightnessContrastChanged(int brightness, int contrast);
    void sg_annotationOpacityChanged(int opacity);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void sl_onBcChanged();
    void sl_emitBcChanged();
    void sl_onAnnotationOpacityChanged(int value);
    void sl_resetBrightness();
    void sl_resetContrast();
    void sl_resetAnnotationOpacity();

private:
    void buildUi();
    void updateValueLabel(QLabel *label, int value);

    QWidget *m_contentWidget;

    QSlider *m_annotationOpacitySlider;
    QLabel *m_annotationOpacityValueLabel;

    QSlider *m_brightnessSlider;
    QLabel *m_brightnessValueLabel;

    QSlider *m_contrastSlider;
    QLabel *m_contrastValueLabel;

    QTimer m_bcTimer;
};

#endif // CANVASADJUSTMENTWIDGET_H
