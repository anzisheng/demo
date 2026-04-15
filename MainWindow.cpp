#include "MainWindow.h"
#include "GLWidget.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setCentralWidget(new GLWidget(this));
    resize(1200, 800);
    setWindowTitle("Particle Fountain - 50 Water Valves");
}