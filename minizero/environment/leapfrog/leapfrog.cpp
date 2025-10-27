#include "leapfrog.h"
#include "random.h"
#include "sgf_loader.h"
#include <algorithm>
#include <string>

namespace minizero::env::leapfrog {

using namespace minizero::utils;


int LeapFrogAction::charToPos(char c) const
{
    c = std::toupper(c);
    if ('A' <= c && c <= 'Z' && c != 'I') {
        return c - 'A' - (c > 'I' ? 1 : 0);
    }
    assert(false);
    return -1;
}


int LeapFrogAction::coordinateToID(int c1, int r1, int c2, int r2) const
{
    // Convert board coordinates to action ID.
    // dir = 0: north
    // dir = 1: northeast
    // dir = 2: east
    // dir = 3: souteast
    // dir = 4: south
    // dir = 5: southwest
    // dir = 6: west
    // dir = 7: northwest
    int dir = 0;
    if ((c2 == c1) && (r2 - r1 == 2)) {
        dir = 0;
    } else if ((c2 - c1 == 2) && (r2 - r1 == 2)) {
        dir = 1;
    } else if ((c2 - c1 == 2) && (r2 == r1)) {
        dir = 2;
    } else if ((c2 - c1 == 2) && (r2 - r1 == -2)) {
        dir = 3;
    } else if ((c2 == c1) && (r2 - r1 == -2)) {
        dir = 4;
    } else if ((c2 - c1 == -2) && (r2 - r1 == -2)) {
        dir = 5;
    } else if ((c2 - c1 == -2) && (r2 == r1)) {
        dir = 6;
    } else if ((c2 - c1 == -2) && (r2 - r1 == 2)) {
        dir = 7;
    } else {
        return -1; // simply return illegal action error
    }

    int from_pos = r1 * board_size_ + c1;

    return (dir * (board_size_ * board_size_)) + from_pos;
}

int LeapFrogAction::actionStringToID(const std::vector<std::string>& action_string_args) const
{
    // Action string: <player> <from><to>
    // Parse coordinates: <from><to>.
    // E.g., c3a1: from c3 to a1
    assert(action_string_args.size() == 2);
    assert(action_string_args[0].size() == 1);
    std::string command_str = action_string_args[1];

    // The index of the destination position in command_str.
    // For example, for a10b10 is 3; for a2b2 is 2.
    int dest_pos_idx = command_str.find_first_not_of("0123456789", 1);

    int r1_r = std::stoi(command_str.substr(1, dest_pos_idx));
    int r2_r = std::stoi(command_str.substr(dest_pos_idx + 1, (command_str.size() - (dest_pos_idx + 1))));
    int r1 = r1_r - 1;
    int r2 = r2_r - 1;

    int c1 = charToPos(command_str[0]);
    int c2 = charToPos(command_str[dest_pos_idx]);
    if (r1 < 0 || r2 < 0 || c1 < 0 || c2 < 0) {
        return -1; // simply return illegal action error
    }
    if (r1 >= board_size_ || r2 >= board_size_ || c1 >= board_size_ || c2 >= board_size_) {
        return -1; // simply return illegal action error
    }
    return coordinateToID(c1, r1, c2, r2);
}


int LeapFrogAction::getFromPos(int action_id) const
{
    return action_id % (board_size_ * board_size_);
}


int LeapFrogAction::getDestPos(int action_id) const
{
    int spatial = board_size_ * board_size_;
    int dir = action_id / spatial;
    int pos = action_id % spatial;

    int row = pos / board_size_;
    int col = pos % board_size_;

    // dir = 0: north
    // dir = 1: northeast
    // dir = 2: east
    // dir = 3: souteast
    // dir = 4: south
    // dir = 5: southwest
    // dir = 6: west
    // dir = 7: northwest
    if (dir == 0) {
        row += 2;
    } else if (dir == 1) {
        row += 2;
        col += 2;
    } else if (dir == 2) {
        col += 2;
    } else if (dir == 3) {
        row -= 2;
        col += 2;
    } else if (dir == 4) {
        row -= 2;
    } else if (dir == 5) {
        row -= 2;
        col -= 2;
    } else if (dir == 6) {
        col -= 2;
    } else if (dir == 7) {
        row += 2;
        col -= 2;
    }

    // Dest position is out of board area.
    // It is not a legal move.
    if (col >= board_size_ || col < 0 || row >= board_size_ || row < 0) {
        return -1;
    }

    return row * board_size_ + col;
}


std::string LeapFrogAction::actionIDtoString(int action_id) const
{
    int pos = getFromPos(action_id);
    int row = pos / board_size_;
    int col = pos % board_size_;

    int dest_pos = getDestPos(action_id);
    int dest_row = dest_pos / board_size_;
    int dest_col = dest_pos % board_size_;

    char col_c = 'a' + col + ('a' + col >= 'i' ? 1 : 0);
    char dest_col_c = 'a' + dest_col + ('a' + dest_col >= 'i' ? 1 : 0);

    return std::string(1, col_c) + std::to_string(row + 1) + std::string(1, dest_col_c) + std::to_string(dest_row + 1);
}


void LeapFrogEnv::reset()
{
    turn_ = Player::kPlayer1;
    actions_.clear();
    bitboard_.reset();

    // Initialize pieces for both players
    // Initial placement:
    // Black: O, White: X, Empty: .
    //   A B C D E F G H
    // 8 . . . . . . . . 8
    // 7 . . . X O . . . 7
    // 6 . . O O X X . . 6
    // 5 . X O X O X O . 5
    // 4 . O X O X O X . 4
    // 3 . . X X O O . . 3
    // 2 . . . O X . . . 2
    // 1 . . . . . . . . 1
    //   A B C D E F G H

    // Place Black pieces (Player 1, represented as O)
    bitboard_.get(Player::kPlayer1).set(6 * board_size_ + 4); // E7

    bitboard_.get(Player::kPlayer1).set(5 * board_size_ + 2); // C6
    bitboard_.get(Player::kPlayer1).set(5 * board_size_ + 3); // D6

    bitboard_.get(Player::kPlayer1).set(4 * board_size_ + 2); // C5
    bitboard_.get(Player::kPlayer1).set(4 * board_size_ + 4); // E5
    bitboard_.get(Player::kPlayer1).set(4 * board_size_ + 6); // G5

    bitboard_.get(Player::kPlayer1).set(3 * board_size_ + 1); // B4
    bitboard_.get(Player::kPlayer1).set(3 * board_size_ + 3); // D4
    bitboard_.get(Player::kPlayer1).set(3 * board_size_ + 5); // F4

    bitboard_.get(Player::kPlayer1).set(2 * board_size_ + 4); // E3
    bitboard_.get(Player::kPlayer1).set(2 * board_size_ + 5); // F3

    bitboard_.get(Player::kPlayer1).set(1 * board_size_ + 3); // D2

    // Place White pieces (Player 2, represented as X)
    bitboard_.get(Player::kPlayer2).set(6 * board_size_ + 3); // D7

    bitboard_.get(Player::kPlayer2).set(5 * board_size_ + 4); // E6
    bitboard_.get(Player::kPlayer2).set(5 * board_size_ + 5); // F6

    bitboard_.get(Player::kPlayer2).set(4 * board_size_ + 1); // B5
    bitboard_.get(Player::kPlayer2).set(4 * board_size_ + 3); // D5
    bitboard_.get(Player::kPlayer2).set(4 * board_size_ + 5); // F5

    bitboard_.get(Player::kPlayer2).set(3 * board_size_ + 2); // C4
    bitboard_.get(Player::kPlayer2).set(3 * board_size_ + 4); // E4
    bitboard_.get(Player::kPlayer2).set(3 * board_size_ + 6); // G4

    bitboard_.get(Player::kPlayer2).set(2 * board_size_ + 2); // C3
    bitboard_.get(Player::kPlayer2).set(2 * board_size_ + 3); // D3
    
    bitboard_.get(Player::kPlayer2).set(1 * board_size_ + 4); // E2

    bitboard_history_.clear();
    bitboard_history_.push_back(bitboard_);
}

bool LeapFrogEnv::act(const LeapFrogAction& action)
{
    if (!isLegalAction(action)) { return false; }

    int from_pos = action.getFromPos();
    int dest_pos = action.getDestPos();

    // Remove piece from source position
    bitboard_.get(action.getPlayer()).reset(from_pos);

    // Calculate jumped-over piece position
    int cap_pos = (from_pos + dest_pos) / 2;

    // Remove jumped-over piece if its enemy's
    if (bitboard_.get(action.nextPlayer()).test(cap_pos)) {
        bitboard_.get(action.nextPlayer()).reset(cap_pos);
    }
    
    // Place piece at destination position
    bitboard_.get(action.getPlayer()).set(dest_pos);

    // Update game state
    actions_.push_back(action);
    bitboard_history_.push_back(bitboard_);
    turn_ = action.nextPlayer();
    
    return true;
}

bool LeapFrogEnv::act(const std::vector<std::string>& action_string_args)
{
    return act(LeapFrogAction(action_string_args));
}

std::vector<LeapFrogAction> LeapFrogEnv::getLegalActions() const
{
    std::vector<LeapFrogAction> actions;
    for (int pos = 0; pos < kNumDirections * board_size_ * board_size_; ++pos) {
        LeapFrogAction action(pos, turn_);
        if (!isLegalAction(action)) { continue; }
        actions.push_back(action);
    }
    return actions;
}

bool LeapFrogEnv::isLegalAction(const LeapFrogAction& action) const
{
    if (action.getPlayer() != getTurn()) { return false; }
    assert(action.getActionID() >= 0 && action.getActionID() < kNumDirections * board_size_ * board_size_);

    // The piece at the source/from position should be ours.
    int pos = action.getFromPos();
    if (getPlayerAtBoardPos(pos) != action.getPlayer()) { return false; }

    // The destination position is out of board.
    int dest_pos = action.getDestPos();
    if (dest_pos == -1) { return false; }

    // The piece at the source position should jump over a piece (either ours or opponent's).
    int mid_pos = (pos + dest_pos) / 2;
    if (getPlayerAtBoardPos(mid_pos) == Player::kPlayerNone) { return false; }

    // The piece at the destination position should be empty
    if (getPlayerAtBoardPos(dest_pos) == Player::kPlayerNone) { return true; }

    return false;
}

bool LeapFrogEnv::isTerminal() const
{
    // Self-play might go to infinite moves
    // Game length capped at 5 * boardsize * boardsize
    if (static_cast<int>(actions_.size()) > 5 * board_size_ * board_size_) { return true; }

    return getLegalActions().empty();
}

float LeapFrogEnv::getEvalScore(bool is_resign /*= false*/) const
{
    Player result = (is_resign ? getNextPlayer(turn_, kLeapFrogNumPlayer) : eval());
    // Small offset to ensure float formatting for binding and training with Python
    const float offset = 0.00001f;
    switch (result) {
        case Player::kPlayer1: return 1.0f + offset;
        case Player::kPlayer2: return -1.0f - offset;
        default: return 0.0f;
    }
}

std::vector<float> LeapFrogEnv::getFeatures(utils::Rotation rotation /* = utils::Rotation::kRotationNone */) const
{
    /* 18 channels:
        0~15. own/opponent position for last 8 turns
        16. 1st player turn
        17. 2nd player turn
    */
    int past_moves = std::min(8, static_cast<int>(bitboard_history_.size()));
    int spatial = board_size_ * board_size_;
    std::vector<float> features(getNumInputChannels() * spatial, 0.0f);
    int last_idx = bitboard_history_.size() - 1;

    // 0 ~ 15
    for (int c = 0; c < 2 * past_moves; c += 2) {
        const LeapFrogBitboard& own_bitboard = bitboard_history_[last_idx - (c / 2)].get(turn_);
        const LeapFrogBitboard& opponent_bitboard = bitboard_history_[last_idx - (c / 2)].get(getNextPlayer(turn_, kLeapFrogNumPlayer));
        for (int pos = 0; pos < spatial; ++pos) {
            int rotation_pos = getRotatePosition(pos, utils::reversed_rotation[static_cast<int>(rotation)]);
            features[pos + c * spatial] = (own_bitboard.test(rotation_pos) ? 1.0f : 0.0f);
            features[pos + (c + 1) * spatial] = (opponent_bitboard.test(rotation_pos) ? 1.0f : 0.0f);
        }
    }

    // 16 ~ 17
    for (int pos = 0; pos < spatial; ++pos) {
        features[pos + 16 * spatial] = static_cast<float>(turn_ == Player::kPlayer1);
        features[pos + 17 * spatial] = static_cast<float>(turn_ == Player::kPlayer2);
    }
    return features;
}

std::vector<float> LeapFrogEnv::getActionFeatures(const LeapFrogAction& action, utils::Rotation rotation /* = utils::Rotation::kRotationNone */) const
{
    // throw std::runtime_error{"LeapFrogEnv::getActionFeatures() is not implemented"};
    std::vector<float> action_features(getPolicySize(), 0.0f);
    action_features[getRotateAction(action.getActionID(), rotation)] = 1.0f;
    return action_features;
}

std::string LeapFrogEnv::getCoordinateString() const
{
    std::ostringstream oss;
    oss << "  ";
    for (int i = 0; i < board_size_; ++i) {
        char c = 'A' + i + ('A' + i >= 'I' ? 1 : 0);
        oss << " " + std::string(1, c) + " ";
    }
    oss << "   ";
    return oss.str();
}

std::string LeapFrogEnv::toString() const
{
    std::ostringstream oss;
    oss << " " << getCoordinateString() << std::endl;
    for (int row = board_size_ - 1; row >= 0; --row) {
        oss << (row >= 9 ? "" : " ") << row + 1 << " ";
        for (int col = 0; col < board_size_; ++col) {
            Player color = getPlayerAtBoardPos(row * board_size_ + col);
            if (color == Player::kPlayer1) {
                oss << " O ";
            } else if (color == Player::kPlayer2) {
                oss << " X ";
            } else {
                oss << " . ";
            }
        }
        oss << (row >= 9 ? "" : " ") << row + 1 << std::endl;
    }
    oss << " " << getCoordinateString() << std::endl;
    return oss.str();
}

Player LeapFrogEnv::eval() const
{
    // If the current player has no legal moves, they lose
    if (getLegalActions().empty()) { return getNextPlayer(turn_, kLeapFrogNumPlayer);}
    return Player::kPlayerNone; 
}

Player LeapFrogEnv::getPlayerAtBoardPos(int position) const
{
    if (bitboard_.get(Player::kPlayer1).test(position)) {
        return Player::kPlayer1;
    } else if (bitboard_.get(Player::kPlayer2).test(position)) {
        return Player::kPlayer2;
    }
    return Player::kPlayerNone;
}

std::vector<float> LeapFrogEnvLoader::getActionFeatures(const int pos, utils::Rotation rotation /* = utils::Rotation::kRotationNone */) const
{
    // throw std::runtime_error{"LeapFrogEnvLoader::getActionFeatures() is not implemented"};
    const LeapFrogAction& action = action_pairs_[pos].first;
    std::vector<float> action_features(getPolicySize(), 0.0f);
    int action_id = ((pos < static_cast<int>(action_pairs_.size())) ? getRotateAction(action.getActionID(), rotation) : utils::Random::randInt() % action_features.size());
    action_features[action_id] = 1.0f;
    return action_features;
}

std::vector<float> LeapFrogEnvLoader::getValue(const int pos) const
{
    if (pos >= static_cast<int>(action_pairs_.size())) { return {0.0f}; }
    auto it = action_pairs_[pos].second.find("V");
    if (it == action_pairs_[pos].second.end() || it->second.empty()) { return {0.0f}; }
    return {std::stof(it->second)};
}

std::vector<float> LeapFrogEnvLoader::getReward(const int pos) const
{
    if (pos >= static_cast<int>(action_pairs_.size())) { return {0.0f}; }
    auto it = action_pairs_[pos].second.find("R");
    if (it == action_pairs_[pos].second.end() || it->second.empty()) { return {0.0f}; }
    return {std::stof(it->second)};
}

} // namespace minizero::env::leapfrog