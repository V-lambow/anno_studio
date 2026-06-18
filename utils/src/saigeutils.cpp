#include "saigeutils.h"

SaigeUtils::SaigeUtils() {}
// 移动文件函数
// 参数: sourcePath-源文件路径, destPath-目标路径, overwrite-是否覆盖已存在文件
// 返回值: 操作成功返回true, 失败返回false
bool SaigeUtils::safeMoveFile(const QString &sourcePath, const QString &destPath, bool overwrite)
{
    // 检查源文件是否存在
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists()) {
        QMessageBox::critical(nullptr, "错误", QString("源文件不存在: %1").arg(sourcePath));
        return false;
    }

    // 检查源文件是否可读
    if (!sourceInfo.isReadable()) {
        QMessageBox::critical(nullptr, "错误", QString("没有权限读取文件: %1").arg(sourcePath));
        return false;
    }

    // 构建目标文件完整路径
    QString targetFilePath;
    QFileInfo destInfo(destPath);
    if (destInfo.isDir()) {
        // 如果目标是目录,则保留原文件名
        targetFilePath = destPath + QDir::separator() + sourceInfo.fileName();
    } else {
        targetFilePath = destPath;
    }

    // 检查目标文件是否已存在
    QFile targetFile(targetFilePath);
    if (targetFile.exists()) {
        if (!overwrite) {
            // 如果不允许覆盖,询问用户
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, "确认",
                                          QString("文件 %1 已存在，是否覆盖?").arg(targetFilePath),
                                          QMessageBox::Yes|QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
            // 先删除已存在的文件
            if (!safeDeleteFile(targetFilePath)) {
                QMessageBox::critical(nullptr, "错误", QString("无法删除已存在的文件: %1").arg(targetFilePath));
                return false;
            }
        }
    }

    // 确保目标目录存在
    QDir targetDir(QFileInfo(targetFilePath).path());
    if (!targetDir.exists()) {
        if (!targetDir.mkpath(".")) {
            QMessageBox::critical(nullptr, "错误", QString("无法创建目标目录: %1").arg(targetDir.path()));
            return false;
        }
    }

    // 执行移动操作
    QFile sourceFile(sourcePath);
    if (!sourceFile.rename(targetFilePath)) {
        // 如果rename失败,尝试先复制再删除(跨文件系统时可能需要)
        if (sourceFile.copy(targetFilePath)) {
            if (sourceFile.remove()) {
                return true;
            } else {
                QMessageBox::warning(nullptr, "警告",
                                     QString("文件已复制到目标位置，但原文件无法删除: %1\n错误: %2")
                                         .arg(sourcePath,sourceFile.errorString()));
                return true; // 复制成功视为部分成功
            }
        } else {
            QMessageBox::critical(nullptr, "错误",
                                  QString("无法移动文件: %1\n错误: %2")
                                      .arg(sourcePath,sourceFile.errorString()));
            return false;
        }
    }

    return true;
}


// 删除文件函数
// 参数: filePath-要删除的文件路径, needConfirm-是否需要用户确认
// 返回值: 操作成功返回true, 失败返回false
bool SaigeUtils::safeDeleteFile(const QString &filePath, bool needConfirm)
{
    // 检查文件是否存在
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(nullptr, "警告", QString("文件不存在: %1").arg(filePath));
        return false;
    }

    // 检查是否有权限删除
    if (!fileInfo.isWritable()) {
        QMessageBox::critical(nullptr, "错误", QString("没有权限删除文件: %1").arg(filePath));
        return false;
    }

    // 询问用户确认
    if (needConfirm) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, "确认删除",
                                      QString("确定要删除 %1 吗?\n此操作不可恢复。").arg(filePath),
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return false;
        }
    }

    // 执行删除操作
    QFile file(filePath);
    if (!file.remove()) {
        QMessageBox::critical(nullptr, "错误",
                              QString("删除文件失败: %1\n错误: %2")
                                  .arg(filePath,file.errorString()));
        return false;
    }

    return true;
}



