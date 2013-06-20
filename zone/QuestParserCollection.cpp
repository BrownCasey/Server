/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2006  EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "../common/debug.h"
#include "../common/MiscFunctions.h"
#include "../common/features.h"
#include "QuestParserCollection.h"
#include "QuestInterface.h"
#include "zone.h"
#include "questmgr.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

extern Zone* zone;

QuestParserCollection::QuestParserCollection() {
	_player_quest_status = QuestUnloaded;
	_global_player_quest_status = QuestUnloaded;
	_global_npc_quest_status = QuestUnloaded;
}

QuestParserCollection::~QuestParserCollection() {
}

void QuestParserCollection::RegisterQuestInterface(QuestInterface *qi, std::string ext) {
	_interfaces[qi->GetIdentifier()] = qi;
	_extensions[qi->GetIdentifier()] = ext;
	_load_precedence.push_back(qi);
}

void QuestParserCollection::AddVar(std::string name, std::string val) {
	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		(*iter)->AddVar(name, val);
		iter++;
	}
}

void QuestParserCollection::Init() {
	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		(*iter)->Init();
		iter++;
	}
}

void QuestParserCollection::ReloadQuests(bool reset_timers) {
	if(reset_timers) {
		quest_manager.ClearAllTimers();
	}

	_npc_quest_status.clear();
	_player_quest_status = QuestUnloaded;
	_global_player_quest_status = QuestUnloaded;
	_global_npc_quest_status = QuestUnloaded;
	_spell_quest_status.clear();
	_item_quest_status.clear();
	_encounter_quest_status.clear();
	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		(*iter)->ReloadQuests();
		iter++;
	}
}

bool QuestParserCollection::HasQuestSub(uint32 npcid, QuestEventID evt) {
	return HasQuestSubLocal(npcid, evt) || HasQuestSubGlobal(evt);
}

bool QuestParserCollection::HasQuestSubLocal(uint32 npcid, QuestEventID evt) {
	std::map<uint32, uint32>::iterator iter = _npc_quest_status.find(npcid);
	
	if(iter != _npc_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			if(qiter->second->HasQuestSub(npcid, evt)) {
				return true;
			}
		}
	} else {
		std::string filename;
		QuestInterface *qi = GetQIByNPCQuest(npcid, filename);
		if(qi) {
			_npc_quest_status[npcid] = qi->GetIdentifier();

			qi->LoadNPCScript(filename, npcid);
			if(qi->HasQuestSub(npcid, evt)) {
				return true;
			}
		} else {
			_npc_quest_status[npcid] = QuestFailedToLoad;
		}
	}
	return false;
}

bool QuestParserCollection::HasQuestSubGlobal(QuestEventID evt) {
	if(_global_npc_quest_status == QuestUnloaded) {
		std::string filename;
		QuestInterface *qi = GetQIByGlobalNPCQuest(filename);
		if(qi) {
			qi->LoadGlobalNPCScript(filename);
			_global_npc_quest_status = qi->GetIdentifier();
			if(qi->HasGlobalQuestSub(evt)) {
				return true;
			}
		}
	} else {
		if(_global_npc_quest_status != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(_global_npc_quest_status);
			if(qiter->second->HasGlobalQuestSub(evt)) {
				return true;
			}
		}
	}
	return false;
}

bool QuestParserCollection::PlayerHasQuestSub(QuestEventID evt) {
	return PlayerHasQuestSubLocal(evt) || PlayerHasQuestSubGlobal(evt);
}

bool QuestParserCollection::PlayerHasQuestSubLocal(QuestEventID evt) {
	if(_player_quest_status == QuestUnloaded) {
		std::string filename;	
		QuestInterface *qi = GetQIByPlayerQuest(filename);
		if(qi) {
			_player_quest_status = qi->GetIdentifier();
			qi->LoadPlayerScript(filename);
			return qi->PlayerHasQuestSub(evt);
		}
	} else if(_player_quest_status != QuestFailedToLoad) {
		std::map<uint32, QuestInterface*>::iterator iter = _interfaces.find(_player_quest_status);
		return iter->second->PlayerHasQuestSub(evt);
	}
	return false;
}

