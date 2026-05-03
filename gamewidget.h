#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include "chart.h"
#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QSet>

class GameWidget : public QWidget {
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);
    ~GameWidget();

    void loadGame(const QString& musicPath, const QString& chartPath);
    void stopGame();
    void generateAutoChart();

signals:
    void returnToMenu();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onTick();
    void onMediaPositionChanged(qint64 position);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

private:
    void startGame();
    void checkHits();
    void checkMisses();
    void triggerHitEffect(int track);
    void triggerMissEffect(int track);

    // Track geometry
    int trackX(int track) const;
    int trackWidth() const;
    int judgeLineY() const;

    // Drawing helpers
    void drawPianoBackground(QPainter& p);
    void drawTracks(QPainter& p);
    void drawJudgeLine(QPainter& p);
    void drawNotes(QPainter& p);
    void drawUI(QPainter& p);
    void drawHitEffects(QPainter& p);

    QMediaPlayer* m_player = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QTimer* m_timer = nullptr;

    Chart m_chart;
    bool m_gameActive = false;
    qint64 m_score = 0;
    int m_combo = 0;
    int m_hits = 0;
    int m_misses = 0;

    // Key state for long press tracking
    QSet<int> m_keysPressed;
    QSet<int> m_keysJustPressed; // For short note hit detection (one hit per press)
    // Track which note index is currently being held per track (for long notes)
    int m_holdingNoteIndex[5] = {-1, -1, -1, -1, -1};
    qint64 m_holdingStartTime[5] = {0, 0, 0, 0, 0};

    // Hit effects: track -> remaining frames
    int m_hitEffects[5] = {0, 0, 0, 0, 0};
    int m_missEffects[5] = {0, 0, 0, 0, 0};

    // Time tracking
    qint64 m_lastPositionMs = 0;
    static constexpr qint64 JUDGE_WINDOW_MS = 80;
    static constexpr qreal PIXELS_PER_MS = 0.45; // Fall speed
};

#endif // GAMEWIDGET_H
