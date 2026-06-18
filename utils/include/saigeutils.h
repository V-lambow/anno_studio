#ifndef SAIGEUTILS_H
#define SAIGEUTILS_H
#include <QString>
#include <QFileInfo>
#include <QMessageBox>
#include <QDir>
#include "../../saige/include/SaigeVisionUtil.h"
#include "base_head.hpp"
#include "json/json.h"



struct SaigeHandle
{
    SaigeInferenceHandle* inferHdl =nullptr;
    SaigeHandleConfig configHdl{};
    SaigeInferenceResult* resHdl =nullptr;
    SaigeInferenceOption optHdl{};
    SaigeModelInfo modelInfo{};
    ~SaigeHandle(){
        if(inferHdl!=nullptr){
            SaigeDestroyInferenceHandle(*inferHdl);
            inferHdl =nullptr;
        }
        if(resHdl!=nullptr){
            SaigeDestroyInferenceResult(*resHdl);
            resHdl =nullptr;
        }

    }

};
class SaigeUtils
{
public:
    SaigeUtils();
    // 移动文件函数
    // 参数: sourcePath-源文件路径, destPath-目标路径, overwrite-是否覆盖已存在文件
    // 返回值: 操作成功返回true, 失败返回false
    static bool safeMoveFile(const QString &sourcePath, const QString &destPath, bool overwrite = false);


    // 删除文件函数
    // 参数: filePath-要删除的文件路径, needConfirm-是否需要用户确认
    // 返回值: 操作成功返回true, 失败返回false
    static bool safeDeleteFile(const QString &filePath, bool needConfirm = true);

    static bool get_SaigeSegRes(SaigeInferenceResult result, LabelInfo& labelInfo, SaigeModelInfo& modelInfo);
    
    static std::variant<std::monostate,std::string> SaveJson_fromSaigeInfer(SaigeInferenceHandle* inferHdl,SaigeModelInfo modelInfo,DLTASK task,QString imagePath);

    static std::variant<QImage,QString> get_SaigeIENRes(SaigeInferenceResult& result,int w,int h,QImage::Format fmt);
};



#endif // SAIGEUTILS_H