bool SaigeUtils::get_SaigeSegRes(SaigeInferenceResult result, LabelInfo& labelInfo, SaigeModelInfo& modelInfo) {
    // 获取分割对象数量
    int numObjects;
    if (SaigeGetInferenceResultNumOfSegmentedObjects(result, &numObjects) != 0) {
        return false;
    }

    if (numObjects == 0) {
        return true;
    }

    // 获取所有分割对象
    SaigeSegmentedObject* objects = new SaigeSegmentedObject[numObjects];
    if (SaigeGetInferenceResultSegmentedObjects(result, objects) != 0) {
        delete[] objects;
        return false;
    }

    // 处理每个分割对象
    for (int i = 0; i < numObjects; i++) {
        SaigeSegmentedObject& obj = objects[i];


        // 获取类别名称
        const char* className = modelInfo.classNames[obj.classIndex];

        // 提取轮廓点
        std::vector<QPointF> points;
        int pointIndex = 0;

        for (int c = 0; c < obj.numOfContours; c++) {
            for (int p = 0; p < obj.numOfPointsOnContour[c]; p++) {
                double x = obj.pointsOnContour[pointIndex++];
                double y = obj.pointsOnContour[pointIndex++];
                points.emplace_back(x, y);
            }
        }

        // 创建 Labelele 对象并添加到 labelInfo
        char* labelName = new char[strlen(className) + 1];
        strcpy(labelName, className);
        Labelele ele = Labelele(labelName, std::move(points));
        ele.area = obj.area;
        ele.score = obj.score;

        ele.bbox = QRectF(QPointF(obj.boundingBox.center[0] - obj.boundingBox.width / 2.0f, obj.boundingBox.center[1] - obj.boundingBox.height / 2.0f), QSizeF(obj.boundingBox.width, obj.boundingBox.height));

        labelInfo.label.emplace_back(ele);
    }

    delete[] objects;
    return true;
}
std::variant<std::monostate, std::string> SaigeUtils::SaveJson_fromSaigeInfer(SaigeInferenceHandle *inferHdl, SaigeModelInfo modelInfo, DLTASK task, QString imagePath)
{
    SaigeInferenceResult *inferRes = new SaigeInferenceResult();
    LabelInfo labelRes;

    try
    {


            if (SaigeCreateInferenceResult(inferRes))
            {
                return "创建推理结果失败";
            }
            QByteArray imagePathData = imagePath.toUtf8();
            const char *pathTmp = imagePathData.constData();

            if (SaigeInferFromImageFile(*inferHdl, pathTmp, *inferRes))
            {
                return "推理失败：" + imagePath.toStdString();
            }
            // 判断是不是seg
            if (task == static_cast<DLTASK>(kProjectTypeSEG))
            {
                bool ret = SaigeUtils::get_SaigeSegRes(*inferRes, labelRes, modelInfo);
                if (!ret)
                {
                    return "获取分割结果失败：" + imagePath.toStdString();
                }

                //  写入 JSON 文件
                Json::Value root;
                for (auto &label : labelRes.label)
                {
                    Json::Value obj;
                    obj["classname"] = label.labelName;
                    obj["score"] = label.score;
                    obj["area"] = label.area;
                    obj["bbox"] = Json::Value(Json::arrayValue);
                    obj["bbox"].append(label.bbox.left());
                    obj["bbox"].append(label.bbox.top());
                    obj["bbox"].append(label.bbox.width());
                    obj["bbox"].append(label.bbox.height());
                    obj["points"] = Json::Value(Json::arrayValue);
                    for (auto &point : label.pts)
                    {
                        Json::Value pnt;
                        pnt.append(point.x());
                        pnt.append(point.y());
                        obj["points"].append(pnt);
                    }
                    root.append(obj);
                }
                Json::StreamWriterBuilder writerBuilder;
                writerBuilder["indentation"] = "    ";
                std::string jsonStr = Json::writeString(writerBuilder, root);

                // 写入 JSON 文件
                // 从图像路径提取目录和文件名（不含扩展名）
                QFileInfo imageInfo(imagePath);
                QString jsonFilePath = imageInfo.path() + QDir::separator() + imageInfo.baseName() + "_infer.json";

                // 写入JSON文件
                QFile jsonFile(jsonFilePath);
                if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    QTextStream out(&jsonFile);
                    out << QString::fromStdString(jsonStr);
                    jsonFile.close();
                }
            }

            // destory inferRes
            // SaigeDestroyInferenceResult(inferRes);
            labelRes.clear();
        
        return std::monostate();
    }
    catch (const std::exception &e)
    {
        return e.what();
    }
}


std::variant<QImage,QString> SaigeUtils::get_SaigeIENRes(SaigeInferenceResult& result,int w,int h,QImage::Format fmt){

    //判断fmt通道数，初始化imagedata，设定大小为w*h*通道数

    if(fmt!=QImage::Format_Grayscale8){
        return QString("invalid image format!");
    }
    unsigned char ** imagedata =(unsigned char**)malloc(w*sizeof(unsigned char*));
    for(int i=0;i<w;i++){
        imagedata[i]=(unsigned char*)malloc(h*sizeof(unsigned char));
    }

    {
    int ret =SaigeGetInferenceResultEnhancedImage(result,imagedata);
    if(ret!=0)
    {
        return QString("获取推理结果失败");        
    }

    auto image = QImage(imagedata[0], w,h,fmt);
    return image;
    }
}
