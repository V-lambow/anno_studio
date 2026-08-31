#include "label_exporter.h"
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QComboBox>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QXmlStreamWriter>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <cmath>
#include "../../MyGraphics/rectitem.h"
#include "../../MyGraphics/painteditem.h"

void LabelExporter::collectFromScene(QGraphicsScene *scene, QComboBox *labelsCombo,
                                     QList<AnnoRect> &rects, QList<AnnoPolygon> &polygons)
{
    if (!scene) return;
    const auto &items = scene->items();
    for (QGraphicsItem *item : items) {
        if (RectItem *r = dynamic_cast<RectItem*>(item)) {
            QRectF rct = r->rect();
            QPointF tl = r->mapToScene(rct.topLeft());
            QPointF br = r->mapToScene(rct.bottomRight());
            rects.append({r->classId(), QRectF(tl, br).normalized()});
        }
        else if (StrokeItem *s = dynamic_cast<StrokeItem*>(item)) {
            auto outlines = s->getOutlinePoints();
            for (auto &poly : outlines) {
                if (poly.size() < 3) continue;
                QVector<QPointF> scenePts;
                scenePts.reserve(poly.size());
                for (auto &pt : poly)
                    scenePts.append(s->mapToScene(pt));
                polygons.append({s->classId(), scenePts});
            }
        }
    }
}

bool LabelExporter::exportYoloHBB(const QString &dir, const QString &imgName,
                                  int imgW, int imgH, const QList<AnnoRect> &rects,
                                  const QStringList &classes)
{
    QFileInfo info(imgName);
    QString outPath = dir + "/" + info.completeBaseName() + ".txt";
    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream ts(&file);
    for (const auto &r : rects) {
        if (r.classId < 0 || r.classId >= classes.size()) continue;
        double cx = (r.bbox.left() + r.bbox.right()) / 2.0 / imgW;
        double cy = (r.bbox.top() + r.bbox.bottom()) / 2.0 / imgH;
        double w = r.bbox.width() / imgW;
        double h = r.bbox.height() / imgH;
        ts << r.classId << " "
           << QString::number(cx, 'f', 6) << " "
           << QString::number(cy, 'f', 6) << " "
           << QString::number(w, 'f', 6) << " "
           << QString::number(h, 'f', 6) << "\n";
    }
    return true;
}

bool LabelExporter::exportYoloSeg(const QString &dir, const QString &imgName,
                                  int imgW, int imgH, const QList<AnnoPolygon> &polygons,
                                  const QStringList &classes)
{
    QFileInfo info(imgName);
    QString outPath = dir + "/" + info.completeBaseName() + ".txt";
    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream ts(&file);
    for (const auto &p : polygons) {
        if (p.classId < 0 || p.classId >= classes.size()) continue;
        if (p.points.size() < 3) continue;
        ts << p.classId;
        for (const auto &pt : p.points) {
            ts << " " << QString::number(pt.x() / imgW, 'f', 6)
               << " " << QString::number(pt.y() / imgH, 'f', 6);
        }
        ts << "\n";
    }
    return true;
}

bool LabelExporter::exportVocDetection(const QString &dir, const QString &imgName,
                                       int imgW, int imgH, const QList<AnnoRect> &rects)
{
    QFileInfo info(imgName);
    QString outPath = dir + "/" + info.completeBaseName() + ".xml";
    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("annotation");
    xml.writeTextElement("filename", info.fileName());
    xml.writeStartElement("size");
    xml.writeTextElement("width", QString::number(imgW));
    xml.writeTextElement("height", QString::number(imgH));
    xml.writeTextElement("depth", "3");
    xml.writeEndElement();

    for (const auto &r : rects) {
        xml.writeStartElement("object");
        xml.writeTextElement("name", QString::number(r.classId));
        xml.writeStartElement("bndbox");
        xml.writeTextElement("xmin", QString::number(int(r.bbox.left())));
        xml.writeTextElement("ymin", QString::number(int(r.bbox.top())));
        xml.writeTextElement("xmax", QString::number(int(r.bbox.right())));
        xml.writeTextElement("ymax", QString::number(int(r.bbox.bottom())));
        xml.writeEndElement();
        xml.writeEndElement();
    }
    xml.writeEndElement();
    return true;
}

bool LabelExporter::exportVocSegmentation(const QString &dir, const QString &imgName,
                                          int imgW, int imgH, const QList<AnnoPolygon> &polygons)
{
    QFileInfo info(imgName);
    QString outPath = dir + "/" + info.completeBaseName() + ".xml";
    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("annotation");
    xml.writeTextElement("filename", info.fileName());
    xml.writeStartElement("size");
    xml.writeTextElement("width", QString::number(imgW));
    xml.writeTextElement("height", QString::number(imgH));
    xml.writeTextElement("depth", "3");
    xml.writeEndElement();

    for (const auto &p : polygons) {
        if (p.points.size() < 3) continue;

        qreal minX = p.points[0].x(), maxX = minX;
        qreal minY = p.points[0].y(), maxY = minY;
        for (const auto &pt : p.points) {
            minX = qMin(minX, pt.x()); maxX = qMax(maxX, pt.x());
            minY = qMin(minY, pt.y()); maxY = qMax(maxY, pt.y());
        }

        xml.writeStartElement("object");
        xml.writeTextElement("name", QString::number(p.classId));
        xml.writeStartElement("bndbox");
        xml.writeTextElement("xmin", QString::number(int(minX)));
        xml.writeTextElement("ymin", QString::number(int(minY)));
        xml.writeTextElement("xmax", QString::number(int(maxX)));
        xml.writeTextElement("ymax", QString::number(int(maxY)));
        xml.writeEndElement();
        xml.writeStartElement("polygon");
        for (int i = 0; i < p.points.size(); ++i) {
            xml.writeTextElement(QString("x%1").arg(i+1), QString::number(p.points[i].x()));
            xml.writeTextElement(QString("y%1").arg(i+1), QString::number(p.points[i].y()));
        }
        xml.writeEndElement();
        xml.writeEndElement();
    }
    xml.writeEndElement();
    return true;
}

