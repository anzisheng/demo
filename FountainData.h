#pragma once
#include <QVector3D>
#include <QString>

struct FountainData {
    int id;
    QVector3D position;
    float height;
    float waterFlow;      // 水流量 (0-1)
    float sprayAngle;     // 喷射角度
    bool enabled;
    
    FountainData() : id(0), height(2.2f), waterFlow(0.5f), sprayAngle(-60.0f), enabled(true) {}
};