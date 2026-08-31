#ifndef BASE_HEAD_HPP
#define BASE_HEAD_HPP
#include <QString>
#include <QRect>
#include <QListWidget>
#include <json/json.h>
enum class CURSOR_MODE
{
    origin = 0,
    pen_brush = 1,
    eraser = 2,
    create_roi = 3,
    create_rect = 4,
};

enum class DLTASK
{
    CLS = 0,
    DET = 1,
    SEG = 2,
    IEN =5
};

struct IMAGEINFO
{
    QString imagePath;
    int height;
    int width;
    void clear()
    {
        imagePath.clear();
        height = 0;
        width = 0;
    }
};

struct InferPara
{
    const char *modelPath;
    int batchSize;
    int height;
    int width;
    DLTASK task;
};
struct Labelele
{
    Labelele() : labelName(nullptr), score(0.0), area(0) {}

    Labelele(char *labelName, std::vector<QPointF> pts) : labelName(std::move(labelName)), pts(std::move(pts))
    {
    }
    void calBoundingBox()
    {
        // 计算点集的包围盒
        if (this->pts.empty())
        {
            bbox = QRectF(); // 空点集对应空矩形
            return;
        }

        // 初始化最小/最大坐标（从第一个点开始）
        qreal minX = this->pts[0].x();
        qreal maxX = this->pts[0].x();
        qreal minY = this->pts[0].y();
        qreal maxY = this->pts[0].y();

        // 遍历所有点更新极值
        for (const auto &point : this->pts)
        {
            minX = qMin(minX, point.x());
            maxX = qMax(maxX, point.x());
            minY = qMin(minY, point.y());
            maxY = qMax(maxY, point.y());
        }
        // 构造包围盒（左上角坐标、宽度、高度）
        bbox = QRectF(minX, minY, maxX - minX, maxY - minY);
    }
    QRectF get_BoundingBox()
    {
        // 如果为空则进行计算
        if (bbox.isNull())
        {
            calBoundingBox();
        }
        return bbox;
    }

    const char *labelName;
    std::vector<QPointF> pts;
    QRectF bbox;
    double score;      // 置信度分数
    unsigned int area; // 面积
};

struct LabelInfo
{
    QString imgName;
    int height;
    int width;
    std::vector<Labelele> label;
    void clear()
    {
        imgName.clear();
        height = 0;
        width = 0;
        label.clear();
    }
};


std::vector<std::string> getListWidgetItemsStrList(
    QListWidget *listWidget,
    const std::string &prefix = "",
    const std::string &suffix = "");

QVector<QString> getListWidgetItemsQStrList(
    QListWidget *listWidget,
    const QString &prefix = "",
    const QString &suffix = "");

std::variant<LabelInfo, QString> getInferLabelInfoFromJson(const Json::Value& root);
#endif // BASE_HEAD_HPP
