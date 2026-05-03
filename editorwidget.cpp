#include "editorwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QKeyEvent>
#include <QFileInfo>

EditorWidget::EditorWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setupUI();

    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.6);

    connect(m_player, &QMediaPlayer::positionChanged, this, &EditorWidget::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &EditorWidget::onDurationChanged);

    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &EditorWidget::onTick);
}

EditorWidget::~EditorWidget() {}

void EditorWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Controls bar
    QHBoxLayout* ctrlLayout = new QHBoxLayout();
    ctrlLayout->setSpacing(8);

    m_btnPlay = new QPushButton("▶ Play");
    m_btnPlay->setFixedWidth(80);
    connect(m_btnPlay, &QPushButton::clicked, this, &EditorWidget::onPlayPause);
    ctrlLayout->addWidget(m_btnPlay);

    m_btnStop = new QPushButton("⏹ Stop");
    m_btnStop->setFixedWidth(80);
    connect(m_btnStop, &QPushButton::clicked, this, &EditorWidget::onStop);
    ctrlLayout->addWidget(m_btnStop);

    ctrlLayout->addSpacing(10);

    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(0, 1000);
    m_slider->setEnabled(false);
    connect(m_slider, &QSlider::sliderMoved, this, &EditorWidget::onSliderMoved);
    ctrlLayout->addWidget(m_slider, 1);

    m_lblTime = new QLabel("00:00 / 00:00");
    m_lblTime->setStyleSheet("color: #ccccdd; font-size: 12px;");
    ctrlLayout->addWidget(m_lblTime);

    ctrlLayout->addSpacing(10);

    QLabel* lblColor = new QLabel("Color:");
    lblColor->setStyleSheet("color: #aaaacc;");
    ctrlLayout->addWidget(lblColor);

    m_cmbColor = new QComboBox();
    m_cmbColor->addItem("Blue (Track Key)");
    m_cmbColor->addItem("Yellow (Space)");
    m_cmbColor->setFixedWidth(130);
    ctrlLayout->addWidget(m_cmbColor);

    QLabel* lblDur = new QLabel("Duration(ms):");
    lblDur->setStyleSheet("color: #aaaacc;");
    ctrlLayout->addWidget(lblDur);

    m_spinDuration = new QSpinBox();
    m_spinDuration->setRange(0, 5000);
    m_spinDuration->setSingleStep(50);
    m_spinDuration->setValue(0);
    m_spinDuration->setFixedWidth(70);
    m_spinDuration->setSuffix("ms");
    ctrlLayout->addWidget(m_spinDuration);

    ctrlLayout->addSpacing(10);

    m_btnSave = new QPushButton("💾 Save");
    connect(m_btnSave, &QPushButton::clicked, this, &EditorWidget::onSaveChart);
    ctrlLayout->addWidget(m_btnSave);

    m_btnLoad = new QPushButton("📂 Load");
    connect(m_btnLoad, &QPushButton::clicked, this, &EditorWidget::onLoadChart);
    ctrlLayout->addWidget(m_btnLoad);

    m_btnAuto = new QPushButton("✨ Auto");
    connect(m_btnAuto, &QPushButton::clicked, this, &EditorWidget::onAutoGenerate);
    ctrlLayout->addWidget(m_btnAuto);

    ctrlLayout->addSpacing(10);

    m_btnBack = new QPushButton("← Back");
    connect(m_btnBack, &QPushButton::clicked, this, &EditorWidget::returnToMenu);
    ctrlLayout->addWidget(m_btnBack);

    mainLayout->addLayout(ctrlLayout);

    m_lblStatus = new QLabel("Click on a track to place a note at current time. Use 0ms for short press, >200ms for long press.");
    m_lblStatus->setStyleSheet("color: #7777aa; font-size: 12px;");
    m_lblStatus->setWordWrap(true);
    mainLayout->addWidget(m_lblStatus);

    mainLayout->addStretch();
}

void EditorWidget::loadMusic(const QString& path)
{
    stopEditor();
    m_chart.clear();
    m_chart.musicFilePath = path;
    m_player->setSource(QUrl::fromLocalFile(path));
    m_slider->setEnabled(true);
    m_lblStatus->setText(QString("Loaded: %1. Click tracks to place notes.").arg(QFileInfo(path).fileName()));
}

void EditorWidget::stopEditor()
{
    m_isPlaying = false;
    m_timer->stop();
    m_player->stop();
    m_btnPlay->setText("▶ Play");
}

