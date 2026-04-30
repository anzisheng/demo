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
    float getWaterValveMaxLength() const { return m_waterValveMaxLength; }
    float getWaterValveSize() const { return m_waterValveSize; }

    float getPoolWidth() const { return m_poolWidth; }
    float getPoolDepth() const { return m_poolDepth; }
    QVector3D getWaterColor() const { return m_waterColor; }
    float getWaterAlpha() const { return m_waterAlpha; }

private:
    ConfigManager() = default;

    int m_waterValveCount = 200;
    float m_waterValveSpacing = 0.25f;
    float m_waterValveBaseHeight = 3.0f;
    float m_waterValveMaxLength = 4.5f;
    float m_waterValveSize = 0.2f;

    float m_poolWidth = 50.0f;
    float m_poolDepth = 8.0f;
    QVector3D m_waterColor = QVector3D(0.2f, 0.65f, 0.95f);
    float m_waterAlpha = 0.65f;
};