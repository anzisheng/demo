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

    if (obj.contains("waterValveCount")) m_waterValveCount = obj["waterValveCount"].toInt();
    if (obj.contains("waterValveSpacing")) m_waterValveSpacing = obj["waterValveSpacing"].toDouble();
    if (obj.contains("waterValveBaseHeight")) m_waterValveBaseHeight = obj["waterValveBaseHeight"].toDouble();
    if (obj.contains("waterValveSize")) m_waterValveSize = obj["waterValveSize"].toDouble();
    if (obj.contains("valveControlImage")) m_valveControlImage = obj["valveControlImage"].toString();
    if (obj.contains("frameInterval")) m_frameInterval = obj["frameInterval"].toDouble();

    if (obj.contains("dropBurstInterval")) m_dropBurstInterval = obj["dropBurstInterval"].toDouble();
    if (obj.contains("dropMinSize")) m_dropMinSize = obj["dropMinSize"].toDouble();
    if (obj.contains("dropMaxSize")) m_dropMaxSize = obj["dropMaxSize"].toDouble();
    if (obj.contains("dropMinLife")) m_dropMinLife = obj["dropMinLife"].toDouble();
    if (obj.contains("dropMaxLife")) m_dropMaxLife = obj["dropMaxLife"].toDouble();
    if (obj.contains("dropSpeedYMin")) m_dropSpeedYMin = obj["dropSpeedYMin"].toDouble();
    if (obj.contains("dropSpeedYMax")) m_dropSpeedYMax = obj["dropSpeedYMax"].toDouble();
    if (obj.contains("gravity")) m_gravity = obj["gravity"].toDouble();

    if (obj.contains("poolWidth")) m_poolWidth = obj["poolWidth"].toDouble();
    if (obj.contains("poolDepth")) m_poolDepth = obj["poolDepth"].toDouble();
    if (obj.contains("waterColor")) {
        QJsonArray arr = obj["waterColor"].toArray();
        if (arr.size() >= 3)
            m_waterColor = QVector3D(arr[0].toDouble(), arr[1].toDouble(), arr[2].toDouble());
    }
    if (obj.contains("waterAlpha")) m_waterAlpha = obj["waterAlpha"].toDouble();

    qDebug() << "Config loaded";
    return true;
}

void ConfigManager::saveConfig(const QString& filePath)
{
    QJsonObject obj;
    obj["waterValveCount"] = m_waterValveCount;
    obj["waterValveSpacing"] = m_waterValveSpacing;
    obj["waterValveBaseHeight"] = m_waterValveBaseHeight;
    obj["waterValveSize"] = m_waterValveSize;
    obj["valveControlImage"] = m_valveControlImage;
    obj["frameInterval"] = m_frameInterval;

    obj["dropBurstInterval"] = m_dropBurstInterval;
    obj["dropMinSize"] = m_dropMinSize;
    obj["dropMaxSize"] = m_dropMaxSize;
    obj["dropMinLife"] = m_dropMinLife;
    obj["dropMaxLife"] = m_dropMaxLife;
    obj["dropSpeedYMin"] = m_dropSpeedYMin;
    obj["dropSpeedYMax"] = m_dropSpeedYMax;
    obj["gravity"] = m_gravity;

    obj["poolWidth"] = m_poolWidth;
    obj["poolDepth"] = m_poolDepth;
    QJsonArray colorArr;
    colorArr.append(m_waterColor.x());
    colorArr.append(m_waterColor.y());
    colorArr.append(m_waterColor.z());
    obj["waterColor"] = colorArr;
    obj["waterAlpha"] = m_waterAlpha;

    QJsonDocument doc(obj);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        qDebug() << "Config saved";
    }
}