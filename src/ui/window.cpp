#include "window.h"
#include "how_to_play_card.h"
#include "modal_backdrop.h"
#include "statistics_card.h"

#include <QApplication>
#include <QEvent>
#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QShortcut>
#include <QVBoxLayout>
#include <algorithm>

namespace minesweeper {
namespace {

QLabel *label(const QString &text, const QString &name) {
    auto *result = new QLabel(text);
    result->setObjectName(name);
    return result;
}

QPushButton *button(const QString &text, const QString &name) {
    auto *result = new QPushButton(text);
    result->setObjectName(name);
    result->setCursor(Qt::PointingHandCursor);
    return result;
}

QWidget *makeMetric(const QString &caption, QLabel *&value) {
    auto *frame = new QFrame;
    frame->setObjectName("metric");
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(18, 12, 18, 12);
    layout->setSpacing(4);
    layout->addWidget(label(caption, "eyebrow"));
    value = label("0", "metricValue");
    layout->addWidget(value);
    return frame;
}

}

Window::Window(QString configDirectory, Theme theme, Difficulty difficulty)
    : game_(difficulty), statistics_(configDirectory + "/stats.json"),
      themeWatcher_(new ThemeWatcher(configDirectory + "/theme.json", std::move(theme), this)) {
    setObjectName("minesweeperWindow");
    setWindowTitle("Minesweeper");
    setMinimumSize(580, 680);
    resize(difficulty == Difficulty::Expert ? 1140 : 720, 800);
    buildInterface();
    buildModal();
    buildShortcuts();
    applyTheme();
    refresh();
    connect(themeWatcher_, &ThemeWatcher::changed, this, [this] {
        applyTheme();
        refresh();
        if (modal_->isVisible()) snapshotRefresh_.start();
    });
    connect(themeWatcher_, &ThemeWatcher::rejected, this, [this](const QString &error) {
        themeError_->setText("Theme not loaded: " + error);
        themeError_->show();
    });
    connect(themeWatcher_, &ThemeWatcher::recovered, themeError_, &QWidget::hide);
    connect(&ticker_, &QTimer::timeout, this, &Window::refresh);
    snapshotRefresh_.setSingleShot(true);
    snapshotRefresh_.setInterval(0);
    connect(&snapshotRefresh_, &QTimer::timeout, this, &Window::captureModalBackground);
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
        stopClock();
        recordOutcome();
    });
    board_->setFocus();
}

void Window::buildInterface() {
    auto *root = new QWidget;
    setCentralWidget(root);
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    content_ = new QWidget(root);
    rootLayout->addWidget(content_);
    auto *layout = new QVBoxLayout(content_);
    layout->setContentsMargins(24, 22, 24, 18);
    layout->setSpacing(16);
    layout->addLayout(buildHeader());
    layout->addLayout(buildDifficultySelector());
    layout->addLayout(buildMetrics());
    layout->addWidget(buildBoardCard(), 1);
    layout->addLayout(buildFooter());
}

QHBoxLayout *Window::buildHeader() {
    auto *header = new QHBoxLayout;
    auto *titles = new QVBoxLayout;
    titles->setSpacing(3);
    titles->addWidget(label("Minesweeper", "title"));
    subtitle_ = label("Find a little clarity in the minefield.", "subtitle");
    titles->addWidget(subtitle_);
    header->addLayout(titles);
    header->addStretch();
    auto *howToPlay = button("How to play", "howToPlay");
    connect(howToPlay, &QPushButton::clicked, this, &Window::showHowToPlay);
    header->addWidget(howToPlay);
    auto *stats = button("Stats", "stats");
    connect(stats, &QPushButton::clicked, this, &Window::showStatistics);
    header->addWidget(stats);
    auto *newGame = button("New game", "primary");
    connect(newGame, &QPushButton::clicked, this, [this] { requestGame(game_.difficulty()); });
    header->addWidget(newGame);
    return header;
}

