#include "entity.h"
#include "rng.h"
#include "game.h"
#include "geometry.h"
#include "map.h"
#include <sstream>

Entity::Entity()
{
  uid = -1;
  posx = 15;
  posy = 15;
  action_points = 0;
  dead = false;
  killed_by_player = false;
}

Entity::~Entity()
{
}

std::string Entity::get_name()
{
  return "Nothing";
}

std::string Entity::get_name_to_player()
{
  return "Nothing";
}

std::string Entity::get_possessive()
{
  return "Nothing's";
}

std::string Entity::conjugate(const std::string &verb)
{
// Dumbest conjugation ever, but it should work for most cases!
// TODO: Special-case stuff?
  if (verb[ verb.length() - 1 ] == 's') {
    return verb + "es";
  }
  return verb + "s";
}

glyph Entity::get_glyph()
{
  return glyph();
}

Point Entity::get_position()
{
  return Point(posx, posy);
}

void Entity::die()
{
// TODO: Drop a corpse.
  for (int i = 0; i < inventory.size(); i++) {
    GAME.map->add_item( inventory[i], posx, posy );
  }
  for (int i = 0; i < items_worn.size(); i++) {
    GAME.map->add_item( items_worn[i], posx, posy );
  }
  if (weapon.is_real()) {
    GAME.map->add_item( weapon, posx, posy );
  }
}

void Entity::gain_action_points()
{
  action_points += get_speed();
}

int Entity::get_speed()
{
  return 100;
}

void Entity::take_turn()
{
}

Intel_level Entity::get_intelligence()
{
  return INTEL_PLANT;
}

bool Entity::has_sense(Sense_type sense)
{
  return false;
}

bool Entity::can_see(Map* map, int x, int y)
{
  if (!map || !has_sense(SENSE_SIGHT)) {
    return false;
  }
  return map->senses(posx, posy, x, y, SENSE_SIGHT);
}

bool Entity::can_move_to(Map* map, int x, int y)
{
  if (!map) {
    return false;
  }
  if (map->move_cost(x, y) == 0) {
    return false;
  }
  return true;
}

void Entity::move_to(Map* map, int x, int y)
{
  posx = x;
  posy = y;
  if (map) {
    action_points -= map->move_cost(x, y);
  }
}

void Entity::pause()
{
  action_points -= 100;
}

void Entity::set_activity(Player_activity_type type, int duration,
                          int primary_uid, int secondary_uid)
{
// TODO: Error or something if we have an activity?
  activity = Player_activity(type, duration, primary_uid, secondary_uid);
}

void Entity::use_ap(int amount)
{
  if (amount < 0) {
    return;
  }
  action_points -= amount;
}

void Entity::shift(int shiftx, int shifty)
{
  posx -= shiftx * SUBMAP_SIZE;
  posy -= shifty * SUBMAP_SIZE;
}

bool Entity::add_item(Item item)
{
  if (!item.is_real()) {
    return false;
  }
  if (item.combines()) {
    Item* added = ref_item_of_type(item.type);
    if (added) {
      return (*added).combine_with(item);
    }
  }
  inventory.push_back(item);
  return true;
}

Item* Entity::ref_item_uid(int uid)
{
  if (weapon.is_real() && weapon.get_uid() == uid) {
    return &weapon;
  }
  for (int i = 0; i < items_worn.size(); i++) {
    if (items_worn[i].get_uid() == uid) {
      return &(items_worn[i]);
    }
  }
  for (int i = 0; i < inventory.size(); i++) {
    if (inventory[i].get_uid() == uid) {
      return &(inventory[i]);
    }
  }
  return NULL;
}

Item Entity::get_item_of_type(Item_type *type)
{
  if (!type) {
    return Item();
  }
  for (int i = 0; i < inventory.size(); i++) {
    if (inventory[i].type == type) {
      return inventory[i];
    }
  }
// TODO: Weapon & armor?
  return Item();
}

Item* Entity::ref_item_of_type(Item_type *type)
{
  if (!type) {
    return NULL;
  }
  for (int i = 0; i < inventory.size(); i++) {
    if (inventory[i].type == type) {
      return &(inventory[i]);
    }
  }
// TODO: Weapon & armor?
  return NULL;
}