bool QuestParserCollection::PlayerHasQuestSubGlobal(QuestEventID evt) {
	if(_global_player_quest_status == QuestUnloaded) {
		std::string filename;	
		QuestInterface *qi = GetQIByPlayerQuest(filename);
		if(qi) {
			_global_player_quest_status = qi->GetIdentifier();
			qi->LoadPlayerScript(filename);
			return qi->GlobalPlayerHasQuestSub(evt);
		}
	} else if(_global_player_quest_status != QuestFailedToLoad) {
		std::map<uint32, QuestInterface*>::iterator iter = _interfaces.find(_global_player_quest_status);
		return iter->second->GlobalPlayerHasQuestSub(evt);
	}
	return false;
}

bool QuestParserCollection::SpellHasQuestSub(uint32 spell_id, QuestEventID evt) {
	std::map<uint32, uint32>::iterator iter = _spell_quest_status.find(spell_id);
	if(iter != _spell_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			return qiter->second->SpellHasQuestSub(spell_id, evt);
		}
	} else {
		std::string filename;
		QuestInterface *qi = GetQIBySpellQuest(spell_id, filename);
		if(qi) {
			_spell_quest_status[spell_id] = qi->GetIdentifier();
			qi->LoadSpellScript(filename, spell_id);
			return qi->SpellHasQuestSub(spell_id, evt);
		} else {
			_spell_quest_status[spell_id] = QuestFailedToLoad;
		}
	}
	return false;
}

bool QuestParserCollection::ItemHasQuestSub(ItemInst *itm, QuestEventID evt) {
	std::string item_script;
	if(itm->GetItem()->ScriptFileID != 0) {
		item_script = "script_";
		item_script += std::to_string(itm->GetItem()->ScriptFileID);
	} else if(strlen(itm->GetItem()->CharmFile) > 0) {
		item_script = itm->GetItem()->CharmFile;
	} else {
		item_script = std::to_string(itm->GetID());
	}

	uint32 item_id = itm->GetID();
	std::map<uint32, uint32>::iterator iter = _item_quest_status.find(item_id);
	if(iter != _item_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			return qiter->second->ItemHasQuestSub(itm, evt);
		}
	} else {
		std::string filename;
		QuestInterface *qi = GetQIByItemQuest(item_script, filename);
		if(qi) {
			_item_quest_status[item_id] = qi->GetIdentifier();
			qi->LoadItemScript(filename, itm);
			return qi->ItemHasQuestSub(itm, evt);
		} else {
			_item_quest_status[item_id] = QuestFailedToLoad;
		}
	}
	return false;
}

int QuestParserCollection::EventNPC(QuestEventID evt, NPC *npc, Mob *init, std::string data, uint32 extra_data,
									std::vector<void*> *extra_pointers) {
	int rl = EventNPCLocal(evt, npc, init, data, extra_data, extra_pointers);
	int rg = EventNPCGlobal(evt, npc, init, data, extra_data, extra_pointers);
	DispatchEventNPC(evt, npc, init, data, extra_data, extra_pointers);
	
	//Local quests returning non-default values have priority over global quests
	if(rl != 0) {
		return rl;
	} else if(rg != 0) {
		return rg;
	}
	
	return 0;
}

int QuestParserCollection::EventNPCLocal(QuestEventID evt, NPC* npc, Mob *init, std::string data, uint32 extra_data,
										 std::vector<void*> *extra_pointers) {
	std::map<uint32, uint32>::iterator iter = _npc_quest_status.find(npc->GetNPCTypeID());
	if(iter != _npc_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			return qiter->second->EventNPC(evt, npc, init, data, extra_data, extra_pointers);
		}
	} else {
		std::string filename;
		QuestInterface *qi = GetQIByNPCQuest(npc->GetNPCTypeID(), filename);
		if(qi) {
			_npc_quest_status[npc->GetNPCTypeID()] = qi->GetIdentifier();
			qi->LoadNPCScript(filename, npc->GetNPCTypeID());
			return qi->EventNPC(evt, npc, init, data, extra_data, extra_pointers);
		} else {
			_npc_quest_status[npc->GetNPCTypeID()] = QuestFailedToLoad;
		}
	}
	return 0;
}