QHBoxLayout *Window::buildDifficultySelector() {
    auto *row = new QHBoxLayout;
    row->setSpacing(8);
    difficulties_ = new QButtonGroup(this);
    const QStringList names{"Easy", "Intermediate", "Expert"};
    for (int index = 0; index < names.size(); ++index) {
        auto *choice = button(names[index], "difficulty");
        choice->setCheckable(true);
        choice->setChecked(index == int(game_.difficulty()));
        difficulties_->addButton(choice, index);
        row->addWidget(choice);
    }
    connect(difficulties_, &QButtonGroup::idClicked, this, [this](int id) { requestGame(Difficulty(id)); });
    auto *centered = new QHBoxLayout;
    centered->setSpacing(0);
    centered->addStretch(2);
    centered->addLayout(row, 6);
    centered->addStretch(2);
    return centered;
}

QHBoxLayout *Window::buildMetrics() {
    auto *metrics = new QHBoxLayout;
    metrics->setSpacing(10);
    metrics->addWidget(makeMetric("MINES LEFT", mines_), 1);
    metrics->addWidget(makeMetric("TIME", time_), 1);
    metrics->addWidget(makeMetric("CLEARED", progress_), 1);
    return metrics;
}

QWidget *Window::buildBoardCard() {
    auto *card = new QFrame;
    card->setObjectName("boardCard");
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(8, 8, 8, 8);
    board_ = new Board(game_);
    connect(board_, &Board::moved, this, &Window::moved);
    layout->addWidget(board_);
    return card;
}

QVBoxLayout *Window::buildFooter() {
    auto *layout = new QVBoxLayout;
    layout->setSpacing(16);
    auto *controls = new QHBoxLayout;
    status_ = label("Your first move is always safe.", "status");
    status_->setWordWrap(true);
    controls->addWidget(status_, 1);
    flagMode_ = button("Flag mode", "flagMode");
    flagMode_->setCheckable(true);
    connect(flagMode_, &QPushButton::toggled, board_, &Board::setFlagMode);
    controls->addWidget(flagMode_);
    pause_ = button("Pause", "pause");
    connect(pause_, &QPushButton::clicked, this, [this] { automaticPause_ = false; setPaused(!paused_); });
    controls->addWidget(pause_);
    layout->addLayout(controls);
    auto *hint = label("Left click · Reveal     Right click · Flag     Double click number · Clear around", "hint");
    hint->setWordWrap(true);
    layout->addWidget(hint);
    themeError_ = label("", "themeError");
    themeError_->setWordWrap(true);
    layout->addWidget(themeError_);
    themeError_->hide();
    statisticsError_ = label("", "statsError");
    statisticsError_->setWordWrap(true);
    layout->addWidget(statisticsError_);
    statisticsError_->hide();
    return layout;
}

void Window::buildModal() {
    modal_ = new ModalBackdrop(centralWidget());
    modal_->installEventFilter(this);
    auto *layout = new QVBoxLayout(modal_);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->addStretch();
    auto *card = new QFrame;
    newGameCard_ = card;
    card->setObjectName("modalCard");
    card->setMaximumWidth(460);
    card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *content = new QVBoxLayout(card);
    content->setSizeConstraint(QLayout::SetMinimumSize);
    content->setContentsMargins(26, 26, 26, 26);
    content->setSpacing(16);
    content->addWidget(label("Start a new game?", "modalTitle"));
    modalText_ = label("", "modalText");
    modalText_->setWordWrap(true);
    content->addWidget(modalText_);
    auto *actions = new QHBoxLayout;
    keepPlaying_ = button("Keep playing", "keepPlaying");
    confirmNew_ = button("New game", "primary");
    keepPlaying_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    confirmNew_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    keepPlaying_->installEventFilter(this);
    confirmNew_->installEventFilter(this);
    connect(keepPlaying_, &QPushButton::clicked, this, &Window::dismissModal);
    connect(confirmNew_, &QPushButton::clicked, this, [this] {
        const auto difficulty = pendingDifficulty_;
        modal_->hide();
        modal_->clearSnapshot();
        startGame(difficulty);
    });
    actions->addWidget(keepPlaying_);
    actions->addWidget(confirmNew_);
    content->addLayout(actions);
    layout->addWidget(card, 0, Qt::AlignHCenter);
    statisticsCard_ = new StatisticsCard;
    statisticsCard_->closeButton()->installEventFilter(this);
    connect(statisticsCard_->closeButton(), &QPushButton::clicked, this, &Window::dismissModal);
    layout->addWidget(statisticsCard_, 0, Qt::AlignHCenter);
    statisticsCard_->hide();
    howToPlayCard_ = new HowToPlayCard;
    howToPlayCard_->closeButton()->installEventFilter(this);
    connect(howToPlayCard_->closeButton(), &QPushButton::clicked, this, &Window::dismissModal);
    layout->addWidget(howToPlayCard_, 0, Qt::AlignHCenter);
    howToPlayCard_->hide();
    layout->addStretch();
    modal_->hide();
}

