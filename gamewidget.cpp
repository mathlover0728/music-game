#include "gamewidget.h"
#include <QPainter>
#include <QKeyEvent>
#include <QFileInfo>
#include <QMessageBox>
#include <QFile>

// Key mapping for tracks 0-4 (A, D, G, J, L)
static int trackKeys[5] = {Qt::Key_A, Qt::Key_D, Qt::Key_G, Qt::Key_J, Qt::Key_L};

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("background-color: #0f0f1e;");

    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.8);

    connect(m_player, &QMediaPlayer::positionChanged, this, &GameWidget::onMediaPositionChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &GameWidget::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &GameWidget::onPlaybackStateChanged);

    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &GameWidget::onTick);
}

GameWidget::~GameWidget() {}

void GameWidget::loadGame(const QString& musicPath, const QString& chartPath)
{
    stopGame();
    m_score = 0;
    m_combo = 0;
    m_hits = 0;
    m_misses = 0;
    for (int i = 0; i < 5; ++i) {
        m_hitEffects[i] = 0;
        m_missEffects[i] = 0;
        m_holdingNoteIndex[i] = -1;
        m_holdingStartTime[i] = 0;
    }
    m_keysPressed.clear();
    m_keysJustPressed.clear();

    m_chart.clear();
    m_chart.musicFilePath = musicPath;

    // Load chart if exists
    if (!chartPath.isEmpty() && QFile::exists(chartPath)) {
        m_chart.loadFromFile(chartPath);
        m_chart.sortNotes();
    } else {
        // Generate a simple auto-chart for demo if no chart exists
        // This allows playing even without manual charting
        QMessageBox::information(this, "No Chart Found",
            "No chart file found for this song. A simple auto-generated chart will be used.\n"
            "Use the Chart Editor to create a proper beatmap.");
        generateAutoChart();
        m_chart.sortNotes();
    }

    m_player->setSource(QUrl::fromLocalFile(musicPath));
}

void GameWidget::generateAutoChart()
{
    // Simple auto chart generation based on regular beats
    // 4 tracks, notes every ~800ms for demo purposes
    m_chart.notes.clear();
    for (int i = 0; i < 60; ++i) {
        int track = i % 5;
        qint64 t = 2000 + i * 800LL;
        bool isLong = (i % 7 == 0);
        qint64 dur = isLong ? 600LL : 0LL;
        NoteColor col = (i % 5 == 0) ? NoteColor::Yellow : NoteColor::Blue;
        m_chart.notes.append(Note(track, col, t, dur));
    }
}

void GameWidget::startGame()
{
    m_gameActive = true;
    m_player->play();
    m_timer->start();
    setFocus();
}

void GameWidget::stopGame()
{
    m_gameActive = false;
    m_timer->stop();
    m_player->stop();
}

void GameWidget::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::LoadedMedia) {
        startGame();
    }
}

void GameWidget::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::StoppedState && m_gameActive) {
        // Song ended
        m_gameActive = false;
        m_timer->stop();
        QMessageBox::information(this, "Game Over",
            QString("Song Finished!\nScore: %1\nHits: %2\nMisses: %3")
                .arg(m_score).arg(m_hits).arg(m_misses));
        emit returnToMenu();
    }
}

void GameWidget::onMediaPositionChanged(qint64 position)
{
    m_lastPositionMs = position;
}

void GameWidget::onTick()
{
    // Update position based on player if available, else increment
    qint64 pos = m_player->position();
    if (pos > 0) {
        m_lastPositionMs = pos;
    }

    checkHits();
    update();
}

