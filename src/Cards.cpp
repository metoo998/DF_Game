#include "Cards.h"

CardCatalog::CardCatalog() {
  cards_ = {
      {1, "恒星广播(合作)", 0, CardCategory::Broadcast, EffectType::TypeI, "合作", "",
       "以恒星为超级天线，向宇宙发布恒星及功率的电磁波", "", 1, 0},
      {101, "恒星广播(伪装)", 0, CardCategory::Broadcast, EffectType::TypeI, "伪装", "",
       "以恒星为超级天线，向宇宙发布恒星及功率的电磁波", "", 1, 0},
      {2, "宇宙广播(合作)", 1, CardCategory::Broadcast, EffectType::TypeI, "合作", "",
       "以简并态物质作为引力波振动弦，向宇宙发射引力波", "", 2, 0},
      {102, "宇宙广播(伪装)", 1, CardCategory::Broadcast, EffectType::TypeI, "伪装", "",
       "以简并态物质作为引力波振动弦，向宇宙发射引力波", "", 2, 0},
      {3, "超距广播(合作)", 1, CardCategory::Broadcast, EffectType::TypeI, "合作", "",
       "利用宇宙的膜结构，在高位空间的投影产生信号的超距通讯", "", -1, 0},
      {103, "超距广播(伪装)", 1, CardCategory::Broadcast, EffectType::TypeI, "伪装", "",
       "利用宇宙的膜结构，在高位空间的投影产生信号的超距通讯", "", -1, 0},
      {4, "太阳能阵列", 2, CardCategory::Energy, EffectType::TypeIII, "", "每回合开始时，能量+1",
       "建设太阳能风帆阵列以吸收恒星辐射的能量", "依赖恒星", -1, 0},
      {5, "聚变反应堆", 3, CardCategory::Energy, EffectType::TypeIII, "", "每回合开始时，能量+1",
       "利用可控核聚变产生能量", "不依赖恒星", -1, 0},
      {6, "反物质引擎", 6, CardCategory::Energy, EffectType::TypeIII, "", "每回合开始时，能量+2",
       "利用反物质湮灭产生能量", "不依赖恒星", -1, 0},
      {7, "戴森球", 6, CardCategory::Energy, EffectType::TypeIII, "", "每回合开始时，能量+3",
       "完全包围恒星并获得其绝大部分能量输出的巨型结构", "依赖恒星，每个星系只能建造 1 个", -1, 0},
      {8, "掩体星环", 6, CardCategory::Defense, EffectType::TypeIII, "", "可在等级2及以下的打击中幸存",
       "在巨行星背阳面建设的太空城，可躲避光粒打击引发的恒星爆发", "", -1, 0},
      {9, "量子幽灵", 10, CardCategory::Defense, EffectType::TypeIII, "", "可在等级3及以下的打击中幸存",
       "文明个体全部量子化后进入量子态，以概率云形式散开，不再需要以实体形式存在", "", -1, 0},
      {10, "光速飞船", 10, CardCategory::Defense, EffectType::TypeIII, "", "可跃迁至随机其他星系，不能携带能量和建造牌，且只能使用1次",
       "操纵时空曲率从而可以达到光速的飞船，可逃离任意打击", "", -1, 0},
      {11, "热核打击", 4, CardCategory::Strike, EffectType::TypeI, "", "打击无特殊效果",
       "向目标行星发射恒星级核弹，造成毁灭性打击", "", -1, 1},
      {12, "光粒打击", 6, CardCategory::Strike, EffectType::TypeI, "", "无论是否被防御，均毁灭目标恒星",
       "将一个质量点加速至接近光速，使被其撞击的恒星爆发", "", -1, 2},
      {13, "反物质打击", 8, CardCategory::Strike, EffectType::TypeI, "", "无论是否被防御，均毁灭目标恒星及所有建造牌",
       "反物质导弹到达目标星系时，会发生完全的质能转换，湮灭一切物质", "", -1, 3},
      {14, "科技锁死", 4, CardCategory::Strike, EffectType::TypeI, "", "打击生效时，目标星系玩家需弃掉手中所有建造牌，不影响其生存",
       "将微观粒子雕刻成超级计算机，干扰目标星系的基础物理研究", "", -1, 0},
      {15, "降维打击", 10, CardCategory::Strike, EffectType::TypeI, "", "彻底清除目标星系",
       "发射二向箔将目标星系空间维度降低，进行彻底“清理”", "", -1, 0},
      {16, "监听基地", 2, CardCategory::Special, EffectType::TypeIII, "", "所在星系接收广播后可不做回应",
       "\"不要回答、不要回答、不要回答\"", "不触发广播回应效果", -1, 0},
      {17, "和谐之眼", 15, CardCategory::Building, EffectType::TypeIII, "", "每回合产能7，无法被摧毁",
       "此刻进化为时间文明（TypeIII状态，觉醒技能时间干涉）", "需要摧毁该星系的恒星", -1, 0},
      {18, "星际远征", 4, CardCategory::Strike, EffectType::TypeI, "", "发动战争并触发能量比拼与占领规则",
       "消耗基础能量+额外能量，触发星球战争与占领机制", "额外能量选项：0/5/10/20", -1, 0},
      {19, "时间干涉", 21, CardCategory::Skill, EffectType::TypeII, "", "觉醒技能，出牌阶段选择目标星系使其消失",
       "对有文明的星系使用无视一切抹除并重置他人状态；对无文明/殖民地无效", "觉醒于和谐之眼", -1, 0},
      {20, "反击", 0, CardCategory::Skill, EffectType::TypeIII, "", "投降方挑战占领方以收回控制权",
       "若挑战时能量高于占领方则结束占领并收回建筑控制权", "投降方自动获得", -1, 0},
  };
}

const CardDefinition* CardCatalog::findById(int id) const {
  for (const auto& card : cards_) {
    if (card.id == id) {
      return &card;
    }
  }
  return nullptr;
}
