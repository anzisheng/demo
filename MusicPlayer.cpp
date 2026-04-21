#include "MusicPlayer.h"
#include <QDebug>
#include <cmath>
#include <QRandomGenerator>

MusicPlayer::MusicPlayer()
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.7f);

    // 定时更新音频特征
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(50);  // 20fps更新
    connect(m_updateTimer, &QTimer::timeout, this, &MusicPlayer::updateAudioFeatures);
    m_updateTimer->start();
}

MusicPlayer::~MusicPlayer()
{
    stop();
}

MusicPlayer& MusicPlayer::getInstance()
{
    static MusicPlayer instance;
    return instance;
}

bool MusicPlayer::loadMusic(const QString& filePath)
{
    m_player->setSource(QUrl::fromLocalFile(filePath));
    m_currentMusicName = QFileInfo(filePath).fileName();

    if (m_player->error() == QMediaPlayer::NoError) {
        emit musicLoaded(m_currentMusicName);
        qDebug() << "Music loaded:" << filePath;
        return true;
    }
    return false;
}

void MusicPlayer::play()
{
    m_player->play();
}

void MusicPlayer::pause()
{
    m_player->pause();
}

void MusicPlayer::stop()
{
    m_player->stop();
}

bool MusicPlayer::isPlaying() const
{
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}

void MusicPlayer::setVolume(int volume)
{
    m_audioOutput->setVolume(volume / 100.0f);
}

int MusicPlayer::getVolume() const
{
    return static_cast<int>(m_audioOutput->volume() * 100);
}

QString MusicPlayer::getCurrentMusicName() const
{
    return m_currentMusicName;
}

void MusicPlayer::updateAudioFeatures()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        // 模拟音乐节拍（实际应用中需要真实音频分析）
        // 这里用正弦波模拟音乐节奏变化
        static float time = 0;
        time += 0.05f;

        // 模拟节拍：每0.5秒一个强拍
        float beat = std::sin(time * 8.0f);
        float newAmplitude = 0.2f + (beat + 1.0f) / 4.0f;

        // 添加随机变化
        newAmplitude += (rand() % 100) / 1000.0f;
        newAmplitude = std::min(0.9f, newAmplitude);

        m_currentAmplitude = newAmplitude;

        // 检测节拍
        m_amplitudeHistory.append(m_currentAmplitude);
        if (m_amplitudeHistory.size() > 10) {
            m_amplitudeHistory.removeFirst();
        }

        float avg = 0;
        for (float a : m_amplitudeHistory) avg += a;
        avg /= m_amplitudeHistory.size();

        if (m_currentAmplitude > avg * 1.3f && m_currentAmplitude > 0.15f) {
            m_beatStrength = std::min(1.0f, m_currentAmplitude / avg);
            emit beatDetected();
        }
        else {
            m_beatStrength *= 0.9f;
        }

        // 低音级别（模拟）
        m_bassLevel = 0.3f + std::sin(time * 4.0f) * 0.2f;

        emit amplitudeChanged(m_currentAmplitude);
    }
    else {
        m_currentAmplitude *= 0.95f;
        m_beatStrength *= 0.95f;
        m_bassLevel *= 0.98f;
    }
}