void GameWidget::checkHits()
{
    qint64 now = m_lastPositionMs;
    if (now <= 0) return;
    for (int i = 0; i < m_chart.notes.size(); ++i) {
        Note& note = m_chart.notes[i];
        if (note.hit || note.missed) continue;
        qint64 now = m_lastPositionMs;
        qint64 diff = note.startTimeMs - now;
        int track = note.track;
        if (note.isShortPress()) {
            if (diff < -JUDGE_WINDOW_MS) {
                note.missed = true;
                m_combo = 0;
                m_misses++;
                m_holdingNoteIndex[track] = -1;
                triggerMissEffect(track);
                continue;
            }
            bool keyMatch = false;
            if (note.color == NoteColor::Blue) {
                keyMatch = m_keysJustPressed.contains(trackKeys[track]);
            }
            else {
                keyMatch = m_keysJustPressed.contains(Qt::Key_Space);
            }

            if (keyMatch && diff >= -JUDGE_WINDOW_MS && diff <= JUDGE_WINDOW_MS) {
                note.hit = true;
                m_score += 10;
                m_combo++;
                m_hits++;
                triggerHitEffect(track);
            }
            continue;
        }
        qint64 endTime = note.startTimeMs + note.durationMs;
        qint64 endDiff = endTime - now;
        if (m_holdingNoteIndex[track] == -1) {
            if (diff < -JUDGE_WINDOW_MS) {
                note.missed = true;
                m_combo = 0;
                m_misses++;
                triggerMissEffect(track);
                continue;
            }

            bool keyJustPressed = false;
            if (note.color == NoteColor::Blue) {
                keyJustPressed = m_keysJustPressed.contains(trackKeys[track]);
            } else {
                keyJustPressed = m_keysJustPressed.contains(Qt::Key_Space);
            }

            if (keyJustPressed && diff >= -JUDGE_WINDOW_MS && diff <= JUDGE_WINDOW_MS) {
                m_holdingNoteIndex[track] = i;
                m_holdingStartTime[track] = now;
            }
            continue;
        }
        if (m_holdingNoteIndex[track] == i) {
            bool keyHeld = false;
            if (note.color == NoteColor::Blue) {
                keyHeld = m_keysPressed.contains(trackKeys[track]);
            } else {
                keyHeld = m_keysPressed.contains(Qt::Key_Space);
            }

            if (endDiff < -JUDGE_WINDOW_MS) {
                note.missed = true;
                m_combo = 0;
                m_misses++;
                m_holdingNoteIndex[track] = -1;
                triggerMissEffect(track);
                continue;
            }

            if (!keyHeld && endDiff > JUDGE_WINDOW_MS) {
                note.missed = true;
                m_combo = 0;
                m_misses++;
                m_holdingNoteIndex[track] = -1;
                triggerMissEffect(track);
                continue;
            }

            if (keyHeld && endDiff >= -JUDGE_WINDOW_MS && endDiff <= JUDGE_WINDOW_MS) {
                note.hit = true;
                m_score += 10;
                m_combo++;
                m_hits++;
                m_holdingNoteIndex[track] = -1;
                triggerHitEffect(track);
            }
            continue;
        }
    }
    // Clear just-pressed keys so each press only hits one note
    m_keysJustPressed.clear();
}

void GameWidget::triggerHitEffect(int track)
{
    if (track >= 0 && track < 5) m_hitEffects[track] = 10;
}

void GameWidget::triggerMissEffect(int track)
{
    if (track >= 0 && track < 5) m_missEffects[track] = 10;
}

// Geometry helpers
int GameWidget::trackX(int track) const
{
    int w = width();
    int tw = trackWidth();
    int totalTracksWidth = 5 * tw;
    int startX = (w - totalTracksWidth) / 2;
    return startX + track * tw;
}

int GameWidget::trackWidth() const
{
    return qMin(120, width() / 6);
}

int GameWidget::judgeLineY() const
{
    return height() - 120;
}

// Events
void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    drawPianoBackground(p);
    drawTracks(p);
    drawJudgeLine(p);
    drawNotes(p);
    drawHitEffects(p);
    drawUI(p);
}