bool LabelExporter::exportCocoDetection(const QString &jsonPath,
                                        const QStringList &imgNames,
                                        const QList<QPair<int,int>> &imgSizes,
                                        const QList<QList<AnnoRect>> &allRects,
                                        const QStringList &classes)
{
    QJsonObject coco;
    QJsonArray cats;
    for (int i = 0; i < classes.size(); ++i) {
        QJsonObject cat;
        cat["id"] = i + 1;
        cat["name"] = classes[i];
        cat["supercategory"] = "";
        cats.append(cat);
    }
    coco["categories"] = cats;

    QJsonArray images;
    QJsonArray annotations;
    int annId = 0;

    for (int imgIdx = 0; imgIdx < imgNames.size(); ++imgIdx) {
        QJsonObject img = makeCocoImage(imgIdx, imgNames[imgIdx],
                                        imgSizes[imgIdx].first, imgSizes[imgIdx].second);
        images.append(img);

        const auto &rects = allRects[imgIdx];
        for (const auto &r : rects) {
            if (r.classId < 0 || r.classId >= classes.size()) continue;
            QJsonObject ann;
            ann["id"] = annId++;
            ann["image_id"] = imgIdx;
            ann["category_id"] = r.classId + 1;
            double x = r.bbox.left(), y = r.bbox.top();
            double w = r.bbox.width(), h = r.bbox.height();
            ann["bbox"] = QJsonArray{x, y, w, h};
            ann["area"] = w * h;
            ann["iscrowd"] = 0;
            annotations.append(ann);
        }
    }
    coco["images"] = images;
    coco["annotations"] = annotations;

    QFile file(jsonPath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(coco).toJson());
    return true;
}

bool LabelExporter::exportCocoSegmentation(const QString &jsonPath,
                                           const QStringList &imgNames,
                                           const QList<QPair<int,int>> &imgSizes,
                                           const QList<QList<AnnoPolygon>> &allPolygons,
                                           const QStringList &classes)
{
    QJsonObject coco;
    QJsonArray cats;
    for (int i = 0; i < classes.size(); ++i) {
        QJsonObject cat;
        cat["id"] = i;
        cat["name"] = classes[i];
        cat["supercategory"] = "";
        cats.append(cat);
    }
    coco["categories"] = cats;

    QJsonArray images;
    QJsonArray annotations;
    int annId = 0;

    for (int imgIdx = 0; imgIdx < imgNames.size(); ++imgIdx) {
        QJsonObject img = makeCocoImage(imgIdx, imgNames[imgIdx],
                                        imgSizes[imgIdx].first, imgSizes[imgIdx].second);
        images.append(img);

        const auto &polygons = allPolygons[imgIdx];
        for (const auto &p : polygons) {
            if (p.classId < 0 || p.classId >= classes.size()) continue;
            if (p.points.size() < 3) continue;

            QJsonArray seg;
            QJsonArray flat;
            for (const auto &pt : p.points) {
                flat.append(pt.x());
                flat.append(pt.y());
            }
            seg.append(flat);

            qreal minX = p.points[0].x(), maxX = minX;
            qreal minY = p.points[0].y(), maxY = minY;
            for (const auto &pt : p.points) {
                minX = qMin(minX, pt.x()); maxX = qMax(maxX, pt.x());
                minY = qMin(minY, pt.y()); maxY = qMax(maxY, pt.y());
            }

            QJsonObject ann;
            ann["id"] = annId++;
            ann["image_id"] = imgIdx;
            ann["category_id"] = p.classId;
            ann["segmentation"] = seg;
            ann["area"] = polygonArea(p.points);
            ann["bbox"] = QJsonArray{minX, minY, maxX - minX, maxY - minY};
            ann["iscrowd"] = 0;
            annotations.append(ann);
        }
    }
    coco["images"] = images;
    coco["annotations"] = annotations;

    QFile file(jsonPath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(coco).toJson());
    return true;
}

double LabelExporter::polygonArea(const QVector<QPointF> &pts)
{
    double area = 0.0;
    int n = pts.size();
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        area += pts[i].x() * pts[j].y();
        area -= pts[j].x() * pts[i].y();
    }
    return std::abs(area) / 2.0;
}

QJsonObject LabelExporter::makeCocoImage(int id, const QString &fileName, int w, int h)
{
    QJsonObject img;
    img["id"] = id;
    img["file_name"] = QFileInfo(fileName).fileName();
    img["width"] = w;
    img["height"] = h;
    return img;
}
