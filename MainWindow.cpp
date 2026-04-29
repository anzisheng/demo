#include "MainWindow.h"
#include "GLWidget.h"
#include "FountainDesigner.h"
#include <QDockWidget>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    GLWidget* glWidget = new GLWidget(this);
    FountainDesigner* designer = new FountainDesigner(glWidget, this);

    QDockWidget* dock = new QDockWidget("spring designer", this);
    dock->setWidget(designer);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    setCentralWidget(glWidget);
    connect(designer, &FountainDesigner::fountainDataChanged,
        glWidget, &GLWidget::updateFountainsFromData);

    resize(1400, 800);
    setWindowTitle("水幕喷泉系统 - 音乐同步 + 水帘 + 地面喷泉");
}