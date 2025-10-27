#pragma once

#include "base_env.h"
#include "configuration.h"
#include <bitset>
#include <string>
#include <vector>

namespace minizero::env::leapfrog {

const std::string kLeapFrogName = "leapfrog";
const int kLeapFrogNumPlayer = 2;
const int kLeapFrogBoardSize = 8;
const int kNumDirections = 8; // 4 orthogonal + 4 diagonal directions

typedef std::bitset<kLeapFrogBoardSize * kLeapFrogBoardSize> LeapFrogBitboard;

class LeapFrogAction : public BaseAction {
public:
    LeapFrogAction() : BaseAction() {}
    LeapFrogAction(int action_id, Player player) : BaseAction(action_id, player) {}
    LeapFrogAction(const std::vector<std::string>& action_string_args) : BaseAction()
    {
        action_id_ = actionStringToID(action_string_args);
        player_ = charToPlayer(action_string_args[0][0]);
    }
    inline Player nextPlayer() const override { return getNextPlayer(getPlayer(), kLeapFrogNumPlayer); }
    inline std::string toConsoleString() const override { return actionIDtoString(action_id_); }
    inline int getFromPos() const { return getFromPos(action_id_); }
    inline int getDestPos() const { return getDestPos(action_id_); }

private:
    int board_size_ = minizero::config::env_board_size;

    int coordinateToID(int c1, int r1, int c2, int r2) const;
    int charToPos(char c) const;
    int getFromPos(int action_id) const;
    int getDestPos(int action_id) const;
    int actionStringToID(const std::vector<std::string>& action_string_args) const;
    std::string actionIDtoString(int action_id) const;
};

class LeapFrogEnv : public BaseBoardEnv<LeapFrogAction> {
public:
    LeapFrogEnv() { reset(); }
    void reset() override;
    bool act(const LeapFrogAction& action) override;
    bool act(const std::vector<std::string>& action_string_args) override;
    std::vector<LeapFrogAction> getLegalActions() const override;
    bool isLegalAction(const LeapFrogAction& action) const override;
    bool isTerminal() const override;
    float getReward() const override { return 0.0f; }
    float getEvalScore(bool is_resign = false) const override;
    std::vector<float> getFeatures(utils::Rotation rotation = utils::Rotation::kRotationNone) const override;
    std::vector<float> getActionFeatures(const LeapFrogAction& action, utils::Rotation rotation = utils::Rotation::kRotationNone) const override;

    inline int getNumInputChannels() const override { return 18; }
    inline int getNumActionFeatureChannels() const override { return kNumDirections; }
    inline int getInputChannelHeight() const override { return getBoardSize(); }
    inline int getInputChannelWidth() const override { return getBoardSize(); }
    inline int getHiddenChannelHeight() const override { return getBoardSize(); }
    inline int getHiddenChannelWidth() const override { return getBoardSize(); }
    inline int getPolicySize() const override { return kNumDirections * getBoardSize() * getBoardSize(); }
    std::string toString() const override;
    inline std::string name() const override { return kLeapFrogName; }
    inline int getNumPlayer() const override { return kLeapFrogNumPlayer; }

    inline int getRotatePosition(int position, utils::Rotation rotation) const override { return position; };
    inline int getRotateAction(int action_id, utils::Rotation rotation) const override { return action_id; };

private:
    Player eval() const;
    std::string getCoordinateString() const;
    Player getPlayerAtBoardPos(int position) const;

    GamePair<LeapFrogBitboard> bitboard_;
    std::vector<GamePair<LeapFrogBitboard>> bitboard_history_;
};

class LeapFrogEnvLoader : public BaseBoardEnvLoader<LeapFrogAction, LeapFrogEnv> {
public:
    std::vector<float> getActionFeatures(const int pos, utils::Rotation rotation = utils::Rotation::kRotationNone) const override;
    std::vector<float> getValue(const int pos) const override;
    std::vector<float> getReward(const int pos) const override;
    inline std::string name() const override { return kLeapFrogName; }
    inline int getPolicySize() const override { return kNumDirections * getBoardSize() * getBoardSize(); }
    inline int getRotatePosition(int position, utils::Rotation rotation) const override { return position; };
    inline int getRotateAction(int action_id, utils::Rotation rotation) const override { return action_id; };
};

} // namespace minizero::env::leapfrog