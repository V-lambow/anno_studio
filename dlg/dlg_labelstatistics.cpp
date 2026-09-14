#include "dlg_labelstatistics.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <numeric>

static QList<LabelStats> computeStats(const DataSet &ds)
{
    QMap<int, LabelStats> map;
    for (const auto &item : ds.labelData) {
        map[item.classId].count++;
        if (item.area > 0)
            map[item.classId].areas.append(item.area);
        map[item.classId].centers.append(QPointF(item.centerRow, item.centerCol));
    }
    QList<LabelStats> result;
    for (auto it = map.begin(); it != map.end(); ++it) {
        LabelStats s;
        s.name = ds.classNames.value(it.key(), QString::number(it.key()));
        s.color = ds.classColors.value(it.key(), Qt::gray);
        s.count = it->count;
        s.areas = it->areas;
        s.centers = it->centers;
        result.append(s);
    }
    std::sort(result.begin(), result.end(),
              [](const LabelStats &a, const LabelStats &b) { return a.count > b.count; });
    return result;
}

BarChartWidget::BarChartWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(220);
}

void BarChartWidget::setData(const QList<LabelStats> &stats, int total)
{
    m_stats = stats;
    m_total = total;
    update();
}

void BarChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect rc = rect().adjusted(10, 10, -10, -10);

    if (m_stats.isEmpty()) {
        p.setPen(Qt::gray);
        p.drawText(rc, Qt::AlignCenter, QString::fromUtf8("暂无数据"));
        return;
    }

    QFont titleFont = p.font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(Qt::black);
    p.drawText(rc.left(), rc.top() + 16, QString::fromUtf8("数量统计 (共 %1 个)").arg(m_total));

    QFont labelFont = p.font();
    labelFont.setPointSize(9);
    labelFont.setBold(false);
    p.setFont(labelFont);

    int barAreaTop = rc.top() + 30;
    int barAreaBottom = rc.bottom() - 30;
    int barAreaHeight = barAreaBottom - barAreaTop;
    int barAreaLeft = rc.left() + 45;
    int barAreaRight = rc.right();

    int n = m_stats.size();
    if (n == 0) return;

    int maxCount = m_stats[0].count;
    if (maxCount == 0) maxCount = 1;

    int totalW = barAreaRight - barAreaLeft;
    int gap = qMax(4, totalW / (3 * n + 1));
    int barWidth = gap * 2;

    for (int i = 0; i < n; ++i) {
        const LabelStats &s = m_stats[i];
        int x = barAreaLeft + gap + i * (barWidth + gap);
        int h = static_cast<int>(barAreaHeight * s.count / static_cast<qreal>(maxCount));
        int y = barAreaBottom - h;

        QColor color = s.color;
        QPainterPath barPath;
        barPath.addRoundedRect(x, y, barWidth, h, 3, 3);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(barPath);

        p.setPen(Qt::black);
        QRect countRc(x, y - 16, barWidth, 14);
        p.drawText(countRc, Qt::AlignHCenter | Qt::AlignBottom, QString::number(s.count));

        QRect nameRc(x - 5, barAreaBottom + 4, barWidth + 10, 16);
        p.drawText(nameRc, Qt::AlignHCenter | Qt::AlignTop, s.name);
    }

    p.setPen(QColor(200, 200, 200));
    for (int i = 0; i <= 4; ++i) {
        int y = barAreaBottom - barAreaHeight * i / 4;
        p.drawLine(barAreaLeft - 3, y, barAreaRight, y);
        p.setPen(QColor(120, 120, 120));
        p.drawText(barAreaLeft - 40, y - 6, 35, 12, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(maxCount * i / 4));
        p.setPen(QColor(200, 200, 200));
    }
}

BoxChartWidget::BoxChartWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(220);
}

void BoxChartWidget::setData(const QList<LabelStats> &stats)
{
    m_stats = stats;
    update();
}

void BoxChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect rc = rect().adjusted(10, 10, -10, -10);

    QList<LabelStats> valid;
    for (const auto &s : m_stats) {
        if (!s.areas.isEmpty())
            valid.append(s);
    }

    if (valid.isEmpty()) {
        p.setPen(Qt::gray);
        QFont f = p.font();
        f.setPointSize(11);
        p.setFont(f);
        p.drawText(rc, Qt::AlignCenter, QString::fromUtf8("暂无面积数据"));
        return;
    }

    QFont titleFont = p.font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(Qt::black);
    p.drawText(rc.left(), rc.top() + 16, QString::fromUtf8("面积统计 (像素²)"));

    QFont labelFont = p.font();
    labelFont.setPointSize(9);
    labelFont.setBold(false);
    p.setFont(labelFont);

    int chartTop = rc.top() + 30;
    int chartBottom = rc.bottom() - 25;
    int chartLeft = rc.left() + 50;
    int chartRight = rc.right();
    int chartHeight = chartBottom - chartTop;

    qreal globalMin = 1e18, globalMax = -1e18;
    for (const auto &s : valid) {
        for (qreal a : s.areas) {
            globalMin = qMin(globalMin, a);
            globalMax = qMax(globalMax, a);
        }
    }
    if (globalMax <= globalMin) globalMax = globalMin + 1;

    qreal range = globalMax - globalMin;
    globalMin -= range * 0.05;
    globalMax += range * 0.05;
    qreal totalRange = globalMax - globalMin;
    if (totalRange <= 0) totalRange = 1;

    auto toY = [&](qreal val) -> int {
        return chartBottom - static_cast<int>(chartHeight * (val - globalMin) / totalRange);
    };

    p.setPen(QColor(200, 200, 200));
    for (int i = 0; i <= 4; ++i) {
        qreal val = globalMin + totalRange * i / 4;
        int y = toY(val);
        p.drawLine(chartLeft - 3, y, chartRight, y);
        p.setPen(QColor(120, 120, 120));
        QString label;
        if (val >= 1e6) label = QString::number(val / 1e6, 'f', 1) + "M";
        else if (val >= 1e3) label = QString::number(val / 1e3, 'f', 1) + "K";
        else label = QString::number(static_cast<int>(val));
        p.drawText(chartLeft - 45, y - 7, 40, 14, Qt::AlignRight | Qt::AlignVCenter, label);
        p.setPen(QColor(200, 200, 200));
    }

    int n = valid.size();
    int totalW = chartRight - chartLeft;
    int gap = qMax(6, totalW / (3 * n + 1));
    int boxW = gap * 2;

    for (int i = 0; i < n; ++i) {
        const LabelStats &s = valid[i];
        QList<qreal> sorted = s.areas;
        std::sort(sorted.begin(), sorted.end());

        int sz = sorted.size();
        qreal q1 = sorted[sz * 25 / 100];
        qreal med = sorted[sz / 2];
        qreal q3 = sorted[sz * 75 / 100];
        qreal iqr = q3 - q1;
        qreal whLo = qMax(sorted.first(), q1 - 1.5 * iqr);
        qreal whHi = qMin(sorted.last(), q3 + 1.5 * iqr);

        int x = chartLeft + gap + i * (boxW + gap);
        int cx = x + boxW / 2;

        QColor color = s.color;
        QColor fillColor = color;
        fillColor.setAlpha(60);

        int yQ1 = toY(q1), yQ3 = toY(q3), yMed = toY(med);
        int yLo = toY(whLo), yHi = toY(whHi);

        p.setPen(QPen(color, 1));
        p.setBrush(fillColor);
        p.drawRect(x, yQ3, boxW, yQ1 - yQ3);

        p.setPen(QPen(color, 2));
        p.drawLine(x, yMed, x + boxW, yMed);

        p.setPen(QPen(color, 1));
        p.drawLine(cx, yHi, cx, yQ3);
        p.drawLine(cx, yQ1, cx, yLo);

        int whiskerW = boxW / 3;
        p.drawLine(cx - whiskerW / 2, yHi, cx + whiskerW / 2, yHi);
        p.drawLine(cx - whiskerW / 2, yLo, cx + whiskerW / 2, yLo);

        QRect nameRc(x - 5, chartBottom + 4, boxW + 10, 16);
        p.setPen(Qt::black);
        p.drawText(nameRc, Qt::AlignHCenter | Qt::AlignTop, s.name);
    }
}

ScatterPlotWidget::ScatterPlotWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(220);
}

void ScatterPlotWidget::setData(const QList<LabelStats> &stats, int imgW, int imgH)
{
    m_stats = stats;
    m_imgW = imgW;
    m_imgH = imgH;
    update();
}

void ScatterPlotWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect rc = rect().adjusted(10, 10, -10, -10);

    QList<LabelStats> valid;
    for (const auto &s : m_stats) {
        if (!s.centers.isEmpty())
            valid.append(s);
    }

    if (valid.isEmpty()) {
        p.setPen(Qt::gray);
        QFont f = p.font();
        f.setPointSize(11);
        p.setFont(f);
        p.drawText(rc, Qt::AlignCenter, QString::fromUtf8("暂无位置数据"));
        return;
    }

    QFont titleFont = p.font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(Qt::black);
    p.drawText(rc.left(), rc.top() + 16, QString::fromUtf8("位置分布 (row x col)"));

    QFont labelFont = p.font();
    labelFont.setPointSize(9);
    labelFont.setBold(false);
    p.setFont(labelFont);

    int chartTop = rc.top() + 30;
    int chartBottom = rc.bottom() - 30;
    int chartLeft = rc.left() + 55;
    int chartRight = rc.right() - 10;
    int chartWidth = chartRight - chartLeft;
    int chartHeight = chartBottom - chartTop;

    qreal maxRow = m_imgH > 0 ? m_imgH : 1;
    qreal maxCol = m_imgW > 0 ? m_imgW : 1;

    for (const auto &s : valid) {
        for (const auto &c : s.centers) {
            if (c.y() > maxRow) maxRow = c.y();
            if (c.x() > maxCol) maxCol = c.x();
        }
    }
    maxRow *= 1.05;
    maxCol *= 1.05;
    if (maxRow <= 0) maxRow = 1;
    if (maxCol <= 0) maxCol = 1;

    auto toX = [&](qreal col) -> int {
        return chartLeft + static_cast<int>(chartWidth * col / maxCol);
    };
    auto toY = [&](qreal row) -> int {
        return chartTop + static_cast<int>(chartHeight * row / maxRow);
    };

    p.setPen(QColor(200, 200, 200));
    for (int i = 0; i <= 4; ++i) {
        int y = chartTop + chartHeight * i / 4;
        p.drawLine(chartLeft, y, chartRight, y);
        qreal val = maxRow * i / 4;
        p.setPen(QColor(120, 120, 120));
        p.drawText(chartLeft - 50, y - 7, 45, 14, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(static_cast<int>(val)));
        p.setPen(QColor(200, 200, 200));
    }
    for (int i = 0; i <= 4; ++i) {
        int x = chartLeft + chartWidth * i / 4;
        p.drawLine(x, chartTop, x, chartBottom);
        qreal val = maxCol * i / 4;
        p.setPen(QColor(120, 120, 120));
        QRect labelRc(x - 20, chartBottom + 4, 40, 14);
        p.drawText(labelRc, Qt::AlignHCenter | Qt::AlignTop, QString::number(static_cast<int>(val)));
        p.setPen(QColor(200, 200, 200));
    }

    p.setPen(Qt::NoPen);
    for (int i = 0; i < valid.size(); ++i) {
        const LabelStats &s = valid[i];
        QColor color = s.color;
        QColor fillColor = color;
        fillColor.setAlpha(160);
        p.setBrush(fillColor);
        for (const auto &c : s.centers) {
            int cx = toX(c.x());
            int cy = toY(c.y());
            p.drawEllipse(QPoint(cx, cy), 4, 4);
        }
    }

    QFont legFont = p.font();
    legFont.setPointSize(8);
    p.setFont(legFont);
    int legX = chartRight - 80;
    int legY = chartTop + 4;
    for (int i = 0; i < valid.size(); ++i) {
        QColor color = valid[i].color;
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPoint(legX, legY + 6), 4, 4);
        p.setPen(Qt::black);
        p.drawText(legX + 8, legY, 70, 14, Qt::AlignLeft | Qt::AlignVCenter, valid[i].name);
        legY += 16;
    }
}

LabelStatisticsDialog::LabelStatisticsDialog(const DataSet &trainData,
                                             const DataSet &nonTrainData,
                                             int imgW, int imgH,
                                             QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("标注统计"));
    resize(780, 700);

    m_tabWidget = new QTabWidget(this);

    m_tabWidget->addTab(createTab(trainData, imgW, imgH), QString::fromUtf8("训练集"));
    m_tabWidget->addTab(createTab(nonTrainData, imgW, imgH), QString::fromUtf8("非训练集"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->addWidget(m_tabWidget);
}

QWidget *LabelStatisticsDialog::createTab(const DataSet &data, int imgW, int imgH)
{
    auto *tab = new QWidget();
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(8);

    QList<LabelStats> stats = computeStats(data);
    int total = 0;
    for (const auto &s : stats) total += s.count;

    auto *barChart = new BarChartWidget(tab);
    barChart->setData(stats, total);

    auto *boxChart = new BoxChartWidget(tab);
    boxChart->setData(stats);

    auto *scatterChart = new ScatterPlotWidget(tab);
    scatterChart->setData(stats, imgW, imgH);

    layout->addWidget(barChart, 1);
    layout->addWidget(boxChart, 1);
    layout->addWidget(scatterChart, 1);

    return tab;
}