void GameWidget::drawPianoBackground(QPainter& p)
{
    // Dark piano-themed background
    p.fillRect(rect(), QColor("#0f0f1e"));

    int keyW = width() / 28;
    int keyH = height() / 5;
    int bottomY = height() - keyH;

    // Draw piano keys at bottom as decorative background
    for (int i = 0; i < 28; ++i) {
        bool isBlack = (i % 7 == 2 || i % 7 == 5 || i % 7 == 0) && i > 0;
        QRect key(i * keyW, bottomY, keyW - 1, keyH);
        if (isBlack) {
            p.fillRect(key, QColor("#1a1a2e"));
        } else {
            p.fillRect(key, QColor("#222244"));
            p.setPen(QColor("#333366"));
            p.drawRect(key);
        }
    }

    // Subtle gradient overlay
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, QColor(15, 15, 30, 200));
    grad.setColorAt(0.7, QColor(15, 15, 30, 50));
    grad.setColorAt(1, QColor(15, 15, 30, 180));
    p.fillRect(rect(), grad);
}

void GameWidget::drawTracks(QPainter& p)
{
    int jy = judgeLineY();
    int tw = trackWidth();

    for (int i = 0; i < 5; ++i) {
        int x = trackX(i);
        QRect trackRect(x, 0, tw, height());

        // Track background - translucent blue
        QColor trackColor(30, 100, 200, 40);
        if (m_keysPressed.contains(trackKeys[i])) {
            trackColor = QColor(60, 160, 255, 100);
        }
        p.fillRect(trackRect, trackColor);

        // Track border
        p.setPen(QColor(50, 120, 220, 120));
        p.drawLine(x, 0, x, height());
        p.drawLine(x + tw, 0, x + tw, height());

        // Track key label at bottom
        QString labels[5] = {"A", "D", "G", "J", "L"};
        p.setPen(QColor(180, 200, 255, 180));
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(QRect(x, jy + 15, tw, 30), Qt::AlignCenter, labels[i]);
    }
}

void GameWidget::drawJudgeLine(QPainter& p)
{
    int jy = judgeLineY();
    int tw = trackWidth();
    int startX = trackX(0);
    int endX = trackX(4) + tw;

    // White judgment line with glow
    QPen pen(QColor(255, 255, 255, 230));
    pen.setWidth(3);
    p.setPen(pen);
    p.drawLine(startX, jy, endX, jy);

    // Glow effect
    p.setPen(QColor(255, 255, 255, 60));
    p.drawLine(startX, jy - 2, endX, jy - 2);
    p.drawLine(startX, jy + 2, endX, jy + 2);
}

void GameWidget::drawNotes(QPainter& p)
{
    qint64 now = m_lastPositionMs;
    if (now <= 0) return;

    int jy = judgeLineY();
    int tw = trackWidth();

    for (const auto& note : m_chart.notes) {
        if (note.hit) continue; // Hidden when hit

        // Calculate note Y position based on time difference
        qreal timeDiff = note.startTimeMs - now; // positive = above judgment line
        qreal noteY = jy - timeDiff * PIXELS_PER_MS;

        int x = trackX(note.track);
        int noteW = tw - 10;
        int noteX = x + 5;

        if (note.isShortPress()) {
            int noteH = 24;
            QRect noteRect(noteX, static_cast<int>(noteY - noteH/2), noteW, noteH);

            // Skip if off-screen
            if (noteRect.bottom() < 0 || noteRect.top() > height())
                continue;

            QColor color = (note.color == NoteColor::Blue) ? QColor(52, 152, 219) : QColor(241, 196, 15);
            if (note.missed) color = QColor(120, 120, 120, 180);

            QLinearGradient grad(noteX, noteRect.top(), noteX, noteRect.bottom());
            grad.setColorAt(0, color.lighter(120));
            grad.setColorAt(1, color);
            p.setBrush(grad);
            p.setPen(QColor(255, 255, 255, 160));
            p.drawRoundedRect(noteRect, 6, 6);
        } else {
            // Long note: draw as elongated bar
            qint64 tailTime = note.startTimeMs + note.durationMs;
            qreal tailY = jy - (tailTime - now) * PIXELS_PER_MS;
            qreal headY = noteY;

            int topY = static_cast<int>(qMin(headY, tailY));
            int bottomY = static_cast<int>(qMax(headY, tailY));
            int noteH = bottomY - topY;
            if (noteH < 20) noteH = 20;

            QRect noteRect(noteX, topY, noteW, noteH);
            if (noteRect.bottom() < 0 || noteRect.top() > height())
                continue;

            QColor color = (note.color == NoteColor::Blue) ? QColor(52, 152, 219) : QColor(241, 196, 15);
            if (note.missed) color = QColor(120, 120, 120, 180);

            // Body gradient
            QLinearGradient grad(noteX, topY, noteX, bottomY);
            grad.setColorAt(0, color.lighter(130));
            grad.setColorAt(0.5, color);
            grad.setColorAt(1, color.darker(110));
            p.setBrush(grad);
            p.setPen(QColor(255, 255, 255, 120));
            p.drawRoundedRect(noteRect, 8, 8);

            // Head highlight
            p.setPen(QPen(QColor(255, 255, 255, 200), 2));
            p.drawLine(noteX + 4, static_cast<int>(headY), noteX + noteW - 4, static_cast<int>(headY));
        }
    }
}

