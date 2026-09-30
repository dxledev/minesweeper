#include "statistics_card.h"

#include <QGridLayout>
#include <QVBoxLayout>

namespace minesweeper {
namespace {

QLabel *textLabel(const QString &text, const QString &name) {
    auto *label = new QLabel(text);
    label->setObjectName(name);
    return label;
}

QString formatTime(qint64 milliseconds) {
    if (!milliseconds) return "—";
    const auto seconds = milliseconds / 1000;
    return QString("%1:%2").arg(seconds / 60).arg(seconds % 60, 2, 10, QChar('0'));
}

}

StatisticsCard::StatisticsCard(QWidget *parent) : QFrame(parent) {
    setObjectName("statsCard");
    setMaximumWidth(720);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    layout->setContentsMargins(28, 28, 28, 28);
    layout->setSpacing(24);
    layout->addWidget(textLabel("Your minefield record", "modalTitle"));
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(18);
    const QStringList headings{"Easy", "Intermediate", "Expert"};
    const QStringList metrics{"Wins", "Losses", "Quits", "Fastest win", "Best win streak", "Safe cells cleared"};
    for (int column = 0; column < 3; ++column) {
        auto *heading = textLabel(headings[column], "statsHeading");
        heading->setAlignment(Qt::AlignCenter);
        grid->addWidget(heading, 0, column + 1);
        grid->setColumnStretch(column + 1, 1);
        for (int row = 0; row < metrics.size(); ++row) {
            auto *value = textLabel("0", QString("stats_%1_%2").arg(column).arg(row));
            value->setAlignment(Qt::AlignCenter);
            values_[column][row] = value;
            grid->addWidget(value, row + 1, column + 1);
        }
    }
    for (int row = 0; row < metrics.size(); ++row)
        grid->addWidget(textLabel(metrics[row], "statsCaption"), row + 1, 0);
    layout->addLayout(grid);
    auto *note = textLabel("Quits count when you leave an unfinished game after 3 minutes on the timer and at least one move. Paused time is excluded. Cells and streaks count recorded games.", "modalText");
    note->setWordWrap(true);
    layout->addWidget(note);
    error_ = textLabel("", "statsError");
    error_->setWordWrap(true);
    layout->addWidget(error_);
    error_->hide();
    close_ = new QPushButton("Back to game");
    close_->setObjectName("closeStats");
    close_->setCursor(Qt::PointingHandCursor);
    layout->addWidget(close_);
}

QSize StatisticsCard::sizeHint() const {
    auto size = QFrame::sizeHint();
    size.setWidth(maximumWidth());
    return size;
}

void StatisticsCard::refresh(const Statistics &statistics, const QString &error) {
    for (int index = 0; index < 3; ++index) {
        const auto &stats = statistics.forDifficulty(Difficulty(index));
        const QStringList values{QString::number(stats.wins), QString::number(stats.losses),
            QString::number(stats.quits), formatTime(stats.fastestWinMilliseconds),
            QString::number(stats.bestWinStreak), QString::number(stats.clearedCells)};
        for (int row = 0; row < values.size(); ++row) values_[index][row]->setText(values[row]);
    }
    error_->setText(error);
    error_->setVisible(!error.isEmpty());
}

}