int QuestParserCollection::EventNPCGlobal(QuestEventID evt, NPC* npc, Mob *init, std::string data, uint32 extra_data,
										  std::vector<void*> *extra_pointers) {
	if(_global_npc_quest_status != QuestUnloaded && _global_npc_quest_status != QuestFailedToLoad) {
		std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(_global_npc_quest_status);
		return qiter->second->EventGlobalNPC(evt, npc, init, data, extra_data, extra_pointers);
	} else {
		std::string filename;
		QuestInterface *qi = GetQIByGlobalNPCQuest(filename);
		if(qi) {
			_global_npc_quest_status = qi->GetIdentifier();
			qi->LoadGlobalNPCScript(filename);
			return qi->EventGlobalNPC(evt, npc, init, data, extra_data, extra_pointers);
		} else {
			_global_npc_quest_status = QuestFailedToLoad;
		}
	}
	return 0;
}

int QuestParserCollection::EventPlayer(QuestEventID evt, Client *client, std::string data, uint32 extra_data,
									   std::vector<void*> *extra_pointers) {
	int rl = EventPlayerLocal(evt, client, data, extra_data, extra_pointers);
	int rg = EventPlayerGlobal(evt, client, data, extra_data, extra_pointers);
	DispatchEventPlayer(evt, client, data, extra_data, extra_pointers);
	
	//Local quests returning non-default values have priority over global quests
	if(rl != 0) {
		return rl;
	} else if(rg != 0) {
		return rg;
	}
	
	return 0;
}

int QuestParserCollection::EventPlayerLocal(QuestEventID evt, Client *client, std::string data, uint32 extra_data,
											std::vector<void*> *extra_pointers) {
	if(_player_quest_status == QuestUnloaded) {
		std::string filename;
		QuestInterface *qi = GetQIByPlayerQuest(filename);
		if(qi) {
			_player_quest_status = qi->GetIdentifier();
			qi->LoadPlayerScript(filename);
			return qi->EventPlayer(evt, client, data, extra_data, extra_pointers);
		}
	} else { 
		if(_player_quest_status != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator iter = _interfaces.find(_player_quest_status);
			return iter->second->EventPlayer(evt, client, data, extra_data, extra_pointers);
		}
	}
	return 0;
}

int QuestParserCollection::EventPlayerGlobal(QuestEventID evt, Client *client, std::string data, uint32 extra_data,
											 std::vector<void*> *extra_pointers) {
	if(_global_player_quest_status == QuestUnloaded) {
		std::string filename;
		QuestInterface *qi = GetQIByGlobalPlayerQuest(filename);
		if(qi) {
			_global_player_quest_status = qi->GetIdentifier();
			qi->LoadGlobalPlayerScript(filename);
			return qi->EventGlobalPlayer(evt, client, data, extra_data, extra_pointers);
		}
	} else { 
		if(_global_player_quest_status != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator iter = _interfaces.find(_global_player_quest_status);
			return iter->second->EventGlobalPlayer(evt, client, data, extra_data, extra_pointers);
		}
	}
	return 0;
}

int QuestParserCollection::EventItem(QuestEventID evt, Client *client, ItemInst *item, Mob *mob, std::string data, uint32 extra_data,
									 std::vector<void*> *extra_pointers) {
	std::string item_script;
	if(item->GetItem()->ScriptFileID != 0) {
		item_script = "script_";
		item_script += std::to_string(item->GetItem()->ScriptFileID);
	} else if(strlen(item->GetItem()->CharmFile) > 0) {
		item_script = item->GetItem()->CharmFile;
	} else {
		item_script = std::to_string(item->GetID());
	}

	uint32 item_id = item->GetID();
	std::map<uint32, uint32>::iterator iter = _item_quest_status.find(item_id);
	if(iter != _item_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			auto ret = qiter->second->EventItem(evt, client, item, mob, data, extra_data, extra_pointers);
			DispatchEventItem(evt, client, item, mob, data, extra_data, extra_pointers);
			return ret;
		}
		DispatchEventItem(evt, client, item, mob, data, extra_data, extra_pointers);
	} else {
		std::string filename;
		QuestInterface *qi = GetQIByItemQuest(item_script, filename);
		if(qi) {
			_item_quest_status[item_id] = qi->GetIdentifier();
			qi->LoadItemScript(filename, item);
			auto ret = qi->EventItem(evt, client, item, mob, data, extra_data, extra_pointers);
			DispatchEventItem(evt, client, item, mob, data, extra_data, extra_pointers);
			return ret;
		} else {
			_item_quest_status[item_id] = QuestFailedToLoad;
			DispatchEventItem(evt, client, item, mob, data, extra_data, extra_pointers);
		}
	}
	return 0;
}

