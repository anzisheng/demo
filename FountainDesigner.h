#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include "GLWidget.h"
#include "FountainData.h"  // 添加这行

class FountainDesigner : public QWidget
{
    Q_OBJECT
public:
    explicit FountainDesigner(GLWidget* glWidget, QWidget *parent = nullptr);
    
    void loadFountains(const QVector<FountainData>& fountains);
    QVector<FountainData> getFountains() const;
    
signals:
    void fountainDataChanged(const QVector<FountainData>& fountains);
    
private slots:
    void onAddFountain();
    void onRemoveFountain();
    void onClearAll();
    void onLoadFile();
    void onSaveFile();
    void onApplyToScene();
    void onTableSelectionChanged();
    void onPropertyChanged();
    void onGenerateArch();
    void onGenerateCircle();
    void onGenerateLine();
    void onGenerateGrid();
    
private:
    void setupUI();
    void updateTable();
    void updatePropertyPanel();
    FountainData getCurrentFountain() const;
    void setCurrentFountain(const FountainData& fountain);
    
    GLWidget* m_glWidget;
    QTableWidget* m_tableWidget;
    QWidget* m_propertyPanel;
    
    // 属性控件
    QDoubleSpinBox* m_xSpin;
    QDoubleSpinBox* m_ySpin;
    QDoubleSpinBox* m_zSpin;
    QDoubleSpinBox* m_heightSpin;
    QDoubleSpinBox* m_flowSpin;
    QDoubleSpinBox* m_angleSpin;
    QCheckBox* m_enabledCheck;
    
    // 批量生成控件
    QSpinBox* m_countSpin;
    QDoubleSpinBox* m_spacingSpin;
    QDoubleSpinBox* m_archHeightSpin;
    QDoubleSpinBox* m_radiusSpin;
    
    QVector<FountainData> m_fountains;
    int m_currentRow;
};