void GameWidget::drawHitEffects(QPainter& p)
{
    int jy = judgeLineY();
    int tw = trackWidth();

    for (int i = 0; i < 5; ++i) {
        int x = trackX(i);
        if (m_hitEffects[i] > 0) {
            qreal alpha = m_hitEffects[i] / 10.0;
            QColor color(100, 200, 255, static_cast<int>(200 * alpha));
            p.setPen(QPen(color, 3));
            p.setBrush(Qt::NoBrush);
            int radius = static_cast<int>((10 - m_hitEffects[i]) * 4);
            p.drawEllipse(QPoint(x + tw/2, jy), radius, radius/2);
        }
        if (m_missEffects[i] > 0) {
            qreal alpha = m_missEffects[i] / 10.0;
            QColor color(255, 80, 80, static_cast<int>(200 * alpha));
            p.setPen(QPen(color, 2));
            int offset = (10 - m_missEffects[i]) * 3;
            p.drawLine(x + 10 + offset, jy - 10, x + tw - 10 - offset, jy + 10);
            p.drawLine(x + tw - 10 - offset, jy - 10, x + 10 + offset, jy + 10);
        }
    }
}

void GameWidget::drawUI(QPainter& p)
{
    // Score panel
    QRect panel(20, 20, 180, 100);
    p.fillRect(panel, QColor(0, 0, 0, 160));
    p.setPen(QColor(255, 255, 255, 60));
    p.drawRoundedRect(panel, 8, 8);

    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 11));
    p.drawText(QRect(30, 28, 160, 22), Qt::AlignLeft, QString("Score: %1").arg(m_score));
    p.drawText(QRect(30, 52, 160, 22), Qt::AlignLeft, QString("Combo: %1").arg(m_combo));
    p.drawText(QRect(30, 76, 160, 22), Qt::AlignLeft, QString("Hits/Misses: %1/%2").arg(m_hits).arg(m_misses));

    // Space key reminder for yellow notes
    p.setPen(QColor(241, 196, 15, 200));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(QRect(width() - 200, 30, 180, 30), Qt::AlignRight, "SPACE = Yellow Notes");

    // ESC hint
    p.setPen(QColor(180, 180, 200, 150));
    p.setFont(QFont("Arial", 10));
    p.drawText(QRect(width() - 200, 60, 180, 20), Qt::AlignRight, "Press ESC to quit");
}

void GameWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        stopGame();
        emit returnToMenu();
        return;
    }

    if (event->isAutoRepeat()) return;

    if (!m_gameActive) return;

    int key = event->key();
    if (key == Qt::Key_Space || key == Qt::Key_A || key == Qt::Key_D ||
        key == Qt::Key_G || key == Qt::Key_J || key == Qt::Key_L) {
        m_keysPressed.insert(key);
        m_keysJustPressed.insert(key);
    }
}

void GameWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;
    m_keysPressed.remove(event->key());
}

void GameWidget::resizeEvent(QResizeEvent *)
{
    update();
}
