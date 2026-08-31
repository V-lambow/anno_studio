#ifndef LABEL_EXPORTER_H
#define LABEL_EXPORTER_H

#include <QString>
#include <QVector>
#include <QPointF>
#include <QRectF>
#include <QJsonObject>
#include <QJsonArray>

class QGraphicsScene;
class QComboBox;

struct AnnoRect {
    int classId;
    QRectF bbox;
};

struct AnnoPolygon {
    int classId;
    QVector<QPointF> points;
};

class LabelExporter
{
public:
    static void collectFromScene(QGraphicsScene *scene, QComboBox *labelsCombo,
                                 QList<AnnoRect> &rects, QList<AnnoPolygon> &polygons);

    static bool exportYoloHBB(const QString &dir, const QString &imgName,
                              int imgW, int imgH, const QList<AnnoRect> &rects,
                              const QStringList &classes);

    static bool exportYoloSeg(const QString &dir, const QString &imgName,
                              int imgW, int imgH, const QList<AnnoPolygon> &polygons,
                              const QStringList &classes);

    static bool exportVocDetection(const QString &dir, const QString &imgName,
                                   int imgW, int imgH, const QList<AnnoRect> &rects);

    static bool exportVocSegmentation(const QString &dir, const QString &imgName,
                                      int imgW, int imgH, const QList<AnnoPolygon> &polygons);

    static bool exportCocoDetection(const QString &jsonPath,
                                    const QStringList &imgNames,
                                    const QList<QPair<int,int>> &imgSizes,
                                    const QList<QList<AnnoRect>> &allRects,
                                    const QStringList &classes);

    static bool exportCocoSegmentation(const QString &jsonPath,
                                       const QStringList &imgNames,
                                       const QList<QPair<int,int>> &imgSizes,
                                       const QList<QList<AnnoPolygon>> &allPolygons,
                                       const QStringList &classes);

private:
    static double polygonArea(const QVector<QPointF> &pts);
    static QJsonObject makeCocoImage(int id, const QString &fileName, int w, int h);
};

#endif // LABEL_EXPORTER_H
