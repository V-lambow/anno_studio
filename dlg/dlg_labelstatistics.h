#ifndef DLG_LABELSTATISTICS_H
#define DLG_LABELSTATISTICS_H

#include <QDialog>
#include <QMap>
#include <QString>
#include <QList>
#include <QPointF>
#include <QColor>
#include <QWidget>

class QTabWidget;

struct LabelStats {
    QString name;
    QColor color;
    int count = 0;
    QList<qreal> areas;
    QList<QPointF> centers;
};

class BarChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BarChartWidget(QWidget *parent = nullptr);
    void setData(const QList<LabelStats> &stats, int total);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<LabelStats> m_stats;
    int m_total = 0;
};

class BoxChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BoxChartWidget(QWidget *parent = nullptr);
    void setData(const QList<LabelStats> &stats);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<LabelStats> m_stats;
};

struct LabelItem {
    int classId;
    qreal area;
    qreal centerRow;
    qreal centerCol;
};

struct DataSet {
    QMap<int, QString> classNames;
    QMap<int, QColor> classColors;
    QList<LabelItem> labelData;
};

class ScatterPlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScatterPlotWidget(QWidget *parent = nullptr);
    void setData(const QList<LabelStats> &stats, int imgW, int imgH);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<LabelStats> m_stats;
    int m_imgW = 0;
    int m_imgH = 0;
};

class LabelStatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LabelStatisticsDialog(const DataSet &trainData,
                                   const DataSet &nonTrainData,
                                   int imgW, int imgH,
                                   QWidget *parent = nullptr);

private:
    QWidget *createTab(const DataSet &data, int imgW, int imgH);

    QTabWidget *m_tabWidget = nullptr;
};

#endif // DLG_LABELSTATISTICS_H
