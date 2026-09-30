#pragma once

#include "core/statistics.h"
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <array>

namespace minesweeper {

class StatisticsCard : public QFrame {
public:
    explicit StatisticsCard(QWidget *parent = nullptr);
    QSize sizeHint() const override;
    void refresh(const Statistics &statistics, const QString &error);
    QPushButton *closeButton() const { return close_; }

private:
    std::array<std::array<QLabel *, 6>, 3> values_{};
    QLabel *error_;
    QPushButton *close_;
};

}
