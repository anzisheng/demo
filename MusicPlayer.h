#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QVector>
#include <QTimer>
#include <QFileInfo>

class MusicPlayer : public QObject
{
    Q_OBJECT
public:
    static MusicPlayer& getInstance();

    bool loadMusic(const QString& filePath);
    void play();
    void pause();
    void stop();
    bool isPlaying() const;
    void setVolume(int volume);
    int getVolume() const;
    QString getCurrentMusicName() const;

    // 获取音频特征
    float getCurrentAmplitude() const { return m_currentAmplitude; }
    float getBeatStrength() const { return m_beatStrength; }
    float getBassLevel() const { return m_bassLevel; }

signals:
    void musicLoaded(const QString& name);
    void playStateChanged(bool playing);
    void beatDetected();
    void amplitudeChanged(float amplitude);

private:
    MusicPlayer();
    ~MusicPlayer();

    void updateAudioFeatures();

    QMediaPlayer* m_player = nullptr;
    QAudioOutput* m_audioOutput = nullptr;

    float m_currentAmplitude = 0.0f;
    float m_beatStrength = 0.0f;
    float m_bassLevel = 0.0f;

    QVector<float> m_amplitudeHistory;
    QString m_currentMusicName;
    QTimer* m_updateTimer = nullptr;
};