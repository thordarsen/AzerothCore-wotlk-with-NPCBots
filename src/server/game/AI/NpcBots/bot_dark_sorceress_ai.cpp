#include "bot_ai.h"
#include "bot_GridNotifiers.h"
#include "botspell.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "TemporarySummon.h"
/*
modelid .morph target 23178 or
chaosblast Chaos Blast soell 59912
Vampiric embrace 15286
shadow mend spell=25531
SHOOT_WAND                          = 5019,

Assert(container(size)) in 
Dark Ranger NpcBot (by Trickerer onlysuffering@gmail.com)
Description:
A former ranger of Quel'thalas forcibly raised from the dead (Warcraft III tribute)
Specifics:
Spell damage taken reduced by 35%, partially immune to control effects, leather/cloth armor,
deals physical/spellshadow damage, spell power bonus: 50% intellect. Main attribute: Agility
Abilities:
1) Silence. Silences an enemy and up to 4 nearby targets for 8 seconds, 15 seconds cooldown
2) Black Arrow. Fires a cursed arrow dealing 150% weapon damage and additional spellshadow damage over time.
If affected target dies from Dark Ranger\'s damage, a Dark Minion will spawn from the corpse
(maximum 5 Minions, 80 seconds duration, only works on humanoids, beasts and dragonkin),
skeleton level depends on level of the killed unit
Deals five times more damage if target is under 20% health
3) Drain Life. Drains health from an enemy every second for 5 seconds (6 ticks),
healing Dark Ranger for 200% of the drained amount
4) Charm NIY
5ex) Auto Shot. A hunter auto shot ability since dark ranger is purely ranged and only uses bows.
Complete - 75%
TODO: Charm
*/

enum DarkSorceressBaseSpells
{
    CHAOS_BLAST_1                       = SPELL_CHAOS_BLAST,
    SHADOWFLAME_SPIRAL_1                = SPELL_SHADOWFLAME_SPIRAL,
    MIND_FLAY_1                         = 15407,
    VAMPIRIC_EMBRACE_1                  = 15286,
    SHADOW_MEND_1                       = 25531,
    SPELL_LOCK_1                        = 19244
};
enum DarkSorceressPassives
{
    MISERY                              = 33193,
    MEDITATION                          = 14777,
};
enum DarkSorceressSpecial
{
    SPELL_THREAT_MOD                    = 31745, //Salvation
};

static const uint32 Darksorceress_spells_damage_arr[] =
{ CHAOS_BLAST_1, MIND_FLAY_1, SHADOWFLAME_SPIRAL_1 };

static const uint32 Darksorceress_spells_cc_arr[] =
{ SPELL_LOCK_1 };

static const uint32 Darksorceress_spells_heal_arr[] =
{ SHADOW_MEND_1 };

static const uint32 Darksorceress_spells_support_arr[] =
{ VAMPIRIC_EMBRACE_1 };

static const std::vector<uint32> Darksorceress_spells_damage(FROM_ARRAY(Darksorceress_spells_damage_arr));
static const std::vector<uint32> Darksorceress_spells_cc(FROM_ARRAY(Darksorceress_spells_cc_arr));
static const std::vector<uint32> Darksorceress_spells_heal(FROM_ARRAY(Darksorceress_spells_heal_arr));
static const std::vector<uint32> Darksorceress_spells_support(FROM_ARRAY(Darksorceress_spells_support_arr));

class dark_sorceress_bot : public CreatureScript
{
public:
    dark_sorceress_bot() : CreatureScript("dark_sorceress_bot") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new dark_sorceress_botAI(creature);
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        return creature->GetBotAI()->OnGossipHello(player, 0);
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
    {
        if (bot_ai* ai = creature->GetBotAI())
            return ai->OnGossipSelect(player, creature, sender, action);
        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, char const* code)
    {
        if (bot_ai* ai = creature->GetBotAI())
            return ai->OnGossipSelectCode(player, creature, sender, action, code);
        return true;
    }

