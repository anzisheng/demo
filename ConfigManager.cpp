#include "ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

ConfigManager& ConfigManager::getInstance()
{
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open config file:" << filePath << ", using default values";
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qDebug() << "Invalid JSON format, using default values";
        return false;
    }

    QJsonObject obj = doc.object();

    // 水阀阵列
    if (obj.contains("waterValveGridWidth")) m_waterValveGridWidth = obj["waterValveGridWidth"].toInt();
    if (obj.contains("waterValveGridHeight")) m_waterValveGridHeight = obj["waterValveGridHeight"].toInt();
    if (obj.contains("waterValveSpacing")) m_waterValveSpacing = obj["waterValveSpacing"].toDouble();
    if (obj.contains("waterValveImagePath")) m_waterValveImagePath = obj["waterValveImagePath"].toString();
    if (obj.contains("waterValveBaseHeight")) m_waterValveBaseHeight = obj["waterValveBaseHeight"].toDouble();
    if (obj.contains("waterValveMaxLength")) m_waterValveMaxLength = obj["waterValveMaxLength"].toDouble();

    // 水帘
    if (obj.contains("curtainWidth")) m_curtainWidth = obj["curtainWidth"].toDouble();
    if (obj.contains("curtainHeight")) m_curtainHeight = obj["curtainHeight"].toDouble();
    if (obj.contains("curtainImagePath")) m_curtainImagePath = obj["curtainImagePath"].toString();
    if (obj.contains("curtainFallDuration")) m_curtainFallDuration = obj["curtainFallDuration"].toDouble();

    // 粒子
    if (obj.contains("spawnRate")) m_spawnRate = obj["spawnRate"].toDouble();
    if (obj.contains("particleMinSize")) m_particleMinSize = obj["particleMinSize"].toDouble();
    if (obj.contains("particleMaxSize")) m_particleMaxSize = obj["particleMaxSize"].toDouble();
    if (obj.contains("particleMinLife")) m_particleMinLife = obj["particleMinLife"].toDouble();
    if (obj.contains("particleMaxLife")) m_particleMaxLife = obj["particleMaxLife"].toDouble();
    if (obj.contains("particleSpeedX")) m_particleSpeedX = obj["particleSpeedX"].toDouble();
    if (obj.contains("particleSpeedYMin")) m_particleSpeedYMin = obj["particleSpeedYMin"].toDouble();
    if (obj.contains("particleSpeedYMax")) m_particleSpeedYMax = obj["particleSpeedYMax"].toDouble();
    if (obj.contains("particleSpeedZ")) m_particleSpeedZ = obj["particleSpeedZ"].toDouble();
    if (obj.contains("gravity")) m_gravity = obj["gravity"].toDouble();

    // 水池
    if (obj.contains("poolWidth")) m_poolWidth = obj["poolWidth"].toDouble();
    if (obj.contains("poolDepth")) m_poolDepth = obj["poolDepth"].toDouble();
    if (obj.contains("waterColor")) {
        QJsonArray arr = obj["waterColor"].toArray();
        if (arr.size() >= 3)
            m_waterColor = QVector3D(arr[0].toDouble(), arr[1].toDouble(), arr[2].toDouble());
    }
    if (obj.contains("waterAlpha")) m_waterAlpha = obj["waterAlpha"].toDouble();

    // 风
    if (obj.contains("windStrength")) m_windStrength = obj["windStrength"].toDouble();
    if (obj.contains("windDirection")) m_windDirection = obj["windDirection"].toDouble();

    qDebug() << "Config loaded successfully";
    return true;
}

void ConfigManager::saveConfig(const QString& filePath)
{
    QJsonObject obj;
    obj["waterValveGridWidth"] = m_waterValveGridWidth;
    obj["waterValveGridHeight"] = m_waterValveGridHeight;
    obj["waterValveSpacing"] = m_waterValveSpacing;
    obj["waterValveImagePath"] = m_waterValveImagePath;
    obj["waterValveBaseHeight"] = m_waterValveBaseHeight;
    obj["waterValveMaxLength"] = m_waterValveMaxLength;

    obj["curtainWidth"] = m_curtainWidth;
    obj["curtainHeight"] = m_curtainHeight;
    obj["curtainImagePath"] = m_curtainImagePath;
    obj["curtainFallDuration"] = m_curtainFallDuration;

    obj["spawnRate"] = m_spawnRate;
    obj["particleMinSize"] = m_particleMinSize;
    obj["particleMaxSize"] = m_particleMaxSize;
    obj["particleMinLife"] = m_particleMinLife;
    obj["particleMaxLife"] = m_particleMaxLife;
    obj["particleSpeedX"] = m_particleSpeedX;
    obj["particleSpeedYMin"] = m_particleSpeedYMin;
    obj["particleSpeedYMax"] = m_particleSpeedYMax;
    obj["particleSpeedZ"] = m_particleSpeedZ;
    obj["gravity"] = m_gravity;

    obj["poolWidth"] = m_poolWidth;
    obj["poolDepth"] = m_poolDepth;
    QJsonArray colorArr;
    colorArr.append(m_waterColor.x());
    colorArr.append(m_waterColor.y());
    colorArr.append(m_waterColor.z());
    obj["waterColor"] = colorArr;
    obj["waterAlpha"] = m_waterAlpha;

    obj["windStrength"] = m_windStrength;
    obj["windDirection"] = m_windDirection;

    QJsonDocument doc(obj);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        qDebug() << "Config saved to:" << filePath;
    }
}