int QuestParserCollection::EventSpell(QuestEventID evt, NPC* npc, Client *client, uint32 spell_id, uint32 extra_data,
									  std::vector<void*> *extra_pointers) {
	std::map<uint32, uint32>::iterator iter = _spell_quest_status.find(spell_id);
	if(iter != _spell_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			auto ret = qiter->second->EventSpell(evt, npc, client, spell_id, extra_data, extra_pointers);
			DispatchEventSpell(evt, npc, client, spell_id, extra_data, extra_pointers);
			return ret;
		}
		DispatchEventSpell(evt, npc, client, spell_id, extra_data, extra_pointers);
	} else {
		std::string filename;
		QuestInterface *qi = GetQIBySpellQuest(spell_id, filename);
		if(qi) {
			_spell_quest_status[spell_id] = qi->GetIdentifier();
			qi->LoadSpellScript(filename, spell_id);
			auto ret = qi->EventSpell(evt, npc, client, spell_id, extra_data, extra_pointers);
			DispatchEventSpell(evt, npc, client, spell_id, extra_data, extra_pointers);
			return ret;
		} else {
			_spell_quest_status[spell_id] = QuestFailedToLoad;
			DispatchEventSpell(evt, npc, client, spell_id, extra_data, extra_pointers);
		}
	}
	return 0;
}

int QuestParserCollection::EventEncounter(QuestEventID evt, std::string encounter_name, uint32 extra_data,
										  std::vector<void*> *extra_pointers) {
	auto iter = _encounter_quest_status.find(encounter_name);
	if(iter != _encounter_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			return qiter->second->EventEncounter(evt, encounter_name, extra_data, extra_pointers);
		}
	} else {
		std::string filename;
		QuestInterface *qi = GetQIByEncounterQuest(encounter_name, filename);
		if(qi) {
			_encounter_quest_status[encounter_name] = qi->GetIdentifier();
			qi->LoadEncounterScript(filename, encounter_name);
			return qi->EventEncounter(evt, encounter_name, extra_data, extra_pointers);
		} else {
			_encounter_quest_status[encounter_name] = QuestFailedToLoad;
		}
	}
	return 0;
}

QuestInterface *QuestParserCollection::GetQIByNPCQuest(uint32 npcid, std::string &filename) {
	//first look for /quests/zone/npcid.ext (precedence)
	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/";
	filename += itoa(npcid);
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//second look for /quests/zone/npcname.ext (precedence)
	const NPCType *npc_type = database.GetNPCType(npcid);
	if(!npc_type) {
		return nullptr;
	}
	std::string npc_name = npc_type->name;
	int sz = static_cast<int>(npc_name.length());
	for(int i = 0; i < sz; ++i) {
		if(npc_name[i] == '`') {
			npc_name[i] = '-';
		}
	}

	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/";
	filename += npc_name;

	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//third look for /quests/global/npcid.ext (precedence)
	filename = "quests/";
	filename += QUEST_GLOBAL_DIRECTORY;
	filename += "/";
	filename += itoa(npcid);
	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//fourth look for /quests/global/npcname.ext (precedence)
	filename = "quests/";
	filename += QUEST_GLOBAL_DIRECTORY;
	filename += "/";
	filename += npc_name;
	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//fifth look for /quests/zone/default.ext (precedence)
	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/";
	filename += "default";
	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//last look for /quests/global/default.ext (precedence)
	filename = "quests/";
	filename += QUEST_GLOBAL_DIRECTORY;
	filename += "/";
	filename += "default";
	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	return nullptr;
}

