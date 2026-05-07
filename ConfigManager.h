#pragma once
#include <QString>
#include <QJsonObject>
#include <QVector3D>

class ConfigManager
{
public:
    static ConfigManager& getInstance();
    bool loadConfig(const QString& filePath = "FountainConfig.json");
    void saveConfig(const QString& filePath = "FountainConfig.json");

    int getWaterValveCount() const { return m_waterValveCount; }
    float getWaterValveSpacing() const { return m_waterValveSpacing; }
    float getWaterValveBaseHeight() const { return m_waterValveBaseHeight; }
    float getWaterValveSize() const { return m_waterValveSize; }
    QString getImageFolder() const { return m_imageFolder; }
    float getImageInterval() const { return m_imageInterval; }
    bool getMirrorHorizontally() const { return m_mirrorHorizontally; }
    float getCameraDistanceScale() const { return m_cameraDistanceScale; }

    float getDropBurstInterval() const { return m_dropBurstInterval; }
    float getDropMinSize() const { return m_dropMinSize; }
    float getDropMaxSize() const { return m_dropMaxSize; }
    float getDropMinLife() const { return m_dropMinLife; }
    float getDropMaxLife() const { return m_dropMaxLife; }
    float getDropSpeedYMin() const { return m_dropSpeedYMin; }
    float getDropSpeedYMax() const { return m_dropSpeedYMax; }
    float getGravity() const { return m_gravity; }

    float getPoolWidth() const { return m_poolWidth; }
    float getPoolDepth() const { return m_poolDepth; }
    QVector3D getWaterColor() const { return m_waterColor; }
    float getWaterAlpha() const { return m_waterAlpha; }

private:
    ConfigManager() = default;

    int m_waterValveCount = 400;
    float m_waterValveSpacing = 0.25f;
    float m_waterValveBaseHeight = 12.0f;
    float m_waterValveSize = 0.2f;
    QString m_imageFolder = "./frames/";
    float m_imageInterval = 3.0f;
    bool m_mirrorHorizontally = false;
    float m_cameraDistanceScale = 1.5f;

    float m_dropBurstInterval = 0.03f;
    float m_dropMinSize = 0.05f;
    float m_dropMaxSize = 0.09f;
    float m_dropMinLife = 1.5f;
    float m_dropMaxLife = 2.0f;
    float m_dropSpeedYMin = -14.0f;
    float m_dropSpeedYMax = -11.0f;
    float m_gravity = -9.8f;

    float m_poolWidth = 100.0f;
    float m_poolDepth = 20.0f;
    QVector3D m_waterColor = QVector3D(0.2f, 0.65f, 0.95f);
    float m_waterAlpha = 0.65f;
};