Item Entity::remove_item_uid(int uid, int count)
{
  Item ret = Item();
  int ret_count = count;
  if (weapon.type && weapon.get_uid() == uid) {
    if (count == 0) {
      ret_count = weapon.count;
    } else if (weapon.count < count) {
      ret_count = weapon.count;
    }
    weapon.count -= count;
    ret = weapon;
    ret.count = ret_count;
    if (weapon.count <= 0 || count == 0) {
      weapon = Item();
    }
    return ret;
  }
// Items_worn should never have a count.
  for (int i = 0; i < items_worn.size(); i++) {
    if (items_worn[i].get_uid() == uid) {
      ret = items_worn[i];
      ret.count = 1;
      items_worn.erase(items_worn.begin() + i);
      return ret;
    }
  }
  for (int i = 0; i < inventory.size(); i++) {
    if (inventory[i].get_uid() == uid) {
      if (count == 0) {
        ret_count = inventory[i].count;
      } else if (inventory[i].count < count) {
        ret_count = inventory[i].count;
      }
      ret = inventory[i];
      ret.count = ret_count;
      if (count == 0 || inventory[i].count <= 0) {
        inventory.erase(inventory.begin() + i);
      }
      return ret;
    }
  }
  return ret;
}

void Entity::wield_item_uid(int uid)
{
  for (int i = 0; i < inventory.size(); i++) {
    if (inventory[i].get_uid() == uid) {
      weapon = inventory[i];
      weapon.count = 1;
      if (inventory[i].count <= 1) {
        inventory.erase(inventory.begin() + i);
      } else {
        inventory[i].count--;
      }
      return;
    }
  }
}

void Entity::sheath_weapon()
{
  if (weapon.is_real()) {
    add_item(weapon);
  }
}

void Entity::wear_item_uid(int uid)
{
// TODO: Return a failure reason when attempting to wear something we're wearing
//       or a non-clothing item
  if (weapon.is_real() && weapon.get_uid() == uid) {
    if (weapon.get_item_class() == ITEM_CLASS_CLOTHING) {
      items_worn.push_back(weapon);
      weapon = Item();
    }
    return;
  }
  for (int i = 0; i < inventory.size(); i++) {
    if (inventory[i].get_uid() == uid) {
      if (inventory[i].get_item_class() == ITEM_CLASS_CLOTHING) {
        items_worn.push_back(inventory[i]);
        items_worn.back().count = 1;
        if (inventory[i].count <= 1) {
          inventory.erase( inventory.begin() + i );
        } else {
          inventory[i].count--;
        }
      }
      return;
    }
  }
}

void Entity::reload_prep(int uid)
{
  Item* it = ref_item_uid(uid);
  if (!it || !it->can_reload()) {
    return;
  }
  Item ammo = pick_ammo_for(it);
  if (ammo.is_real()) {
    set_activity(PLAYER_ACTIVITY_RELOAD, it->time_to_reload(),
                 it->get_uid(), ammo.get_uid());
  }
}

Item Entity::pick_ammo_for(Item *it)
{
  if (!it || !it->can_reload()) {
    return Item();
  } 
  if (it->charges == it->get_max_charges()) {
    return Item();
  }
  if (it->charges > 0 && it->ammo) {
    return get_item_of_type(it->ammo);
  }
// TODO: Automate ammo selection for NPCs
  return Item();
}

bool Entity::is_wielding_item_uid(int uid)
{
  return weapon.is_real() && weapon.get_uid() == uid;
}

bool Entity::is_wearing_item_uid(int uid)
{
  for (int i = 0; i < items_worn.size(); i++) {
    if (items_worn[i].is_real() && items_worn[i].get_uid() == uid) {
      return true;
    }
  }
  return false;
}

bool Entity::is_carrying_item_uid(int uid)
{
  for (int i = 0; i < inventory.size(); i++) {
    if (inventory[i].is_real() && inventory[i].get_uid() == uid) {
      return true;
    }
  }
  return false;
}

bool Entity::has_item_uid(int uid)
{
  return (is_wielding_item_uid(uid) || is_wearing_item_uid(uid) ||
          is_carrying_item_uid(uid));
}

std::string Entity::drop_item_message(Item &it)
{
  int uid = it.get_uid();
  if (!it.is_real() || !ref_item_uid(uid)) {
    return "You don't have that item.";
  }
  std::stringstream ret;
  ret << get_name_to_player() << " " << conjugate("drop") << " " <<
         get_possessive() << " " << it.get_name() << ".";
  return ret.str();
}

std::string Entity::wear_item_message(Item &it)
{
  int uid = it.get_uid();
  if (!it.is_real() || !ref_item_uid(uid)) {
    return "You don't have that item.";
  }
  if (it.get_item_class() != ITEM_CLASS_CLOTHING) {
    return it.get_name_indefinite() + " is not clothing!";
  }
  if (is_wearing_item_uid(uid)) { 
    return "You're already wearing that.";
  }
  std::stringstream ret;
  ret << get_name_to_player() << " " << conjugate("put") << " on " <<
         get_possessive() << " " << it.get_name() << ".";
  return ret.str();
}

