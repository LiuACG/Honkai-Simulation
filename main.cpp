#include <iostream>
#include <random>
#include <thread>
#include <utility>

#define ENABLE_LOG 0

#if defined(ENABLE_LOG) && (ENABLE_LOG == 1)
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

static float GetRandom() {
    static thread_local std::mt19937 re(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> rnd(0.f, 1.f);
    return rnd(re);
}

class Player {
public:
    enum class AttackResult {
        ATTACKER_DEAD,
        DEFENDER_DEAD,
        ALL_ALIVE,
    };

    Player() = default;

    Player(const int hit, const int def, const int atk, const int spd, std::string name)
        : name(std::move(name)), hit(hit), def(def), atk(atk), spd(spd) {
        if (name.find('&') != std::string::npos)
            is_group = true;
    }

    Player(const Player& other) = default;
    Player(Player&& other) noexcept = default;
    Player& operator=(const Player& other) = default;
    Player& operator=(Player&& other) noexcept = default;

    virtual ~Player() = default;

    virtual AttackResult Attack(int round, Player& defender) = 0;

    virtual AttackResult DoAtk(const int round, Player& attacker, const int atk) {
        const int acc_atk = attacker.IsHit() ? std::max(0, atk) : 0;
        hit -= acc_atk;
        LOG("回合%d %s 对 %s 普攻，造成%d点伤害，%s 剩余血量 %d\n", round, attacker.name.c_str(), name.c_str(), acc_atk, name.c_str(), hit);
        return hit <= 0 ? AttackResult::DEFENDER_DEAD : AttackResult::ALL_ALIVE;
    }

    virtual AttackResult DoUlt(const int round, Player& attacker, const int atk, const std::string& skill_name, const bool force_hit = false) {
        const int acc_atk = force_hit || attacker.IsHit() ? atk : 0;
        hit -= acc_atk;
        LOG("回合%d %s 使用了技能：【%s】对 %s 造成%d点伤害，%s 剩余血量 %d\n", round, attacker.name.c_str(), skill_name.c_str(), name.c_str(), acc_atk, name.c_str(), hit);
        return hit <= 0 ? AttackResult::DEFENDER_DEAD : AttackResult::ALL_ALIVE;
    }

    [[nodiscard]] bool IsHit() const { return GetRandom() <= hit_rate; }

    [[nodiscard]] bool RefreshSkillAvailableStatus() {
        if (buff_charm == 0)
            return true;
        return buff_charm-- == 0;
    }

    std::string name;

    int hit = 0;
    int def = 0;
    int atk = 0;
    int spd = 0;

    int buff_self = 0;
    int buff_opponent = 0;
    int buff_charm = 0;

    bool is_group = false;

    float hit_rate = 1.f;
};

class Kiana final : public Player {
public:
    Kiana() : Player(100, 11, 24, 23, "琪亚娜") {}

    AttackResult Attack(const int round, Player& defender) override {
        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        if (round % ATK_EVERY_ROUND == 0) {
            const auto result = defender.DoUlt(round, *this, atk + defender.def, "吃我一矛！");
            if (result != AttackResult::ALL_ALIVE)
                return result;

            if (GetRandom() <= 0.35f) {
                LOG("回合%d %s 因为【音浪~太强~】技能效果进入了眩晕状态\n", round, name.c_str());
                buff_self = 1;
            }
        } else {
            const auto result = defender.DoAtk(round, *this, atk - defender.def);
            if (result != AttackResult::ALL_ALIVE)
                return result;
        }

        return AttackResult::ALL_ALIVE;
    }

private:
    enum { ATK_EVERY_ROUND = 2 };
};

class Mei final : public Player {
public:
    Mei() : Player(100, 12, 22, 30, "芽衣") {}

    AttackResult Attack(const int round, Player& defender) override {
        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        if (round % ATK_EVERY_ROUND == 0) {
            for (int i = 0; i < 5; ++i) {
                const auto result = defender.DoUlt(round, *this, 3, "雷电家的龙女仆");
                if (result != AttackResult::ALL_ALIVE)
                    return result;
            }
        } else {
            const auto result = defender.DoAtk(round, *this, atk - defender.def);
            if (result != AttackResult::ALL_ALIVE)
                return result;

            if (GetRandom() <= 0.3f) {
                LOG("回合%d %s 使用了技能：【崩坏世界的歌姬】麻痹对方一回合\n", round, name.c_str());
                defender.buff_opponent = 1;
            }
        }

        return AttackResult::ALL_ALIVE;
    }

private:
    enum { ATK_EVERY_ROUND = 2 };
};

class Bronya final : public Player {
public:
    Bronya() : Player(100, 10, 21, 20, "布洛妮娅") {}

    AttackResult Attack(const int round, Player& defender) override {
        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        if (round % ATK_EVERY_ROUND == 0) {
            const auto result = defender.DoUlt(round, *this, static_cast<int>(GetRandom() * 99.f + 1.f), "摩托拜客哒！");
            if (result != AttackResult::ALL_ALIVE)
                return result;
        } else {
            const auto result = defender.DoAtk(round, *this, atk - defender.def);
            if (result != AttackResult::ALL_ALIVE)
                return result;
        }

        return AttackResult::ALL_ALIVE;
    }

private:
    enum { ATK_EVERY_ROUND = 3 };
};

class Himeko final : public Player {
public:
    Himeko() : Player(100, 9, 23, 12, "姬子") {
        if (is_group) {
            LOG("回合0 %s 使用了技能：【真爱不死】伤害提升100%%\n", name.c_str());
        }
    }

    AttackResult Attack(const int round, Player& defender) override {
        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        const int gan = is_group ? 2 : 1;

        if (round % ATK_EVERY_ROUND == 0) {
            if (GetRandom() <= 0.35f) {
                LOG("回合%d %s 使用了技能：【干杯，朋友】攻击力提升了100%%，命中率下降35%%\n", round, name.c_str());
                atk *= 2;
                hit_rate = std::max(0.f, hit_rate - 0.35f);
            }
        }

        return defender.DoAtk(round, *this, (atk - defender.def) * gan);
    }

private:
    enum { ATK_EVERY_ROUND = 2 };
};

class Rita final : public Player {
public:
    Rita() : Player(100, 11, 26, 17, "丽塔") {}

    AttackResult Attack(const int round, Player& defender) override {
        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        if (round % ATK_EVERY_ROUND == 0) {
            LOG("回合%d %s 使用了技能：【完美心意】使 %s 回复了4点血量并陷入两个回合的魅惑阶段，伤害永久减低60%%\n", round, name.c_str(), defender.name.c_str());

            defender.hit = std::min(100, defender.hit + 4);
            defender.buff_charm = 2;
            is_skill_activate_ = true;
        } else {
            int current_round_atk = atk;
            if (GetRandom() < 0.35f) {
                current_round_atk = std::max(0, current_round_atk - 3);
                defender.atk = std::max(0, defender.atk - 4);
                LOG("回合%d %s 使用了技能：【女仆的温柔清理】使 %s 攻击力永久下降4点\n", round, name.c_str(), defender.name.c_str());
            }

            const auto result = defender.DoAtk(round, *this, current_round_atk - defender.def);
            if (result != AttackResult::ALL_ALIVE)
                return result;
        }

        return AttackResult::ALL_ALIVE;
    }

    AttackResult DoAtk(const int round, Player& attacker, const int atk) override {
        return Player::DoAtk(round, attacker, is_skill_activate_ ? static_cast<int>(roundf(static_cast<float>(atk) * 0.4f)) : atk);
    }

    AttackResult DoUlt(const int round, Player& attacker, const int atk, const std::string& skill_name, const bool force_hit) override {
        return Player::DoUlt(round, attacker, is_skill_activate_ ? static_cast<int>(roundf(static_cast<float>(atk) * 0.4f)) : atk, skill_name, force_hit);
    }

private:
    enum { ATK_EVERY_ROUND = 4 };
    bool is_skill_activate_ = false;
};

class Sakura final : public Player {
public:
    Sakura() : Player(100, 9, 20, 18, "八重樱&卡莲") {}

    AttackResult Attack(const int round, Player& defender) override {
        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        if (GetRandom() <= 0.3f) {
            hit = std::min(100, hit + 25);
            LOG("回合%d %s 使用了技能：【八重樱的饭团】吃下了饭团，回复25点血量\n", round, name.c_str());
        }

        if (round % ATK_EVERY_ROUND == 0) {
            const auto result = defender.DoUlt(round, *this, 25, "卡莲的饭团");
            if (result != AttackResult::ALL_ALIVE)
                return result;
        } else {
            const auto result = defender.DoAtk(round, *this, atk - defender.def);
            if (result != AttackResult::ALL_ALIVE)
                return result;
        }

        return AttackResult::ALL_ALIVE;
    }

private:
    enum { ATK_EVERY_ROUND = 2 };
};

class Corvus final : public Player {
public:
    Corvus() : Player(100, 14, 23, 14, "渡鸦") {}

    AttackResult Attack(const int round, Player& defender) override {
        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        float gan = 1.f;
        if (dynamic_cast<Kiana*>(&defender) != nullptr || GetRandom() <= 0.25f) {
            LOG("回合%d %s 使用了技能：【不是针对你】伤害提升了25%%\n", round, name.c_str());
            gan += 0.25f;
        }

        if (round % ATK_EVERY_ROUND == 0) {
            for (int i = 0; i < 7; ++i) {
                const auto result = defender.DoUlt(round, *this, static_cast<int>(std::round(static_cast<float>(16 - defender.def) * gan)), "别墅小岛");
                if (result != AttackResult::ALL_ALIVE)
                    return result;
            }
        } else {
            const auto result = defender.DoAtk(round, *this, static_cast<int>(std::round(static_cast<float>(atk - defender.def) * gan)));
            if (result != AttackResult::ALL_ALIVE)
                return result;
        }

        return AttackResult::ALL_ALIVE;
    }

private:
    enum { ATK_EVERY_ROUND = 3 };
};

class Theresa final : public Player {
public:
    Theresa() : Player(100, 12, 19, 22, "德莉莎") {}

    AttackResult Attack(const int round, Player& defender) override {
        const bool skill_available = RefreshSkillAvailableStatus();

        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        if (skill_available && round % ATK_EVERY_ROUND == 0) {
            for (int i = 0; i < 5; ++i) {
                const auto result = defender.DoUlt(round, *this, 16 - defender.def, "在线踢人");
                if (result != AttackResult::ALL_ALIVE)
                    return result;
            }
        } else {
            const auto result = defender.DoAtk(round, *this, atk - defender.def);
            if (result != AttackResult::ALL_ALIVE)
                return result;

            if (skill_available && GetRandom() <= 0.3f) {
                LOG("回合%d %s 使用了技能：【血犹大第一可爱】降低了对方5点的防御\n", round, name.c_str());
                defender.def = std::max(0, defender.def - 5);
            }
        }

        return AttackResult::ALL_ALIVE;
    }

private:
    enum { ATK_EVERY_ROUND = 3 };
};

class Olenyeva final : public Player {
public:
    Olenyeva() : Player(100, 10, 18, 10, "萝莎莉娅&莉莉娅") {}

    AttackResult Attack(const int round, Player& defender) override {
        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        if (boom_ > 0) {
            --boom_;
            defender.DoUlt(round, *this, GetRandom() <= 0.5f ? 233 : 50, "变成星星吧！", true);
        }

        return defender.DoAtk(round, *this, atk - defender.def);
    }

    AttackResult DoAtk(const int round, Player& attacker, const int atk) override {
        const int acc_atk = attacker.IsHit() ? std::max(0, atk) : 0;
        hit -= acc_atk;
        LOG("回合%d %s 对 %s 普攻，造成%d点伤害，%s 剩余血量 %d\n", round, attacker.name.c_str(), name.c_str(), acc_atk, name.c_str(), hit);

        if (hit <= 0) {
            if (resurrection_stone_ > 0) {
                LOG("回合%d %s 使用了技能：【96度生命之水】并恢复至20点血量\n", round, name.c_str());
                hit = 20;
                boom_ = 1;
                --resurrection_stone_;
                return AttackResult::ALL_ALIVE;
            }
            return AttackResult::DEFENDER_DEAD;
        }
        return AttackResult::ALL_ALIVE;
    }

    AttackResult DoUlt(const int round, Player& attacker, const int atk, const std::string& skill_name, const bool force_hit) override {
        const int acc_atk = force_hit || attacker.IsHit() ? atk : 0;
        hit -= acc_atk;
        LOG("回合%d %s 使用了技能：【%s】对 %s 造成%d点伤害，%s 剩余血量 %d\n", round, attacker.name.c_str(), skill_name.c_str(), name.c_str(), acc_atk, name.c_str(), hit);

        if (hit <= 0) {
            if (resurrection_stone_ > 0) {
                LOG("回合%d %s 使用了技能：【96度生命之水】并恢复至20点血量\n", round, name.c_str());
                hit = 20;
                boom_ = 1;
                --resurrection_stone_;
                return AttackResult::ALL_ALIVE;
            }
            return AttackResult::DEFENDER_DEAD;
        }
        return AttackResult::ALL_ALIVE;
    }

private:
    int boom_ = 0;
    int resurrection_stone_ = 1;
};

class Seele final : public Player {
public:
    Seele() : Player(100, 13, 23, 26, "希儿") {}

    AttackResult Attack(const int round, Player& defender) override {
        status_ ^= 1;

        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        if (status_ == 0) {
            const int origin_hit = hit;
            hit = std::min(100, hit + static_cast<int>(GetRandom() * 14 + 1));
            def += 5;
            atk -= 10;
            LOG("回合%d 希尔转变为白形态，防御力上升了，攻击力下降了，回复了%d点血量\n", round, hit - origin_hit);
        } else {
            def -= 5;
            atk += 10;
            LOG("回合%d 希尔转变为黑形态，攻击力上升了，防御力下降了\n", round);
        }

        return defender.DoAtk(round, *this, atk - defender.def);
    }

private:
    int status_ = 0; // 0 是白希，1是黑希
};

class Durandal final : public Player {
public:
    Durandal() : Player(100, 10, 19, 15, "幽兰黛尔&史丹") {}

    AttackResult Attack(const int round, Player& defender) override {
        atk += 3;

        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        return defender.DoAtk(round, *this, atk - defender.def);
    }

    AttackResult DoUlt(int round, Player& attacker, const int atk, const std::string& name, const bool force_hit) override {
        if (GetRandom() < 0.16f) {
            attacker.hit -= 30;
            return attacker.hit <= 0 ? AttackResult::ATTACKER_DEAD : AttackResult::ALL_ALIVE;
        }
        hit -= atk;
        return hit <= 0 ? AttackResult::DEFENDER_DEAD : AttackResult::ALL_ALIVE;
    }
};

class FuHua final : public Player {
public:
    FuHua() : Player(100, 15, 17, 16, "符华") {}

    AttackResult Attack(const int round, Player& defender) override {
        if (buff_self || buff_opponent) {
            if (buff_self) --buff_self;
            if (buff_opponent) --buff_opponent;

            return AttackResult::ALL_ALIVE;
        }

        if (round % ATK_EVERY_ROUND == 0) {
            const auto result = defender.DoUlt(round, *this, 18, "形之笔墨");
            if (result != AttackResult::ALL_ALIVE)
                return result;

            defender.hit_rate = std::max(0.f, defender.hit_rate - 0.25f);
        } else {
            const auto result = defender.DoAtk(round, *this, atk);
            if (result != AttackResult::ALL_ALIVE)
                return result;
        }

        return AttackResult::ALL_ALIVE;
    }

private:
    enum { ATK_EVERY_ROUND = 3 };
};

void Simulation() {
#if defined(ENABLE_LOG) && (ENABLE_LOG == 1)
    const int times = 1;
#else
    const int times = 1000000;
#endif

    int p0_win = 0;
    int p1_win = 0;

    for (int i = 0; i < times; ++i) {
        // Olenyeva p0;
        // Kiana p1;

        Corvus p0;
        Mei p1;

        // Sakura p0;
        // Seele p1;

        for (int round = 1; ; ++round) {
            const auto p1_status = p1.Attack(round, p0);
            if (p1_status != Player::AttackResult::ALL_ALIVE) {
                if (p1_status == Player::AttackResult::DEFENDER_DEAD) p1_win++;
                if (p1_status == Player::AttackResult::ATTACKER_DEAD) p0_win++;
                break;
            }

            const auto p0_status = p0.Attack(round, p1);
            if (p0_status != Player::AttackResult::ALL_ALIVE) {
                if (p0_status == Player::AttackResult::DEFENDER_DEAD) p0_win++;
                if (p0_status == Player::AttackResult::ATTACKER_DEAD) p1_win++;
                break;
            }
        }
    }

    printf("P0 Win Ratio: %.4f%%\n", static_cast<double>(p0_win) / static_cast<double>(times) * 100.);
}

void Sim2() {
    const float a = 1.9f;
    const float b = 3.4f;
    const float p = 0.708430f;

    const int times = 1000000;
    const int total_tickets = 10;
    for (int i = 0; i <= total_tickets; ++i) {
        int cnt = 0;
        int sum = 0;
        for (int j = 0; j < times; ++j) {
            int tickets_avd;
            if (GetRandom() <= p) {
                tickets_avd = static_cast<int>(ceil(static_cast<float>(i) * a));
            } else {
                tickets_avd = static_cast<int>(ceil(static_cast<float>(total_tickets - i) * b));
            }

            cnt += (tickets_avd - total_tickets) < 0;
            sum += tickets_avd;
        }
        printf("E(%3d) = %8.4f, %8.4f - %8.4f, LOSS: %.4f%%\n", i, static_cast<float>(sum) / static_cast<float>(times), ceil(static_cast<float>(i) * a), ceil(static_cast<float>(total_tickets - i) * b), static_cast<float>(cnt) / static_cast<float>(times));
    }
}

int
main(int argc, char* argv[]) {
    Simulation();

    return 0;
}
