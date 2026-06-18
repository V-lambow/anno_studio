#include "base_head.hpp"



std::vector<std::string> getListWidgetItemsStrList(
    QListWidget* listWidget,
    const std::string& prefix,
    const std::string& suffix)
{
    std::vector<std::string> itemsVector;

    // 检查列表部件是否有效
    if (!listWidget) {
        return itemsVector;
    }

    // 遍历所有项目
    for (int i = 0; i < listWidget->count(); ++i) {
        // 获取当前项
        QListWidgetItem* item = listWidget->item(i);
        if (item) {  // 确保项目有效
            // 组合前缀、项目文本和后缀
            QString combinedText = QString::fromStdString(prefix) +
                                   item->text() +
                                   QString::fromStdString(suffix);
            // 转换为std::string并添加到向量中
            itemsVector.push_back(combinedText.toStdString());
        }
    }
    return itemsVector;
}


QVector<QString> getListWidgetItemsQStrList(
    QListWidget* listWidget,
    const QString& prefix,
    const QString& suffix)
{
    QVector<QString> itemsVector;

    // 检查列表部件是否有效
    if (!listWidget) {
        return itemsVector;
    }

    // 遍历所有项目
    for (int i = 0; i < listWidget->count(); ++i) {
        // 获取当前项
        QListWidgetItem* item = listWidget->item(i);
        if (item) {  // 确保项目有效
            // 组合前缀、项目文本和后缀（直接使用QString操作，避免不必要的转换）
            QString combinedText = prefix + item->text() + suffix;
            // 添加到向量中
            itemsVector.push_back(combinedText);
        }
    }
    return itemsVector;
}
std::variant<LabelInfo, QString> getInferLabelInfoFromJson(const Json::Value& root){
     LabelInfo labelRes;
    
    // 检查root是否为数组
    if (!root.isArray()) {
        return QString("JSON文件中没有'labels'字段"); 

    }
    
    // 遍历JSON数组中的每个对象
    for (const auto& jsonObj : root) {
        Labelele label;
        
        // 解析基本字段
        if (jsonObj.isMember("classname") && jsonObj["classname"].isString()) {
            std::string str = jsonObj["classname"].asString();

            label.labelName =new char[str.size() + 1];
            std::strcpy(const_cast<char*>(label.labelName), str.c_str()); 
            
        }
        
        if (jsonObj.isMember("score") && jsonObj["score"].isNumeric()) {
            label.score = jsonObj["score"].asDouble();
        }
        
        if (jsonObj.isMember("area") && jsonObj["area"].isNumeric()) {
            label.area = jsonObj["area"].asDouble();
        }
        
        // 解析bbox
        if (jsonObj.isMember("bbox") && jsonObj["bbox"].isArray() && jsonObj["bbox"].size() >= 4) {
            QRectF bbox;
            bbox.setLeft(jsonObj["bbox"][0].asDouble());
            bbox.setTop(jsonObj["bbox"][1].asDouble());
            bbox.setWidth(jsonObj["bbox"][2].asDouble());
            bbox.setHeight(jsonObj["bbox"][3].asDouble());
            label.bbox = bbox;
        }
        
        // 解析points数组
        if (jsonObj.isMember("points") && jsonObj["points"].isArray()) {
            for (const auto& jsonPoint : jsonObj["points"]) {
                if (jsonPoint.isArray() && jsonPoint.size() >= 2) {
                    QPointF point;
                    point.setX(jsonPoint[0].asDouble());
                    point.setY(jsonPoint[1].asDouble());
                    label.pts.push_back(point);
                }
            }
        }
        
        labelRes.label.push_back(label);
    }
    
    return labelRes;
}
