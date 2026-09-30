#pragma once

#include <cstdint>
#include <random>
#include <string_view>
#include <vector>

namespace minesweeper {

enum class Difficulty { Easy, Intermediate, Expert };
enum class State { Ready, Playing, Won, Lost };
struct Dimensions { int rows; int columns; int mines; };
struct Cell {
    std::uint8_t adjacent = 0;
    bool mine = false;
    bool revealed = false;
    bool flagged = false;
};

Dimensions dimensions(Difficulty difficulty);
Difficulty parseDifficulty(std::string_view name);
std::string_view difficultyName(Difficulty difficulty);

class Game {
public:
    explicit Game(Difficulty difficulty = Difficulty::Easy);
    Game(Difficulty difficulty, std::uint32_t seed);
    bool reveal(int index);
    bool toggleFlag(int index);
    bool chord(int index);
    Difficulty difficulty() const { return difficulty_; }
    const Dimensions &size() const { return dimensions_; }
    const Cell &cell(int index) const { return cells_.at(index); }
    const std::vector<Cell> &cells() const { return cells_; }
    std::vector<int> neighbors(int index) const;
    State state() const { return state_; }
    int flags() const { return flags_; }
    int revealedCount() const { return revealed_; }
    int detonated() const { return detonated_; }
    bool finished() const { return state_ == State::Won || state_ == State::Lost; }

private:
    Difficulty difficulty_;
    Dimensions dimensions_;
    std::vector<Cell> cells_;
    std::mt19937 random_;
    State state_ = State::Ready;
    int flags_ = 0;
    int revealed_ = 0;
    int detonated_ = -1;
    bool valid(int index) const;
    void placeMines(int first);
    void floodReveal(int first);
    void checkWin();
};

}