    struct dark_sorceress_botAI : public bot_ai
    {
        dark_sorceress_botAI(Creature* creature) : bot_ai(creature)
        {
            _botclass = BOT_CLASS_DARK_SORCERESS;

            InitUnitFlags();

            //dark ranger immunities
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_POSSESS, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CHARM, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_SHAPESHIFT, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_TRANSFORM, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_DISARM, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_DISARM_OFFHAND, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_DISARM_RANGED, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_FEAR, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISARM, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_TURN, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SLEEP, true);
        }

        bool doCast(Unit* victim, uint32 spellId)
        {
            if (CheckBotCast(victim, spellId) != SPELL_CAST_OK)
                return false;

            return bot_ai::doCast(victim, spellId);
        }

        void StartAttack(Unit* u, bool force = false)
        {
            if (!bot_ai::StartAttack(u, force))
                return;
            GetInPosition(force, u);
        }

        void EnterEvadeMode(EvadeReason why = EVADE_REASON_OTHER) override { bot_ai::EnterEvadeMode(why); }
        void MoveInLineOfSight(Unit* u) override { bot_ai::MoveInLineOfSight(u); }
        void JustDied(Unit* u) override { bot_ai::JustDied(u); }
        void DoNonCombatActions(uint32 /*diff*/) { }

        void KilledUnit(Unit* u) override
        {
            bot_ai::KilledUnit(u);
        }

        void Counter(uint32 diff)
        {
            if (Rand() > 55)
                return;

            if (IsSpellReady(SPELL_LOCK_1, diff))
            {
                Unit* target = FindCastingTarget(CalcSpellMaxRange(SPELL_LOCK_1), 0, SPELL_LOCK_1);
                if (target && doCast(target, GetSpell(SPELL_LOCK_1)))
                    return;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!GlobalUpdate(diff))
                return;

            DoVehicleActions(diff);
            if (!CanBotAttackOnVehicle())
                return;

            //CheckDrainLife(diff);

            if (IsPotionReady())
            {
                if (me->GetPower(POWER_MANA) < 100)//DRAINLIFE_COST)
                    DrinkPotion(true);
                else if (GetHealthPCT(me) < 30)
                    DrinkPotion(false);
            }
            BuffAndHealGroup(diff);
            if (ProcessImmediateNonAttackTarget())
                return;

            if (!CheckAttackTarget())
            {
                me->InterruptSpell(CURRENT_AUTOREPEAT_SPELL);


                return;
            }

            if (IsCasting())
                return;

            DoAttack(diff);
        }

