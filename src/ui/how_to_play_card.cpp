#include "how_to_play_card.h"

#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace minesweeper {
namespace {

QLabel *rule(const QString &text) {
    auto *label = new QLabel(text);
    label->setObjectName("ruleText");
    label->setWordWrap(true);
    return label;
}

}

HowToPlayCard::HowToPlayCard(QWidget *parent) : QFrame(parent) {
    setObjectName("howToPlayCard");
    setMaximumWidth(600);
    auto policy = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    auto *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    layout->setContentsMargins(26, 26, 26, 26);
    layout->setSpacing(13);

    auto *title = new QLabel("How to play");
    title->setObjectName("modalTitle");
    layout->addWidget(title);
    layout->addWidget(rule("Reveal every safe cell without opening a mine. Opening a mine ends the game."));
    layout->addWidget(rule("A number tells you how many mines touch that cell, including diagonals. An empty cell has no neighboring mines and opens nearby safe cells automatically."));
    layout->addWidget(rule("Use the numbers to work out which hidden cells are safe. Right click a suspected mine to place or remove a flag. Flags help you keep track, but are not required to win."));
    layout->addWidget(rule("When the flags around a revealed number match it, double click that number to reveal its remaining neighbors. A wrong flag can expose a mine."));
    layout->addWidget(rule("Your first reveal and its neighboring cells are mine-free."));

    close_ = new QPushButton("Back to game");
    close_->setObjectName("closeHowToPlay");
    close_->setCursor(Qt::PointingHandCursor);
    layout->addSpacing(10);
    layout->addWidget(close_);
}

QSize HowToPlayCard::sizeHint() const {
    auto size = QFrame::sizeHint();
    size.setWidth(maximumWidth());
    size.setHeight(heightForWidth(size.width()));
    return size;
}

int HowToPlayCard::heightForWidth(int width) const {
    const auto margins = layout()->contentsMargins();
    const int contentWidth = width - 2 * frameWidth() - margins.left() - margins.right();
    int height = 2 * frameWidth() + margins.top() + margins.bottom();
    for (int index = 0; index < layout()->count(); ++index) {
        auto *item = layout()->itemAt(index);
        auto *widget = item->widget();
        auto *label = qobject_cast<QLabel *>(widget);
        height += label ? label->heightForWidth(contentWidth) : item->sizeHint().height();
        if (index) height += layout()->spacing();
    }
    return height;
}

void HowToPlayCard::resizeEvent(QResizeEvent *event) {
    QFrame::resizeEvent(event);
    const auto margins = layout()->contentsMargins();
    const int contentWidth = width() - 2 * frameWidth() - margins.left() - margins.right();
    for (auto *label : findChildren<QLabel *>())
        label->setMinimumHeight(label->heightForWidth(contentWidth));
    setMinimumHeight(heightForWidth(width()));
}

}
