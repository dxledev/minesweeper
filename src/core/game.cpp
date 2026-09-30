#include "game.h"

#include <algorithm>
#include <stdexcept>

namespace minesweeper {

Dimensions dimensions(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy: return {9, 9, 10};
    case Difficulty::Intermediate: return {16, 16, 40};
    case Difficulty::Expert: return {16, 30, 99};
    }
    throw std::invalid_argument("Invalid difficulty");
}

Difficulty parseDifficulty(std::string_view name) {
    if (name == "easy") return Difficulty::Easy;
    if (name == "intermediate") return Difficulty::Intermediate;
    if (name == "expert") return Difficulty::Expert;
    throw std::invalid_argument("Difficulty must be easy, intermediate, or expert");
}

std::string_view difficultyName(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy: return "easy";
    case Difficulty::Intermediate: return "intermediate";
    case Difficulty::Expert: return "expert";
    }
    throw std::invalid_argument("Invalid difficulty");
}

Game::Game(Difficulty difficulty) : Game(difficulty, std::random_device{}()) {}

Game::Game(Difficulty difficulty, std::uint32_t seed)
    : difficulty_(difficulty), dimensions_(dimensions(difficulty)),
      cells_(dimensions_.rows * dimensions_.columns), random_(seed) {}

bool Game::valid(int index) const {
    return index >= 0 && index < int(cells_.size());
}

std::vector<int> Game::neighbors(int index) const {
    std::vector<int> result;
    result.reserve(8);
    if (!valid(index)) return result;
    const int row = index / dimensions_.columns;
    const int column = index % dimensions_.columns;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (!dx && !dy) continue;
            const int y = row + dy, x = column + dx;
            if (y >= 0 && y < dimensions_.rows && x >= 0 && x < dimensions_.columns)
                result.push_back(y * dimensions_.columns + x);
        }
    }
    return result;
}

void Game::placeMines(int first) {
    const auto excluded = neighbors(first);
    std::vector<int> candidates;
    candidates.reserve(cells_.size());
    for (int index = 0; index < int(cells_.size()); ++index) {
        if (index != first && std::find(excluded.begin(), excluded.end(), index) == excluded.end())
            candidates.push_back(index);
    }
    std::shuffle(candidates.begin(), candidates.end(), random_);
    for (int index = 0; index < dimensions_.mines; ++index)
        cells_[candidates[index]].mine = true;
    for (int index = 0; index < int(cells_.size()); ++index) {
        for (int neighbor : neighbors(index))
            cells_[index].adjacent += cells_[neighbor].mine;
    }
    state_ = State::Playing;
}

void Game::floodReveal(int first) {
    std::vector<int> pending{first};
    cells_[first].revealed = true;
    ++revealed_;
    while (!pending.empty()) {
        const int index = pending.back();
        pending.pop_back();
        if (cells_[index].adjacent) continue;
        for (int neighbor : neighbors(index)) {
            auto &cell = cells_[neighbor];
            if (cell.revealed || cell.flagged || cell.mine) continue;
            cell.revealed = true;
            ++revealed_;
            if (!cell.adjacent) pending.push_back(neighbor);
        }
    }
}

void Game::checkWin() {
    if (revealed_ == int(cells_.size()) - dimensions_.mines) {
        state_ = State::Won;
        for (auto &cell : cells_) {
            if (cell.mine) cell.flagged = true;
        }
        flags_ = dimensions_.mines;
    }
}

bool Game::reveal(int index) {
    if (!valid(index) || finished() || cells_[index].flagged || cells_[index].revealed)
        return false;
    if (state_ == State::Ready) placeMines(index);
    if (cells_[index].mine) {
        cells_[index].revealed = true;
        detonated_ = index;
        state_ = State::Lost;
    } else {
        floodReveal(index);
        checkWin();
    }
    return true;
}

bool Game::toggleFlag(int index) {
    if (!valid(index) || finished() || cells_[index].revealed) return false;
    cells_[index].flagged = !cells_[index].flagged;
    flags_ += cells_[index].flagged ? 1 : -1;
    return true;
}

bool Game::chord(int index) {
    if (!valid(index) || finished() || !cells_[index].revealed || !cells_[index].adjacent)
        return false;
    const auto adjacent = neighbors(index);
    const int count = std::count_if(adjacent.begin(), adjacent.end(), [this](int neighbor) {
        return cells_[neighbor].flagged;
    });
    if (count != cells_[index].adjacent) return false;
    bool changed = false;
    for (int neighbor : adjacent) changed = reveal(neighbor) || changed;
    return changed;
}

}