void EditorWidget::onPlayPause()
{
    if (m_isPlaying) {
        m_player->pause();
        m_isPlaying = false;
        m_timer->stop();
        m_btnPlay->setText("▶ Play");
    } else {
        if (m_player->mediaStatus() == QMediaPlayer::NoMedia) {
            QMessageBox::warning(this, "No Music", "Please import music first.");
            return;
        }
        m_player->play();
        m_isPlaying = true;
        m_timer->start();
        m_btnPlay->setText("⏸ Pause");
    }
}

void EditorWidget::onStop()
{
    stopEditor();
    m_player->setPosition(0);
    onPositionChanged(0);
}

void EditorWidget::onPositionChanged(qint64 position)
{
    if (!m_slider->isSliderDown()) {
        m_slider->setValue(static_cast<int>(position));
    }
    int total = static_cast<int>(m_player->duration());
    m_slider->setMaximum(total > 0 ? total : 1000);

    auto fmt = [](int ms) {
        int sec = ms / 1000;
        int min = sec / 60;
        sec %= 60;
        return QString("%1:%2").arg(min).arg(sec, 2, 10, QChar('0'));
    };
    m_lblTime->setText(QString("%1 / %2").arg(fmt(static_cast<int>(position))).arg(fmt(total)));
    update();
}

void EditorWidget::onDurationChanged(qint64 duration)
{
    m_slider->setMaximum(static_cast<int>(duration));
}

void EditorWidget::onSliderMoved(int value)
{
    m_player->setPosition(value);
}

void EditorWidget::onSaveChart()
{
    if (m_chart.musicFilePath.isEmpty()) {
        QMessageBox::warning(this, "Error", "No music loaded.");
        return;
    }
    QString path = m_chart.suggestedChartPath();
    if (path.isEmpty()) {
        path = QFileDialog::getSaveFileName(this, "Save Chart", QString(), "JSON (*.json)");
    }
    if (!path.isEmpty()) {
        if (m_chart.saveToFile(path)) {
            m_lblStatus->setText(QString("Chart saved to: %1 (%2 notes)").arg(path).arg(m_chart.notes.size()));
            QMessageBox::information(this, "Saved", QString("Chart saved with %1 notes.").arg(m_chart.notes.size()));
        } else {
            QMessageBox::warning(this, "Error", "Failed to save chart.");
        }
    }
}

void EditorWidget::onLoadChart()
{
    QString path = QFileDialog::getOpenFileName(this, "Load Chart", QString(), "JSON (*.json)");
    if (!path.isEmpty()) {
        if (m_chart.loadFromFile(path)) {
            m_chart.sortNotes();
            emit musicLoaded(m_chart.musicFilePath);
            m_player->setSource(QUrl::fromLocalFile(m_chart.musicFilePath));
            m_slider->setEnabled(true);
            m_lblStatus->setText(QString("Loaded chart: %1 notes").arg(m_chart.notes.size()));
            update();
        } else {
            QMessageBox::warning(this, "Error", "Failed to load chart.");
        }
    }
}

void EditorWidget::onAutoGenerate()
{
    if (m_chart.musicFilePath.isEmpty()) {
        QMessageBox::warning(this, "No Music", "Please load music first.");
        return;
    }
    // Generate simple beat pattern for demo
    m_chart.notes.clear();
    qint64 durationMs = m_player->duration();
    if (durationMs <= 0) durationMs = 120000; // 2 min default
    int bpm = 120;
    qint64 beatMs = 60000 / bpm;
    for (qint64 t = 2000; t < durationMs - 2000; t += beatMs) {
        int track = static_cast<int>((t / beatMs) % 5);
        bool isLong = ((t / beatMs) % 7 == 0);
        qint64 dur = isLong ? beatMs * 2 : 0;
        NoteColor col = ((t / beatMs) % 5 == 0) ? NoteColor::Yellow : NoteColor::Blue;
        m_chart.notes.append(Note(track, col, t, dur));
    }
    m_chart.sortNotes();
    m_lblStatus->setText(QString("Auto-generated %1 notes.").arg(m_chart.notes.size()));
    update();
}

void EditorWidget::onTick()
{
    update();
}

// Geometry helpers
int EditorWidget::trackWidth() const
{
    return qMin(120, (width() - 80) / 5);
}

int EditorWidget::trackX(int track) const
{
    int tw = trackWidth();
    int total = 5 * tw;
    int start = (width() - total) / 2;
    return start + track * tw;
}

int EditorWidget::trackAt(int x) const
{
    for (int i = 0; i < 5; ++i) {
        int tx = trackX(i);
        int tw = trackWidth();
        if (x >= tx && x < tx + tw) return i;
    }
    return -1;
}

int EditorWidget::timeToY(qint64 timeMs) const
{
    // Current time at center of view, +/- 3 seconds visible
    qint64 now = m_player->position();
    qint64 diff = timeMs - now; // negative = above center, positive = below
    int centerY = height() / 2;
    return centerY + static_cast<int>(diff * PIXELS_PER_MS);
}

