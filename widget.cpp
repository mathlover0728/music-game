#include "widget.h"
#include "gamewidget.h"
#include "editorwidget.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QFileInfo>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(900, 700);

    setupUI();
}

Widget::~Widget() {}

void Widget::setupUI()
{
    // Main container with dark rounded background
    QWidget* container = new QWidget(this);
    container->setObjectName("mainContainer");
    container->setStyleSheet(
        "#mainContainer {"
        "  background-color: #1a1a2e;"
        "  border-radius: 12px;"
        "  border: 1px solid #2d2d44;"
        "}"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addWidget(container);

    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    setupTitleBar();
    containerLayout->addWidget(m_titleBar);

    m_stack = new QStackedWidget();
    m_stack->setStyleSheet("background: transparent;");
    containerLayout->addWidget(m_stack, 1);

    setupMenu();
    m_stack->addWidget(m_menuScreen);

    m_gameScreen = new GameWidget();
    connect(m_gameScreen, &GameWidget::returnToMenu, this, &Widget::onReturnToMenu);
    m_stack->addWidget(m_gameScreen);

    m_editorScreen = new EditorWidget();
    connect(m_editorScreen, &EditorWidget::returnToMenu, this, &Widget::onReturnToMenu);
    connect(m_editorScreen, &EditorWidget::musicLoaded, this, &Widget::onMusicLoaded);
    m_stack->addWidget(m_editorScreen);

    m_stack->setCurrentIndex(0);
}

void Widget::setupTitleBar()
{
    m_titleBar = new QWidget();
    m_titleBar->setFixedHeight(44);
    m_titleBar->setStyleSheet(
        "background-color: #16162a;"
        "border-top-left-radius: 12px;"
        "border-top-right-radius: 12px;"
    );

    QHBoxLayout* layout = new QHBoxLayout(m_titleBar);
    layout->setContentsMargins(16, 0, 8, 0);
    layout->setSpacing(8);

    // Icon / title
    m_titleLabel = new QLabel("🎹  Rhythm Game");
    m_titleLabel->setStyleSheet("color: #e0e0ff; font-size: 15px; font-weight: bold;");
    layout->addWidget(m_titleLabel);
    layout->addStretch();

    // Minimize button
    m_btnMinimize = new QPushButton("—");
    m_btnMinimize->setFixedSize(32, 32);
    m_btnMinimize->setCursor(Qt::PointingHandCursor);
    m_btnMinimize->setStyleSheet(
        "QPushButton {"
        "  background-color: transparent;"
        "  color: #a0a0c0;"
        "  border-radius: 6px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2d2d55;"
        "  color: #ffffff;"
        "}"
    );
    connect(m_btnMinimize, &QPushButton::clicked, this, &Widget::onMinimize);
    layout->addWidget(m_btnMinimize);

    // Close button
    m_btnClose = new QPushButton("✕");
    m_btnClose->setFixedSize(32, 32);
    m_btnClose->setCursor(Qt::PointingHandCursor);
    m_btnClose->setStyleSheet(
        "QPushButton {"
        "  background-color: transparent;"
        "  color: #a0a0c0;"
        "  border-radius: 6px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e74c3c;"
        "  color: #ffffff;"
        "}"
    );
    connect(m_btnClose, &QPushButton::clicked, this, &Widget::onClose);
    layout->addWidget(m_btnClose);
}

void Widget::setupMenu()
{
    m_menuScreen = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_menuScreen);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(24);

    QLabel* title = new QLabel("🎵  Piano Rhythm");
    title->setStyleSheet("color: #ffffff; font-size: 36px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* subtitle = new QLabel("Import music, create charts, and play!");
    subtitle->setStyleSheet("color: #8888aa; font-size: 16px;");
    subtitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle);

    layout->addSpacing(30);

    auto createMenuBtn = [](const QString& text, const QString& color) -> QPushButton* {
        QPushButton* btn = new QPushButton(text);
        btn->setFixedSize(280, 56);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString(
            "QPushButton {"
            "  background-color: %1;"
            "  color: #ffffff;"
            "  border-radius: 10px;"
            "  font-size: 17px;"
            "  font-weight: bold;"
            "  border: none;"
            "}"
            "QPushButton:hover {"
            "  background-color: %2;"
            "}"
        ).arg(color, color));
        return btn;
    };

    QPushButton* btnImport = createMenuBtn("Import Music", "#3b5998");
    connect(btnImport, &QPushButton::clicked, this, &Widget::onImportMusic);
    layout->addWidget(btnImport, 0, Qt::AlignCenter);

    QPushButton* btnEditor = createMenuBtn("Chart Editor", "#8e44ad");
    connect(btnEditor, &QPushButton::clicked, this, &Widget::onOpenEditor);
    layout->addWidget(btnEditor, 0, Qt::AlignCenter);

    QPushButton* btnPlay = createMenuBtn("Play Game", "#27ae60");
    connect(btnPlay, &QPushButton::clicked, this, &Widget::onPlayGame);
    layout->addWidget(btnPlay, 0, Qt::AlignCenter);

    // Track key info
    QLabel* info = new QLabel("Tracks: A | D | G | J | L     Yellow notes: SPACE");
    info->setStyleSheet("color: #555577; font-size: 13px; margin-top: 20px;");
    info->setAlignment(Qt::AlignCenter);
    layout->addWidget(info);

    layout->addStretch();
}

void Widget::onImportMusic()
{
    QString path = QFileDialog::getOpenFileName(this, "Import Music",
        QString(), "Audio Files (*.mp3 *.wav *.flac *.ogg);;All Files (*)");
    if (!path.isEmpty()) {
        m_musicPath = path;
        m_titleLabel->setText(QString("🎹  Rhythm Game  —  %1").arg(QFileInfo(path).fileName()));
        QMessageBox::information(this, "Music Imported",
            QString("Imported: %1\nNow you can use the Chart Editor or Play Game.").arg(path));
    }
}

void Widget::onOpenEditor()
{
    if (m_musicPath.isEmpty()) {
        QMessageBox::warning(this, "No Music", "Please import music first!");
        return;
    }
    m_editorScreen->loadMusic(m_musicPath);
    m_stack->setCurrentWidget(m_editorScreen);
}

void Widget::onPlayGame()
{
    if (m_musicPath.isEmpty()) {
        QMessageBox::warning(this, "No Music", "Please import music first!");
        return;
    }
    // Try to load chart if exists
    QString chartPath = QFileInfo(m_musicPath).absolutePath() + "/" +
                        QFileInfo(m_musicPath).completeBaseName() + ".json";
    m_gameScreen->loadGame(m_musicPath, chartPath);
    m_stack->setCurrentWidget(m_gameScreen);
}

void Widget::onReturnToMenu()
{
    m_gameScreen->stopGame();
    m_editorScreen->stopEditor();
    m_stack->setCurrentWidget(m_menuScreen);
}

void Widget::onMusicLoaded(const QString& path)
{
    m_musicPath = path;
    m_titleLabel->setText(QString("🎹  Rhythm Game  —  %1").arg(QFileInfo(path).fileName()));
}

void Widget::onMinimize()
{
    showMinimized();
}

void Widget::onClose()
{
    close();
}

// Window drag
void Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_titleBar->geometry().contains(event->pos())) {
        m_dragging = true;
        m_dragStartPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragStartPos);
        event->accept();
    }
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}
