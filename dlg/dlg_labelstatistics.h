#ifndef DLG_LABELSTATISTICS_H
#define DLG_LABELSTATISTICS_H

#include <QDialog>
#include <QMap>
#include <QString>
#include <QList>
#include <QWidget>

class QTabWidget;

struct LabelStats {
    QString name;
    int count = 0;
    QList<qreal> areas;
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

struct DataSet {
    QMap<int, QString> classNames;
    QList<QPair<int, qreal>> labelData;
};

class LabelStatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LabelStatisticsDialog(const DataSet &trainData,
                                   const DataSet &nonTrainData,
                                   QWidget *parent = nullptr);

private:
    QWidget *createTab(const DataSet &data);

    QTabWidget *m_tabWidget = nullptr;
};

#endif // DLG_LABELSTATISTICS_H
