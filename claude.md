# AnnoStudio

基于 Qt 的图像标注与推理工具

## 技术栈

- Qt 6.1.0+（兼容 Qt 5）
- C++17
- CMake 3.16+
- jsoncpp 1.9.5
- Halcon 2305
- SaigeVision 2.6.3

## 项目结构

```
├── dlg/                    # 对话框组件
│   ├── dlg_saigeinfer.*    # 赛视推理对话框
│   └── dlg_ultrainfer.*    # Ultra推理对话框
├── halcon/                 # Halcon库文件
│   ├── include/
│   └── lib/
├── MyGraphics/             # 图形视图组件
│   ├── mygraphicsscene.*   # 图形场景
│   ├── mygraphicsview.*    # 图形视图
│   ├── painteditem.*       # 绘制项
│   ├── rectitem.*          # 矩形项
│   ├── inferpattern.*      # 推理模式
│   ├── visibilitymanager.* # 可见性管理
│   └── roioverlayitem.h    # ROI叠加层
├── saige/                  # SaigeVision库
│   ├── include/
│   └── lib/
├── utils/                  # 工具类
│   ├── include/
│   │   ├── base_head.hpp
│   │   ├── saigeutils.h
│   │   └── label_exporter.h
│   └── src/
├── resource/               # 资源文件
├── main.cpp
├── mainwindow.*
└── CMakeLists.txt
```

## 构建命令

```bash
# Windows (MSVC)
cmake -B build -G "Visual Studio 17 2022" -DQt6_DIR="D:/code/Qt/Qt6.8.3/6.8.3/msvc2022_64/lib/cmake/Qt6"
cmake --build build --config Release
```

## 核心功能

- 图像标注：画笔、矩形绘制
- 赛视Seg模型推理（即时/批量）
- 训练集/非训练集管理
- ROI区域标注与管理
- 标注数据导出（YOLO HBB/Seg、VOC Det/Seg、COCO Det/Seg）

## 命名规范

- 类/结构体：大驼峰 `MyGraphicsView`
- 变量/函数：小驼峰 `m_curProjectPath`
- 成员变量：`m_` 前缀（公有）、`_` 后缀（私有）
- Qt信号：`sg_` 前缀
- Qt槽：`sl_` 前缀
- 全局变量：`g_` 前缀
- 常量：`k_` 前缀
- 宏：全大写加下划线