void Window::buildShortcuts() {
    auto *newGame = new QShortcut(QKeySequence("Ctrl+N"), this);
    connect(newGame, &QShortcut::activated, this, [this] { requestGame(game_.difficulty()); });
    auto *pause = new QShortcut(QKeySequence("P"), this);
    connect(pause, &QShortcut::activated, this, [this] {
        if (!modal_->isVisible()) { automaticPause_ = false; setPaused(!paused_); }
    });
    auto *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escape, &QShortcut::activated, this, [this] {
        if (modal_->isVisible()) dismissModal();
    });
}

void Window::applyTheme() {
    const auto &theme = themeWatcher_->theme();
    QPalette palette;
    palette.setColor(QPalette::Window, theme.color("background"));
    palette.setColor(QPalette::WindowText, theme.color("text"));
    palette.setColor(QPalette::Base, theme.color("surface"));
    palette.setColor(QPalette::Text, theme.color("text"));
    palette.setColor(QPalette::Button, theme.color("surface_alt"));
    palette.setColor(QPalette::ButtonText, theme.color("text"));
    palette.setColor(QPalette::Highlight, theme.color("accent"));
    palette.setColor(QPalette::HighlightedText, theme.color("accent_text"));
    setPalette(palette);
    QString style = R"(
        QMainWindow#minesweeperWindow { background: $background; }
        QWidget { color: $text; font-size: 13px; }
        QLabel { background: transparent; }
        QLabel#title { font-size: 28px; font-weight: 700; }
        QLabel#subtitle, QLabel#hint, QLabel#eyebrow { color: $muted; }
        QLabel#eyebrow { font-size: 10px; font-weight: 600; }
        QLabel#metricValue { font-size: 25px; font-weight: 600; }
        QLabel#hint { font-size: 11px; }
        QLabel#themeError, QLabel#statsError { color: $danger; font-size: 11px; }
        QFrame#metric, QFrame#boardCard { background: $surface; border: 1px solid $border; border-radius: 14px; }
        QPushButton { background: $surface_alt; border: 1px solid $border; border-radius: 9px; padding: 10px 16px; font-weight: 600; }
        QPushButton:hover { background: $selection; }
        QPushButton:focus { border: 1px solid $accent; }
        QPushButton:checked { color: $accent; background: $matching; border: 1px solid $accent; }
        QPushButton#primary { background: $accent; color: $accent_text; border-color: $accent; }
        QPushButton#primary:hover { border-color: $text; }
        QPushButton:disabled { color: $muted; }
        QFrame#modalCard, QFrame#statsCard, QFrame#howToPlayCard { background: $surface; border: 1px solid $border; border-radius: 16px; }
        QFrame#modalCard QPushButton, QFrame#statsCard QPushButton, QFrame#howToPlayCard QPushButton { min-height: 22px; padding: 10px 16px; }
        QFrame#howToPlayCard QLabel#ruleText { color: $text; font-size: 16px; }
        QFrame#howToPlayCard QLabel#modalTitle { font-size: 26px; }
        QFrame#statsCard QLabel, QFrame#statsCard QPushButton { font-size: 16px; }
        QFrame#statsCard QLabel { min-height: 26px; }
        QLabel#statsHeading { color: $accent; font-weight: 600; }
        QLabel#statsCaption { color: $muted; }
        QLabel#modalTitle { font-size: 23px; font-weight: 600; }
        QLabel#modalText { color: $muted; font-size: 14px; }
        QFrame#statsCard QLabel#modalTitle { font-size: 26px; min-height: 38px; }
        QFrame#statsCard QLabel#modalText { font-size: 15px; }
    )";
    auto keys = colorKeys();
    std::sort(keys.begin(), keys.end(), [](const QString &left, const QString &right) {
        return left.size() > right.size();
    });
    for (const auto &key : keys) style.replace("$" + key, theme.hex(key));
    setStyleSheet(style);
    board_->setTheme(theme);
    modal_->setTint(theme.color("background"));
}

