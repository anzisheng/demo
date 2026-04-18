#include "FountainDesigner.h"
#include "FountainFile.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <cmath>

FountainDesigner::FountainDesigner(GLWidget* glWidget, QWidget* parent)
    : QWidget(parent)
    , m_glWidget(glWidget)
    , m_currentRow(-1)
{
    setupUI();
}

void FountainDesigner::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ========== 工具栏 ==========
    QHBoxLayout* toolBar = new QHBoxLayout();

    QPushButton* addBtn = new QPushButton("+ 添加喷泉");
    QPushButton* removeBtn = new QPushButton("- 删除");
    QPushButton* clearBtn = new QPushButton("清空全部");
    QPushButton* loadBtn = new QPushButton("加载文件");
    QPushButton* saveBtn = new QPushButton("保存文件");
    QPushButton* applyBtn = new QPushButton("应用到场景");

    toolBar->addWidget(addBtn);
    toolBar->addWidget(removeBtn);
    toolBar->addWidget(clearBtn);
    toolBar->addWidget(loadBtn);
    toolBar->addWidget(saveBtn);
    toolBar->addWidget(applyBtn);

    connect(addBtn, &QPushButton::clicked, this, &FountainDesigner::onAddFountain);
    connect(removeBtn, &QPushButton::clicked, this, &FountainDesigner::onRemoveFountain);
    connect(clearBtn, &QPushButton::clicked, this, &FountainDesigner::onClearAll);
    connect(loadBtn, &QPushButton::clicked, this, &FountainDesigner::onLoadFile);
    connect(saveBtn, &QPushButton::clicked, this, &FountainDesigner::onSaveFile);
    connect(applyBtn, &QPushButton::clicked, this, &FountainDesigner::onApplyToScene);

    mainLayout->addLayout(toolBar);

    // ========== 批量生成工具 ==========
    QGroupBox* batchGroup = new QGroupBox("批量生成");
    QHBoxLayout* batchLayout = new QHBoxLayout();

    QPushButton* archBtn = new QPushButton("拱形排列");
    QPushButton* circleBtn = new QPushButton("圆形排列");
    QPushButton* lineBtn = new QPushButton("直线排列");
    QPushButton* gridBtn = new QPushButton("网格排列");

    m_countSpin = new QSpinBox();
    m_countSpin->setRange(1, 200);
    m_countSpin->setValue(20);
    m_spacingSpin = new QDoubleSpinBox();
    m_spacingSpin->setRange(0.2, 2.0);
    m_spacingSpin->setValue(0.5);
    m_archHeightSpin = new QDoubleSpinBox();
    m_archHeightSpin->setRange(0, 5);
    m_archHeightSpin->setValue(2.0);
    m_radiusSpin = new QDoubleSpinBox();
    m_radiusSpin->setRange(1, 10);
    m_radiusSpin->setValue(5.0);

    batchLayout->addWidget(archBtn);
    batchLayout->addWidget(circleBtn);
    batchLayout->addWidget(lineBtn);
    batchLayout->addWidget(gridBtn);
    batchLayout->addWidget(new QLabel("个数:"));
    batchLayout->addWidget(m_countSpin);
    batchLayout->addWidget(new QLabel("间距:"));
    batchLayout->addWidget(m_spacingSpin);
    batchLayout->addWidget(new QLabel("拱高:"));
    batchLayout->addWidget(m_archHeightSpin);
    batchLayout->addWidget(new QLabel("半径:"));
    batchLayout->addWidget(m_radiusSpin);

    batchGroup->setLayout(batchLayout);
    mainLayout->addWidget(batchGroup);

    connect(archBtn, &QPushButton::clicked, this, &FountainDesigner::onGenerateArch);
    connect(circleBtn, &QPushButton::clicked, this, &FountainDesigner::onGenerateCircle);
    connect(lineBtn, &QPushButton::clicked, this, &FountainDesigner::onGenerateLine);
    connect(gridBtn, &QPushButton::clicked, this, &FountainDesigner::onGenerateGrid);

    // ========== 喷泉列表 ==========
    m_tableWidget = new QTableWidget();
    m_tableWidget->setColumnCount(6);
    m_tableWidget->setHorizontalHeaderLabels({ "ID", "X", "Y", "Z", "高度", "启用" });
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(m_tableWidget, &QTableWidget::itemSelectionChanged,
        this, &FountainDesigner::onTableSelectionChanged);

    mainLayout->addWidget(m_tableWidget);

    // ========== 属性面板 ==========
    m_propertyPanel = new QWidget();
    QGridLayout* propLayout = new QGridLayout(m_propertyPanel);

    propLayout->addWidget(new QLabel("X:"), 0, 0);
    m_xSpin = new QDoubleSpinBox();
    m_xSpin->setRange(-15, 15);
    m_xSpin->setSingleStep(0.1);

    propLayout->addWidget(new QLabel("Y:"), 0, 2);
    m_ySpin = new QDoubleSpinBox();
    m_ySpin->setRange(0, 10);
    m_ySpin->setSingleStep(0.1);

    propLayout->addWidget(new QLabel("Z:"), 0, 4);
    m_zSpin = new QDoubleSpinBox();
    m_zSpin->setRange(-10, 10);
    m_zSpin->setSingleStep(0.1);

    propLayout->addWidget(new QLabel("高度:"), 1, 0);
    m_heightSpin = new QDoubleSpinBox();
    m_heightSpin->setRange(0.5, 5);
    m_heightSpin->setSingleStep(0.1);

    propLayout->addWidget(new QLabel("水流量:"), 1, 2);
    m_flowSpin = new QDoubleSpinBox();
    m_flowSpin->setRange(0, 1);
    m_flowSpin->setSingleStep(0.05);

    propLayout->addWidget(new QLabel("喷射角:"), 1, 4);
    m_angleSpin = new QDoubleSpinBox();
    m_angleSpin->setRange(-90, 90);
    m_angleSpin->setSingleStep(5);

    propLayout->addWidget(new QLabel("启用:"), 2, 0);
    m_enabledCheck = new QCheckBox();
    m_enabledCheck->setChecked(true);

    QPushButton* updateBtn = new QPushButton("更新属性");
    propLayout->addWidget(updateBtn, 2, 2, 1, 2);

    connect(updateBtn, &QPushButton::clicked, this, &FountainDesigner::onPropertyChanged);

    mainLayout->addWidget(m_propertyPanel);

    // 初始示例数据
    onGenerateArch();
}