std::string Entity::wield_item_message(Item &it)
{
  int uid = it.get_uid();
  std::stringstream ret;
  ret << get_name_to_player();
  if (!it.is_real() || !ref_item_uid(uid)) { // Should this indicate a bug?
    ret << " don't have that item.";
  } else if (is_wielding_item_uid(uid)) {
    ret << "'re already wielding that.";
  } else if (is_wearing_item_uid(uid)) {
    ret << "'re wearing that item - take it off first.";
  } else {
    ret << " " << conjugate("wield") << " " << it.get_name_definite() << ".";
  }
  return ret.str();
}

std::string Entity::sheath_weapon_message()
{
  if (!weapon.is_real()) {
    return "";
  }
  std::stringstream ret;
  ret << get_name_to_player() << " " << conjugate("put") << " away " <<
         get_possessive() << " " << weapon.get_name() << ".";
  return ret.str();
}

Attack Entity::base_attack()
{
  return Attack();
}

Attack Entity::std_attack()
{
  Attack att = base_attack();
  if (weapon.is_real()) {
    att.use_weapon(weapon, 0, 0); // TODO : Use stats here
  }
  return att;
}

  
void Entity::attack(Entity* target)
{
  if (!target) {
    debugmsg("'%s' attempted attack() on a null target.");
    return;
  }

  Attack att = std_attack();

  action_points -= att.speed;

  bool you_see = GAME.player->can_sense(GAME.map, posx, posy);
  bool attacker_is_you = is_you();

  std::string miss_verb = (attacker_is_you ? "miss" : "misses");

  if (hit_roll(att.to_hit) < target->dodge_roll()) {
    if (you_see) {
      std::stringstream msg;
      msg << get_name_to_player() << " " << miss_verb << " " <<
             target->get_name_to_player() << "!";
      GAME.add_msg( msg.str().c_str() );
    }
// TODO: action_point penalty for missing?
    return;
  }

  Body_part bp_hit = (target->is_player() ? random_body_part_to_hit() :
                                            BODYPART_NULL);

// TODO: Should total_damage be reduced by damage absorbed by armor?
  Damage_set damage = att.roll_damage();
  for (int i = 0; i < DAMAGE_MAX; i++) {
    int dam = damage.get_damage(i);
    target->take_damage(Damage_type(i), dam, get_name_to_player(), bp_hit);
  }

  if (you_see) {
    std::stringstream damage_ss;
    damage_ss << get_name_to_player() << " ";
    if (attacker_is_you) {
      damage_ss << att.verb_first;
    } else {
      damage_ss << att.verb_third;
    }
    damage_ss << " ";
    if (bp_hit == BODYPART_NULL) {
      damage_ss << target->get_name_to_player();
    } else {
      damage_ss << target->get_possessive() << " " << body_part_name(bp_hit);
    }
    if (target->is_you()) {
      damage_ss << " for " << damage.total_damage() << " damage";
    }
    damage_ss << "!";
    GAME.add_msg( damage_ss.str().c_str() );
  }
}
  
int Entity::hit_roll(int bonus)
{
  return rng(1, 10) + bonus;
}

int Entity::dodge_roll()
{
  return rng(1, 10);
}

// This one gets overloaded fully.
void Entity::take_damage(Damage_type type, int damage, std::string reason,
                         Body_part part)
{
}

Ranged_attack Entity::throw_item(Item it)
{
  Ranged_attack ret = it.get_thrown_attack();
// TODO: Base on skill.
  ret.variance.push_back(-10);
  return ret;
}

Ranged_attack Entity::fire_weapon()
{
// TODO: Base on skill
// TODO: Use >1 round if we've selected a burst/auto shot
  if (!weapon.is_real() || weapon.get_item_class() != ITEM_CLASS_LAUNCHER ||
      !weapon.ammo || weapon.charges == 0) {
    return Ranged_attack();
  }
  weapon.charges--;
  return weapon.get_fired_attack();
}

bool Entity::can_sense(Map* map, int x, int y)
{
// Default Entity function just uses sight
  return map->senses(posx, posy, x, y);
}

Entity_pool::Entity_pool()
{
  next_uid = 0;
}

Entity_pool::~Entity_pool()
{
  for (std::list<Entity*>::iterator it = instances.begin();
       it != instances.end();
       it++) {
    delete (*it);
  }
}

void Entity_pool::add_entity(Entity* ent)
{
  if (!ent) {
    debugmsg("Tried to push NULL to Entity_pool");
    return;
  }
  ent->uid = next_uid;
  next_uid++;
  instances.push_back(ent);
  uid_map[ent->uid] = ent;
}

Entity* Entity_pool::lookup_uid(int uid)
{
  if (uid_map.count(uid) == 0) {
    return NULL;
  }
  return uid_map[uid];
}

Entity* Entity_pool::entity_at(int posx, int posy)
{
  for (std::list<Entity*>::iterator it = instances.begin();
       it != instances.end();
       it++) {
    if ((*it)->posx == posx && (*it)->posy == posy) {
      return (*it);
    }
  }
  return NULL;
}