QuestInterface *QuestParserCollection::GetQIByPlayerQuest(std::string &filename) {

	if(!zone)
	return nullptr;

	//first look for /quests/zone/player_v[instance_version].ext (precedence)
	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/";
	filename += "player_v";
	filename += itoa(zone->GetInstanceVersion());
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}	

	//second look for /quests/zone/player.ext (precedence)
	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/";
	filename += "player";

	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//third look for /quests/global/player.ext (precedence)
	filename = "quests/";
	filename += QUEST_GLOBAL_DIRECTORY;
	filename += "/";
	filename += "player";
	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	return nullptr;
}

QuestInterface *QuestParserCollection::GetQIByGlobalNPCQuest(std::string &filename) {
	// simply look for /quests/global/global_npc.ext
	filename = "quests/";
	filename += QUEST_GLOBAL_DIRECTORY;
	filename += "/";
	filename += "global_npc";
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	return nullptr;
}

QuestInterface *QuestParserCollection::GetQIByGlobalPlayerQuest(std::string &filename) {
	//first look for /quests/global/player.ext (precedence)
	filename = "quests/";
	filename += QUEST_GLOBAL_DIRECTORY;
	filename += "/";
	filename += "global_player";
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	return nullptr;
}

QuestInterface *QuestParserCollection::GetQIBySpellQuest(uint32 spell_id, std::string &filename) {
	//first look for /quests/zone/spells/spell_id.ext (precedence)
	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/spells/";
	filename += itoa(spell_id);
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//second look for /quests/spells/spell_id.ext (precedence)
	filename = "quests/spells/";
	filename += itoa(spell_id);

	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}	

	return nullptr;
}

QuestInterface *QuestParserCollection::GetQIByItemQuest(std::string item_script, std::string &filename) {
	//first look for /quests/zone/items/item_script.ext (precedence)
	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/items/";
	filename += item_script;
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}
	
	//second look for /quests/items/item_script.ext (precedence)
	filename = "quests/items/";
	filename += item_script;

	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	return nullptr;
}

QuestInterface *QuestParserCollection::GetQIByEncounterQuest(std::string encounter_name, std::string &filename) {
	//first look for /quests/zone/encounters/encounter_name.ext (precedence)
	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/encounters/";
	filename += encounter_name;
	std::string tmp;
	FILE *f = nullptr;

	auto iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		auto ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		++iter;
	}
	
	//second look for /quests/encounters/encounter_name.ext (precedence)
	filename = "quests/encounters/";
	filename += encounter_name;

	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		auto ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		++iter;
	}

	return nullptr;
}

void QuestParserCollection::GetErrors(std::list<std::string> &err) {
	err.clear();
	auto iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		(*iter)->GetErrors(err);
		++iter;
	}
}

void QuestParserCollection::DispatchEventNPC(QuestEventID evt, NPC* npc, Mob *init, std::string data, uint32 extra_data,
											 std::vector<void*> *extra_pointers) {
	auto iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		(*iter)->DispatchEventNPC(evt, npc, init, data, extra_data, extra_pointers);
		++iter;
	}
}

void QuestParserCollection::DispatchEventPlayer(QuestEventID evt, Client *client, std::string data, uint32 extra_data,
												std::vector<void*> *extra_pointers) {
	auto iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		(*iter)->DispatchEventPlayer(evt, client, data, extra_data, extra_pointers);
		++iter;
	}
}

void QuestParserCollection::DispatchEventItem(QuestEventID evt, Client *client, ItemInst *item, Mob *mob, std::string data,
											  uint32 extra_data, std::vector<void*> *extra_pointers) {
	auto iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		(*iter)->DispatchEventItem(evt, client, item, mob, data, extra_data, extra_pointers);
		++iter;
	}
}

void QuestParserCollection::DispatchEventSpell(QuestEventID evt, NPC* npc, Client *client, uint32 spell_id, uint32 extra_data,
											   std::vector<void*> *extra_pointers) {
	auto iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		(*iter)->DispatchEventSpell(evt, npc, client, spell_id, extra_data, extra_pointers);
		++iter;
	}
}