void FountainDesigner::loadFountains(const QVector<FountainData>& fountains)
{
    m_fountains = fountains;
    updateTable();
}

QVector<FountainData> FountainDesigner::getFountains() const
{
    return m_fountains;
}

void FountainDesigner::onAddFountain()
{
    FountainData f;
    f.id = m_fountains.size() + 1;
    f.position = QVector3D(0, 5.5, 0);
    f.height = 2.2;
    f.waterFlow = 0.5;
    f.sprayAngle = -60;
    f.enabled = true;
    m_fountains.append(f);
    updateTable();
}

void FountainDesigner::onRemoveFountain()
{
    if (m_currentRow >= 0 && m_currentRow < m_fountains.size()) {
        m_fountains.removeAt(m_currentRow);
        updateTable();
        if (m_currentRow >= m_fountains.size()) {
            m_currentRow = m_fountains.size() - 1;
        }
        updatePropertyPanel();
    }
}

void FountainDesigner::onClearAll()
{
    m_fountains.clear();
    updateTable();
}

void FountainDesigner::onLoadFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "打开喷泉文件", "", "喷泉文件 (*.fountain);;JSON文件 (*.json)");
    if (!filePath.isEmpty()) {
        if (FountainFile::loadFromFile(filePath, m_fountains)) {
            updateTable();
            QMessageBox::information(this, "成功", "加载了 " + QString::number(m_fountains.size()) + " 个喷泉");
        }
        else {
            QMessageBox::warning(this, "错误", "无法加载文件");
        }
    }
}

void FountainDesigner::onSaveFile()
{
    QString filePath = QFileDialog::getSaveFileName(this, "保存喷泉文件", "", "喷泉文件 (*.fountain)");
    if (!filePath.isEmpty()) {
        if (FountainFile::saveToFile(filePath, m_fountains)) {
            QMessageBox::information(this, "成功", "保存了 " + QString::number(m_fountains.size()) + " 个喷泉");
        }
        else {
            QMessageBox::warning(this, "错误", "无法保存文件");
        }
    }
}

void FountainDesigner::onApplyToScene()
{
    emit fountainDataChanged(m_fountains);
}

void FountainDesigner::onTableSelectionChanged()
{
    QModelIndexList selected = m_tableWidget->selectionModel()->selectedRows();
    if (!selected.isEmpty()) {
        m_currentRow = selected.first().row();
        updatePropertyPanel();
    }
}

void FountainDesigner::onPropertyChanged()
{
    if (m_currentRow >= 0 && m_currentRow < m_fountains.size()) {
        FountainData f = getCurrentFountain();
        m_fountains[m_currentRow] = f;
        updateTable();
        emit fountainDataChanged(m_fountains);
    }
}

