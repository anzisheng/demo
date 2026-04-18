#include "MainWindow.h"
#include "GLWidget.h"
#include "FountainDesigner.h"
#include <QSplitter>
#include <QDockWidget>
#include <QTimer>  // 添加这行

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    GLWidget* glWidget = new GLWidget(this);
    FountainDesigner* designer = new FountainDesigner(glWidget, this);

    // 创建设计器停靠窗口
    QDockWidget* dock = new QDockWidget("desiger", this);
    dock->setWidget(designer);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    // 设置中央窗口为OpenGL视图
    setCentralWidget(glWidget);

    // 连接信号：当设计器数据变化时更新3D场景
    connect(designer, &FountainDesigner::fountainDataChanged,
        glWidget, &GLWidget::updateFountainsFromData);

    // 延迟创建默认喷泉数据，等待OpenGL初始化完成
    QTimer::singleShot(500, this, [glWidget, designer]() {
        QVector<FountainData> defaultFountains;
        for (int i = -5; i <= 5; ++i) {
            FountainData fd;
            fd.id = i + 6;
            fd.position = QVector3D(i * 0.8f, 5.5f, 0.0f);
            fd.height = 2.2f;
            fd.waterFlow = 0.5f + (i % 3) * 0.2f;
            fd.sprayAngle = -60.0f;
            fd.enabled = true;
            defaultFountains.append(fd);
        }
        designer->loadFountains(defaultFountains);
        glWidget->updateFountainsFromData(defaultFountains);
        });

    resize(1400, 800);
    setWindowTitle("喷泉地图设计器 - 3D喷泉系统");
}