qint64 Window::elapsedMilliseconds() const {
    return accumulatedMilliseconds_ + (clock_.isValid() ? clock_.elapsed() : 0);
}

void Window::stopClock() {
    if (clock_.isValid()) {
        accumulatedMilliseconds_ += clock_.elapsed();
        clock_.invalidate();
    }
    ticker_.stop();
}

void Window::refresh() {
    mines_->setText(QString::number(game_.size().mines - game_.flags()));
    const qint64 seconds = elapsedMilliseconds() / 1000;
    time_->setText(QString("%1:%2").arg(seconds / 60, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0')));
    progress_->setText(QString("%1 / %2").arg(game_.revealedCount()).arg(int(game_.cells().size()) - game_.size().mines));
    const auto &size = game_.size();
    subtitle_->setText(QString("%1 × %2 cells · %3 mines").arg(size.columns).arg(size.rows).arg(size.mines));
    difficulties_->button(int(game_.difficulty()))->setChecked(true);
    pause_->setEnabled(game_.state() == State::Playing);
    pause_->setText(paused_ ? "Resume" : "Pause");
    flagMode_->setEnabled(!game_.finished());
    if (game_.state() == State::Won) status_->setText("Minefield cleared. Well played!");
    else if (game_.state() == State::Lost) status_->setText("A mine! Start a new game to try again.");
    else if (paused_) status_->setText("Take your time. Resume when you're ready.");
    else if (game_.state() == State::Ready) status_->setText("Your first move is always safe.");
    else status_->setText("Read the numbers. Trust your next move.");
    status_->setStyleSheet(QString("color: %1;").arg(game_.state() == State::Lost ? themeWatcher_->theme().hex("danger")
        : game_.state() == State::Won ? themeWatcher_->theme().hex("success") : themeWatcher_->theme().hex("text")));
}

void Window::moved() {
    madeMove_ = true;
    if (game_.state() == State::Playing && !clock_.isValid() && !paused_) {
        clock_.start();
        ticker_.start(1000);
    }
    if (game_.finished()) {
        stopClock();
        recordOutcome();
    }
    refresh();
}

void Window::setPaused(bool paused) {
    if (game_.state() != State::Playing) return;
    paused_ = paused;
    if (paused_) stopClock();
    else { clock_.start(); ticker_.start(1000); }
    board_->setPaused(paused_);
    refresh();
}

void Window::requestGame(Difficulty difficulty) {
    if (modal_->isVisible()) return;
    if (game_.state() != State::Playing) { startGame(difficulty); return; }
    pendingDifficulty_ = difficulty;
    difficulties_->button(int(game_.difficulty()))->setChecked(true);
    modalText_->setText("Starting a new game will replace your current minefield. You will lose your progress.");
    showModal(newGameCard_, keepPlaying_);
}

void Window::showStatistics() {
    if (modal_->isVisible()) return;
    if (game_.finished()) recordOutcome();
    try {
        statistics_.reload();
        if (statisticsMessage_.startsWith("Stats not loaded:")) statisticsMessage_.clear();
    } catch (const std::exception &error) {
        statisticsMessage_ = "Stats not loaded: " + QString::fromUtf8(error.what());
    }
    statisticsCard_->refresh(statistics_, statisticsMessage_);
    showModal(statisticsCard_, statisticsCard_->closeButton());
}

void Window::showHowToPlay() {
    if (modal_->isVisible()) return;
    showModal(howToPlayCard_, howToPlayCard_->closeButton());
}

void Window::showModal(QWidget *card, QPushButton *focus) {
    modalPaused_ = paused_;
    setPaused(true);
    newGameCard_->setVisible(card == newGameCard_);
    statisticsCard_->setVisible(card == statisticsCard_);
    howToPlayCard_->setVisible(card == howToPlayCard_);
    modal_->setGeometry(centralWidget()->rect());
    modal_->show();
    if (card == newGameCard_) {
        modalText_->setMinimumHeight(modalText_->heightForWidth(modalText_->width()));
        modal_->layout()->activate();
    }
    captureModalBackground();
    modal_->raise();
    focus->setFocus();
}

void Window::recordOutcome() {
    if (outcomeRecorded_) return;
    try {
        statistics_.record(game_.difficulty(), game_.state(), elapsedMilliseconds(), madeMove_, game_.revealedCount());
        outcomeRecorded_ = true;
        statisticsMessage_.clear();
        statisticsError_->hide();
    } catch (const std::exception &error) {
        statisticsMessage_ = "Stats not saved: " + QString::fromUtf8(error.what());
        statisticsError_->setText(statisticsMessage_);
        statisticsError_->show();
    }
}

void Window::dismissModal() {
    modal_->hide();
    modal_->clearSnapshot();
    setPaused(modalPaused_);
    board_->setFocus();
}

void Window::captureModalBackground() {
    if (!modal_->isVisible()) return;
    const bool overlayPaused = paused_;
    paused_ = modalPaused_;
    board_->setPaused(paused_);
    refresh();
    content_->layout()->activate();
    QPixmap snapshot(content_->size() * content_->devicePixelRatioF());
    snapshot.setDevicePixelRatio(content_->devicePixelRatioF());
    snapshot.fill(themeWatcher_->theme().color("background"));
    QPainter painter(&snapshot);
    // Render transparent content over the actual window color, bypassing grab's palette fill.
    content_->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    modal_->capture(snapshot);
    paused_ = overlayPaused;
    board_->setPaused(paused_);
    refresh();
}

void Window::startGame(Difficulty difficulty) {
    stopClock();
    recordOutcome();
    accumulatedMilliseconds_ = 0;
    paused_ = false;
    automaticPause_ = false;
    madeMove_ = false;
    outcomeRecorded_ = false;
    game_ = Game(difficulty);
    board_->resetSelection();
    board_->setPaused(false);
    flagMode_->setChecked(false);
    board_->setFocus();
    refresh();
}

void Window::closeEvent(QCloseEvent *event) {
    QMainWindow::closeEvent(event);
    if (event->isAccepted()) {
        stopClock();
        recordOutcome();
    }
}

void Window::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (modal_) {
        modal_->setGeometry(centralWidget()->rect());
        if (modal_->isVisible()) snapshotRefresh_.start();
    }
}

void Window::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    if (event->type() != QEvent::ActivationChange || !board_ || !modal_ || modal_->isVisible()) return;
    if (!isActiveWindow() && game_.state() == State::Playing && !paused_) {
        automaticPause_ = true;
        setPaused(true);
    } else if (isActiveWindow() && automaticPause_) {
        automaticPause_ = false;
        setPaused(false);
    }
}

bool Window::eventFilter(QObject *object, QEvent *event) {
    if (object == modal_ && event->type() == QEvent::MouseButtonPress) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        const QPoint point = mouse->position().toPoint();
        for (QWidget *card : {static_cast<QWidget *>(newGameCard_), static_cast<QWidget *>(statisticsCard_), static_cast<QWidget *>(howToPlayCard_)})
            if (card->isVisible() && card->geometry().contains(point)) return false;
        dismissModal();
        return true;
    }
    if (modal_->isVisible() && event->type() == QEvent::KeyPress) {
        const auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Escape) {
            dismissModal();
            return true;
        }
        if (key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab) {
            if (statisticsCard_->isVisible()) statisticsCard_->closeButton()->setFocus();
            else if (howToPlayCard_->isVisible()) howToPlayCard_->closeButton()->setFocus();
            else (object == keepPlaying_ ? confirmNew_ : keepPlaying_)->setFocus();
            return true;
        }
    }
    return QMainWindow::eventFilter(object, event);
}

}