void FountainDesigner::onGenerateArch()
{
    m_fountains.clear();

    int count = m_countSpin->value();
    float width = count * m_spacingSpin->value();
    float archHeight = m_archHeightSpin->value();
    float startX = -width / 2.0f;
    float baseY = 5.5f;

    for (int i = 0; i < count; ++i) {
        float x = startX + i * m_spacingSpin->value();

        // 抛物线公式
        float a = -4.0f * archHeight / (width * width);
        float yOffset = a * x * x + archHeight;

        FountainData f;
        f.id = i + 1;
        f.position = QVector3D(x, baseY + yOffset, 0);
        f.height = 2.2;
        f.waterFlow = 0.5;
        f.sprayAngle = -60;
        f.enabled = true;
        m_fountains.append(f);
    }

    updateTable();
    emit fountainDataChanged(m_fountains);
}

void FountainDesigner::onGenerateCircle()
{
    m_fountains.clear();

    int count = m_countSpin->value();
    float radius = m_radiusSpin->value();
    float baseY = 5.5f;

    for (int i = 0; i < count; ++i) {
        float angle = 2 * M_PI * i / count;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;

        FountainData f;
        f.id = i + 1;
        f.position = QVector3D(x, baseY, z);
        f.height = 2.2;
        f.waterFlow = 0.5;
        f.sprayAngle = -60;
        f.enabled = true;
        m_fountains.append(f);
    }

    updateTable();
    emit fountainDataChanged(m_fountains);
}

void FountainDesigner::onGenerateLine()
{
    m_fountains.clear();

    int count = m_countSpin->value();
    float startX = -count * m_spacingSpin->value() / 2.0f;
    float baseY = 5.5f;

    for (int i = 0; i < count; ++i) {
        float x = startX + i * m_spacingSpin->value();

        FountainData f;
        f.id = i + 1;
        f.position = QVector3D(x, baseY, 0);
        f.height = 2.2;
        f.waterFlow = 0.5;
        f.sprayAngle = -60;
        f.enabled = true;
        m_fountains.append(f);
    }

    updateTable();
    emit fountainDataChanged(m_fountains);
}

void FountainDesigner::onGenerateGrid()
{
    m_fountains.clear();

    int cols = sqrt(m_countSpin->value());
    int rows = cols;
    float spacing = m_spacingSpin->value();
    float startX = -cols * spacing / 2.0f;
    float startZ = -rows * spacing / 2.0f;
    float baseY = 5.5f;
    int id = 1;

    for (int i = 0; i < cols; ++i) {
        for (int j = 0; j < rows; ++j) {
            float x = startX + i * spacing;
            float z = startZ + j * spacing;

            FountainData f;
            f.id = id++;
            f.position = QVector3D(x, baseY, z);
            f.height = 2.2;
            f.waterFlow = 0.5;
            f.sprayAngle = -60;
            f.enabled = true;
            m_fountains.append(f);
        }
    }

    updateTable();
    emit fountainDataChanged(m_fountains);
}

void FountainDesigner::updateTable()
{
    m_tableWidget->setRowCount(m_fountains.size());
    for (int i = 0; i < m_fountains.size(); ++i) {
        const auto& f = m_fountains[i];
        m_tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(f.id)));
        m_tableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(f.position.x(), 'f', 2)));
        m_tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(f.position.y(), 'f', 2)));
        m_tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(f.position.z(), 'f', 2)));
        m_tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(f.height, 'f', 2)));
        m_tableWidget->setItem(i, 5, new QTableWidgetItem(f.enabled ? "是" : "否"));
    }
    m_tableWidget->resizeColumnsToContents();
}

void FountainDesigner::updatePropertyPanel()
{
    if (m_currentRow >= 0 && m_currentRow < m_fountains.size()) {
        const auto& f = m_fountains[m_currentRow];
        m_xSpin->setValue(f.position.x());
        m_ySpin->setValue(f.position.y());
        m_zSpin->setValue(f.position.z());
        m_heightSpin->setValue(f.height);
        m_flowSpin->setValue(f.waterFlow);
        m_angleSpin->setValue(f.sprayAngle);
        m_enabledCheck->setChecked(f.enabled);
        m_propertyPanel->setEnabled(true);
    }
    else {
        m_propertyPanel->setEnabled(false);
    }
}

FountainData FountainDesigner::getCurrentFountain() const
{
    FountainData f;
    f.id = m_currentRow + 1;
    f.position = QVector3D(m_xSpin->value(), m_ySpin->value(), m_zSpin->value());
    f.height = m_heightSpin->value();
    f.waterFlow = m_flowSpin->value();
    f.sprayAngle = m_angleSpin->value();
    f.enabled = m_enabledCheck->isChecked();
    return f;
}