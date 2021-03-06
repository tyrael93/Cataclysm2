#ifndef _ENTITY_H_
#define _ENTITY_H_

#include "glyph.h"
#include "map.h"
#include "item.h"
#include "player_activity.h"
#include "geometry.h"
#include "enum.h"
#include <string>

class Entity
{
public:
  Entity();
  virtual ~Entity();

  virtual std::string get_name();
  virtual std::string get_name_to_player();
  virtual std::string get_possessive();
  virtual std::string conjugate(const std::string &verb);
  virtual glyph get_glyph();
  virtual Point get_position();

  virtual bool is_player()  { return false; }
  virtual bool is_monster() { return false; }
  virtual bool is_you()     { return false; } // As in THE player

  virtual void die();
  virtual void gain_action_points();
  virtual int  get_speed();
  virtual void take_turn();

  virtual Intel_level get_intelligence();
  virtual bool has_sense(Sense_type sense);
  virtual bool can_see    (Map* map, int x, int y);
  virtual bool can_move_to(Map* map, int x, int y);
  virtual void move_to    (Map* map, int x, int y);
  virtual void pause();

// Misc action functions
  void set_activity(Player_activity_type type, int duration,
                    int primary_uid = -1, int secondary_uid = -1);
  void use_ap(int amount);
  void shift(int shiftx, int shifty); // For map shifting
  
// Inventory functions
  virtual bool add_item(Item item);
  Item  get_item_of_type(Item_type *type);
  Item* ref_item_of_type(Item_type *type);
  Item* ref_item_uid   (int uid);
  Item  remove_item_uid(int uid, int count = 0);
  void  wield_item_uid (int uid);
  void  sheath_weapon();
  void  wear_item_uid  (int uid);
  void  reload_prep    (int uid);
  virtual Item pick_ammo_for(Item *it);

  bool is_wielding_item_uid(int uid);
  bool is_wearing_item_uid(int uid);
  bool is_carrying_item_uid(int uid);
  bool has_item_uid(int uid); // wielding, wearing or carrying

// Message functions
  virtual std::string drop_item_message(Item &it);
  virtual std::string wear_item_message(Item &it);
  virtual std::string wield_item_message(Item &it);
  virtual std::string sheath_weapon_message();

// Combat functions
  virtual Attack base_attack();
  virtual Attack std_attack(); // With weapon if it exists
  virtual void attack(Entity* target);
  virtual int  hit_roll(int bonus);
  virtual int  dodge_roll();
  virtual void take_damage(Damage_type type, int damage, std::string reason,
                           Body_part part = BODYPART_NULL);
  virtual Ranged_attack throw_item(Item it);
  virtual Ranged_attack fire_weapon();

  virtual bool can_sense(Map* map, int x, int y);

  int uid;
  int posx, posy;
  int action_points;
  bool dead;
  bool killed_by_player;

  Player_activity activity;

  Item weapon;
  std::vector<Item> inventory;
  std::vector<Item> items_worn;
};

/* For now, Entity_pool does NOT include a map which uses location as a key.
 * In order for this map to be useful, we'd have to update it every turn, which
 * means it' probably be more trouble than it's worth, except when the map is
 * being called several times per turn.  We'd also have to update it after
 * every monster moves, which is a lot.
 * This means that monster_at() has to iterate over all monsters, which is
 * potentially slow, but what can you do.
 */

class Entity_pool
{
public:
  Entity_pool();
  ~Entity_pool();

  void add_entity(Entity* ent);

  Entity* lookup_uid(int uid);
  Entity* entity_at(int posx, int posy);

  std::list<Entity*> instances;
private:
  std::map<int,Entity*> uid_map;
  int next_uid;
};
  
#endif