qint64 EditorWidget::yToTime(int y) const
{
    int centerY = height() / 2;
    qint64 now = m_player->position();
    qint64 diff = static_cast<qint64>((y - centerY) / PIXELS_PER_MS);
    return now + diff;
}

void EditorWidget::addNoteAt(int track, qint64 timeMs)
{
    NoteColor col = (m_cmbColor->currentIndex() == 1) ? NoteColor::Yellow : NoteColor::Blue;
    qint64 dur = m_spinDuration->value();
    // Quantize to nearest 10ms
    timeMs = (timeMs / 10) * 10;
    m_chart.notes.append(Note(track, col, timeMs, dur));
    m_chart.sortNotes();
    m_lblStatus->setText(QString("Added note at %1ms on track %2 (%3, %4ms)")
        .arg(timeMs).arg(track).arg(col == NoteColor::Blue ? "Blue" : "Yellow").arg(dur));
    update();
}

// Input
void EditorWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int track = trackAt(event->pos().x());
        if (track >= 0 && track < 5) {
            qint64 timeMs = yToTime(event->pos().y());
            if (timeMs >= 0) {
                addNoteAt(track, timeMs);
            }
        }
    }
}

void EditorWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        stopEditor();
        emit returnToMenu();
        return;
    }
    // Number keys 1-5 to quickly place note at current time on track
    if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_5) {
        int track = event->key() - Qt::Key_1;
        qint64 timeMs = m_player->position();
        if (timeMs >= 0) {
            addNoteAt(track, timeMs);
        }
    }
}

void EditorWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor("#0f0f1e"));

    int jy = height() / 2; // current time line
    int tw = trackWidth();

    // Draw track backgrounds
    for (int i = 0; i < 5; ++i) {
        int x = trackX(i);
        QRect r(x, 60, tw, height() - 60);
        p.fillRect(r, QColor(30, 100, 200, 30));
        p.setPen(QColor(50, 120, 220, 100));
        p.drawRect(r);

        // Track label
        QString labels[5] = {"A", "D", "G", "J", "L"};
        p.setPen(QColor(180, 200, 255, 200));
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(QRect(x, 65, tw, 30), Qt::AlignCenter, labels[i]);
    }

    // Current time line (center)
    p.setPen(QPen(QColor(255, 255, 255, 220), 2));
    int startX = trackX(0);
    int endX = trackX(4) + tw;
    p.drawLine(startX, jy, endX, jy);

    // Time labels
    p.setPen(QColor(255, 255, 255, 150));
    p.setFont(QFont("Arial", 10));
    for (int sec = -3; sec <= 3; ++sec) {
        int y = jy + static_cast<int>(sec * 1000 * PIXELS_PER_MS);
        if (y > 60 && y < height()) {
            p.drawLine(startX - 10, y, startX, y);
            p.drawText(QRect(startX - 50, y - 8, 40, 16), Qt::AlignRight,
                QString("%1s").arg(sec));
        }
    }

    // Draw notes
    qint64 now = m_player->position();
    for (const auto& note : m_chart.notes) {
        int x = trackX(note.track) + 5;
        int w = tw - 10;

        if (note.isShortPress()) {
            int y = timeToY(note.startTimeMs);
            QRect nr(x, y - 10, w, 20);
            if (nr.bottom() < 60 || nr.top() > height()) continue;

            QColor c = (note.color == NoteColor::Blue) ? QColor(52, 152, 219) : QColor(241, 196, 15);
            p.setBrush(c);
            p.setPen(QColor(255, 255, 255, 180));
            p.drawRoundedRect(nr, 5, 5);
        } else {
            int y1 = timeToY(note.startTimeMs);
            int y2 = timeToY(note.startTimeMs + note.durationMs);
            QRect nr(x, qMin(y1, y2), w, qMax(y1, y2) - qMin(y1, y2));
            if (nr.bottom() < 60 || nr.top() > height()) continue;

            QColor c = (note.color == NoteColor::Blue) ? QColor(52, 152, 219) : QColor(241, 196, 15);
            p.setBrush(c);
            p.setPen(QColor(255, 255, 255, 180));
            p.drawRoundedRect(nr, 6, 6);
        }
    }

    // Note count
    p.setPen(QColor(200, 200, 220, 200));
    p.setFont(QFont("Arial", 11));
    p.drawText(QRect(20, height() - 40, 200, 25), Qt::AlignLeft,
        QString("Notes: %1").arg(m_chart.notes.size()));
}

void EditorWidget::resizeEvent(QResizeEvent *)
{
    update();
}