        void DoAttack(uint32 diff)
        {
            Unit* mytar = opponent ? opponent : disttarget ? disttarget : nullptr;
            if (!mytar)
                return;

            StartAttack(mytar, IsMelee());

            Counter(diff);

            MoveBehind(mytar);

            float dist = me->GetDistance(mytar);
            float maxRangeLong = 30.f;
            if (IsSpellReady(CHAOS_BLAST_1, diff) && doCast(mytar, GetSpell(CHAOS_BLAST_1)))
                return;
            if (IsSpellReady(SHADOWFLAME_SPIRAL_1, diff) && doCast(mytar, GetSpell(SHADOWFLAME_SPIRAL_1)))
                return;


            if (IsSpellReady(MIND_FLAY_1, diff) && doCast(mytar, GetSpell(MIND_FLAY_1)))
                return;
            // bool inpostion = !mytar->HasAuraType(SPELL_AURA_MOD_CONFUSE) || dist > maxRangeLong - 15.f;

            //Auto Shot
            if (Spell const* shot = me->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL))
            {
                if (shot->GetSpellInfo()->Id == SHOOT_WAND && shot->m_targets.GetUnitTarget() != mytar)
                    me->InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
            }
            else if (IsSpellReady(SHOOT_WAND, diff) && me->GetDistance(mytar) < 30 && GetEquips(BOT_SLOT_RANGED) &&
                doCast(mytar, SHOOT_WAND))
                return;

        }


        void ApplyClassSpellCritMultiplierAll(Unit const* /*victim*/, float& /*crit_chance*/, SpellInfo const* /*spellInfo*/, SpellSchoolMask /*schoolMask*/, WeaponAttackType /*attackType*/) const override
        {
        }

        void ApplyClassDamageMultiplierMeleeSpell(int32& damage, SpellNonMeleeDamage& damageinfo, SpellInfo const* spellInfo, WeaponAttackType /*attackType*/, bool /*iscrit*/) const override
        {
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            float flat_mod = 0.f;

            //2) apply bonus damage mods
            float pctbonus = 1.0f;

            damage = int32(damage * pctbonus + flat_mod);
        }

        void ApplyClassDamageMultiplierSpell(int32& damage, SpellNonMeleeDamage& damageinfo, SpellInfo const* spellInfo, WeaponAttackType /*attackType*/, bool iscrit) const override
        {
            //uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //uint8 lvl = me->GetLevel();
            float fdamage = float(damage);
            float flat_mod = 0.f;

            //2) apply bonus damage mods
            float pctbonus = 1.0f;
            if (iscrit) {pctbonus += 0.333f;}


            damage = int32(fdamage * pctbonus + flat_mod);
        }

        void ApplyClassEffectMods(SpellInfo const* spellInfo, uint8 effIndex, float& value) const override
        {
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //uint8 lvl = me->GetLevel();
            float pctbonus = 1.0f;

            value = value * pctbonus;
        }

        void OnClassSpellGo(SpellInfo const* /*spellInfo*/) override
        {
            //uint32 spellId = spellInfo->Id;
            //uint32 baseId = spellInfo->GetFirstRankSpell()->Id;

            //Rapid Killing: use up buff manually
            //if (baseId == AIMED_SHOT_1 || baseId == ARCANE_SHOT_1 || baseId == CHIMERA_SHOT_1)
            //{
            //    if (AuraEffect const* rapi = me->GetAuraEffect(RAPID_KILLING_BUFF, 0))
            //        if (rapi->IsAffectedOnSpell(spellInfo))
            //            me->RemoveAura(RAPID_KILLING_BUFF);
            //}
        }

        void SpellHitTarget(Unit* wtarget, SpellInfo const* spell) override
        {
            Unit* target = wtarget->ToUnit();
            if (!target)
                return;

            if (target == me)
                return;

            //uint32 baseId = spell->GetFirstRankSpell()->Id;
            //uint8 lvl = me->GetLevel();

            //if (baseId == HUNTERS_MARK_1)
            //{
            //    //DarkRanger's Mark helper
            //    if (AuraEffect* mark = target->GetAuraEffect(spell->Id, 1, me->GetGUID()))
            //    {
            //        //Glyph of DarkRanger's Mark: +20% effect
            //        //Improved DarkRanger's Mark: +30% effect
            //        if (lvl >= 15)
            //            mark->ChangeAmount(mark->GetAmount() + mark->GetAmount() / 2);
            //        else if (lvl >= 10)
            //            mark->ChangeAmount(mark->GetAmount() * 13 / 10);
            //    }
            //}

            OnSpellHitTarget(target, spell);
        }

        void SpellHit(Unit* wcaster, SpellInfo const* spell) override
        {
            Unit* caster = wcaster->ToUnit();
            if (!caster)
                return;

            OnSpellHit(caster, spell);
        }

        void OnBotDamageDealt(Unit* victim, uint32 damage, CleanDamage const* /*cleanDamage*/, DamageEffectType /*damagetype*/, SpellInfo const* spellInfo) override
        {
            return;
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType damageType) override
        {
            bot_ai::DamageDealt(victim, damage, damageType);
        }

        void DamageTaken(Unit* u, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellSchoolMask /*schoolMask*/) override
        {
            if (!u)
                return;
            if (!u->IsInCombat() && !me->IsInCombat())
                return;
            OnOwnerDamagedBy(u);
        }

        void OwnerAttackedBy(Unit* u) override
        {
            if (!u)
                return;
            OnOwnerDamagedBy(u);
        }


        void SummonedCreatureDies(Creature* /*summon*/, Unit* /*killer*/) override
        {
            //TC_LOG_ERROR("entities.unit", "SummonedCreatureDies: %s's %s", me->GetName().c_str(), summon->GetName().c_str());
            //if (summon == botPet)
            //    botPet = nullptr;
        }

        float GetSpellAttackRange(bool longRange) const override
        {
            return longRange ? 28.f : 15.f;
        }

        uint32 GetAIMiscValue(uint32 data) const override
        {
            switch (data)
            {
                default:
                    return 0;
            }
        }

        void Reset() override
        {
            //for (uint8 i = 0; i != MAX_SPELL_SCHOOL; ++i)
            //    me->m_threatModifier[1] = 0.0f;

            DefaultInit();

            //threat mod
            if (Aura* threat = me->AddAura(SPELL_THREAT_MOD, me))
                threat->GetEffect(0)->ChangeAmount(-100);
        }
        bool HealTarget(Unit* target, uint32 diff) override
        {
            if (!target || !target->IsAlive() || target->GetShapeshiftForm() == FORM_SPIRITOFREDEMPTION || me->GetDistance(target) > 40)
                return false;
            uint8 hp = GetHealthPCT(target);
            bool pointed = IsPointedHealTarget(target);
            if (hp > 90 && !(pointed && me->GetMap()->IsRaid()) &&
                (!target->IsInCombat() || target->getAttackers().empty() || !IsTank(target) || !me->GetMap()->IsRaid()))
                return false;

            int32 hps = GetHPS(target);
            int32 xphp = target->GetHealth() + hps * 2.5f;
            int32 hppctps = int32(hps * 100.f / float(target->GetMaxHealth()));
            int32 xphploss = xphp > int32(target->GetMaxHealth()) ? 0 : abs(int32(xphp - target->GetMaxHealth()));
            int32 xppct = hp + hppctps * 2.5f;
            if (xppct >= 75 && hp >= 25 && !pointed)
                return false;


            if (IsCasting()) return false;

            Unit const* u = target->GetVictim();
            bool tanking = u && IsTank(target) && u->ToCreature() && u->ToCreature()->isWorldBoss();

            if (IsSpellReady(SHADOW_MEND_1, diff) && xphploss > _heals[SHADOW_MEND_1])
            {
                if (doCast(target, GetSpell(SHADOW_MEND_1)))
                    return true;
            }

            return false;
        }
        void ReduceCD(uint32 /*diff*/) override
        {
            //if (trapTimer > diff)                   trapTimer -= diff;
        }

        void InitPowers() override
        {
            me->SetPowerType(POWER_MANA);
        }

        void InitSpells() override
        {
            //uint8 lvl = me->GetLevel();
            InitSpellMap(SHADOW_MEND_1);
            InitSpellMap(CHAOS_BLAST_1);
            InitSpellMap(SHADOWFLAME_SPIRAL_1);
            
            InitSpellMap(MIND_FLAY_1);
            InitSpellMap(SPELL_LOCK_1);
            InitSpellMap(VAMPIRIC_EMBRACE_1);
        }

        void ApplyClassPassives() const override
        {
            uint8 level = master->GetLevel();
            RefreshAura(MISERY, level >= 50 ? 1 : 0);
            RefreshAura(MEDITATION, level >= 20 ? 1 : 0);
        }

        std::vector<uint32> const* GetDamagingSpellsList() const override
        {
            return &Darksorceress_spells_damage;
        }
        std::vector<uint32> const* GetCCSpellsList() const override
        {
            return &Darksorceress_spells_cc;
        }
        std::vector<uint32> const* GetHealingSpellsList() const override
        {
            return &Darksorceress_spells_heal;
        }
        std::vector<uint32> const* GetSupportSpellsList() const override
        {
            return &Darksorceress_spells_support;
        }
    private:
        typedef std::unordered_map<uint32 /*baseId*/, int32 /*amount*/> HealMap;
        HealMap _heals;
    };
};

void AddSC_dark_sorceress_bot()
{
    new dark_sorceress_bot();
}
