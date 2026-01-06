/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/
////////////////////////////////////////////////////////////////////////////////
//
//  (c) 2001-2003 Electronic Arts Inc.
//
////////////////////////////////////////////////////////////////////////////////

// AICoopPlayer.cpp
// Computerized opponent that cooperates with a human player.

#include "GameLogic/AICoopPlayer.h"
#include "Common/NameKeyGenerator.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/Team.h"
#include "Common/Thing.h"
#include "Common/ThingTemplate.h"
#include "Common/WellKnownKeys.h"
#include "GameLogic/AIPlayer.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Object.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/SidesList.h"
#include "PreRTS.h"

#include "Common/DjDebug.h"

AICoopPlayer::AICoopPlayer(Player *p) : AISkirmishPlayer(p) {
  DjLog("AICoopPlayer created for player %d (%ls) Side: %s",
        p->getPlayerIndex(), p->getPlayerDisplayName().str(),
        p->getSide().str());
  m_skirmishScriptsLoaded = false;
}

AICoopPlayer::~AICoopPlayer() {}

void AICoopPlayer::update() {
  // Check if this is assist mode for a human player
  if (m_player->getPlayerType() == PLAYER_HUMAN) {
    // ASSIST MODE: Help human player
    assistHumanPlayer();
    return;
  }

  // Standard AI mode for computer players
  if (TheGameLogic->getFrame() % 30 == 0) { // Log once per second
    bool hasBuildList = (m_player->getBuildList() != NULL);
    bool canBuildBase = m_player->getCanBuildBase();
    DjLog("AICoopPlayer::update - Alive. Frame: %d. HasBuildList: %d. "
          "CanBuildBase: %d",
          TheGameLogic->getFrame(), hasBuildList, canBuildBase);
  }

  // Run standard Skirmish AI logic
  AISkirmishPlayer::update();

  // Add coop-specific behavior
  attemptCoopBehavior();
}

Player *AICoopPlayer::findHumanAlly() {
  for (Int i = 0; i < ThePlayerList->getPlayerCount(); i++) {
    Player *other = ThePlayerList->getNthPlayer(i);
    if (other != m_player && other->getPlayerType() == PLAYER_HUMAN) {
      if (m_player->getRelationship(other->getDefaultTeam()) == ALLIES) {
        return other;
      }
    }
  }
  return NULL;
}

void AICoopPlayer::attemptCoopBehavior() {
  Player *humanAlly = findHumanAlly();
  if (!humanAlly) {
    return;
  }

  UnsignedInt currentFrame = TheGameLogic->getFrame();
  UnsignedInt lastAttacked = humanAlly->getAttackedFrame();

  if (currentFrame > lastAttacked && (currentFrame - lastAttacked) < 150) {
    DjLog("AICoopPlayer::attemptCoopBehavior - Ally %ls is under attack!",
          humanAlly->getPlayerDisplayName().str());
    acquireEnemy();
  }
}

void AICoopPlayer::acquireEnemy() {
  Player *humanAlly = findHumanAlly();

  if (humanAlly) {
    UnsignedInt currentFrame = TheGameLogic->getFrame();
    UnsignedInt lastAttacked = humanAlly->getAttackedFrame();

    if (currentFrame > lastAttacked && (currentFrame - lastAttacked) < 300) {
      for (Int i = 0; i < ThePlayerList->getPlayerCount(); i++) {
        Player *potentialEnemy = ThePlayerList->getNthPlayer(i);

        if (potentialEnemy == m_player || potentialEnemy == humanAlly)
          continue;
        if (m_player->getRelationship(potentialEnemy->getDefaultTeam()) !=
            ENEMIES)
          continue;

        if (humanAlly->getAttackedBy(potentialEnemy->getPlayerIndex())) {
          DjLog("AICoopPlayer::acquireEnemy - Switching target to defend ally "
                "from %ls",
                potentialEnemy->getPlayerDisplayName().str());

          m_currentEnemy = potentialEnemy;
          m_frameToCheckEnemy = currentFrame + 90;
          return;
        }
      }
    }
  }

  AISkirmishPlayer::acquireEnemy();
}

//=============================================================================
// AI Assist Mode for Human Players
//=============================================================================

void AICoopPlayer::assistHumanPlayer() {
  // Log assist mode activity periodically
  if (TheGameLogic->getFrame() % 300 == 0) {
    DjLog("AICoopPlayer::assistHumanPlayer - Assisting player %d",
          m_player->getPlayerIndex());
  }

  // Ensure BuildList is loaded for human player
  // Ensure BuildList is loaded for human player
  if (m_player->getBuildList() == NULL) {
    DjLog("AICoopPlayer: Initializing build list for Human Assist...");
    newMap(); // Load the build list from the map/side definition

    // Attempt to load Skirmish Scripts from matching AI side
    // This enables the "Priority Build" logic used by bots
    // Attempt to load Skirmish Scripts from matching AI side
    // This enables the "Priority Build" logic used by bots
    SidesInfo *mySideInfo =
        TheSidesList->getSideInfo(m_player->getPlayerIndex());

    // Only load if we haven't already. This bypasses the issue where
    // getScriptList() is non-NULL but useless.
    if (!m_skirmishScriptsLoaded) {
      m_skirmishScriptsLoaded =
          true; // Set flag immediately so we don't retry locally

      AsciiString sideName = m_player->getSide();
      AsciiString fullAiName = "Skirmish";
      fullAiName.concat(sideName);

      DjLog("AICoopPlayer: Searching for AI scripts for %s...",
            fullAiName.str());
      SidesInfo *aiSideInfo = TheSidesList->findSkirmishSideInfo(fullAiName);
      if (aiSideInfo && aiSideInfo->getScriptList()) {
        DjLog("AICoopPlayer: FOUND AI scripts. Attaching to Human Player.");

        // Force replace script list
        mySideInfo->setScriptList(aiSideInfo->getScriptList());

        // Now load the TEAMS associated with this AI side
        // Scripts reference teams by name (e.g. "ChinaBarracks"), so we must
        DjLog("AICoopPlayer: Attempting to load teams for AI Name: '%s'",
              fullAiName.str());

        Int numTeams = TheSidesList->getNumSkirmishTeams();
        DjLog("AICoopPlayer: Total Skirmish Teams in SidesList: %d", numTeams);

        Int loadedTeams = 0;
        for (Int i = 0; i < numTeams; ++i) {
          TeamsInfo *info = TheSidesList->getSkirmishTeamInfo(i);
          if (!info)
            continue;
          const Dict *dict = info->getDict();
          if (!dict)
            continue;

          // Teams store their owner in "TeamOwner", not "PlayerName"
          AsciiString teamOwnerName = dict->getAsciiString(TheKey_teamOwner);

          // Log the first few to check format
          if (i < 5) {
            DjLog("AICoopPlayer: Team #%d TeamOwner: '%s'", i,
                  teamOwnerName.str());
          }

          if (teamOwnerName.compareNoCase(fullAiName) == 0) {
            AsciiString teamName = dict->getAsciiString(TheKey_teamName);
            DjLog("AICoopPlayer: MATCH FOUND! Loading team: %s",
                  teamName.str());

            TheTeamFactory->initTeam(teamName, m_player->getName(), false,
                                     (Dict *)dict);
            loadedTeams++;
          }
        }
        DjLog("AICoopPlayer: Loaded %d Skirmish Teams for Human Player.",
              loadedTeams);
      } else {
        DjLog("AICoopPlayer: FAIL - Could not find AI scripts for %s",
              fullAiName.str());
        // Fallback: Enable AutomaticBuild for everything so it works anyway
        for (BuildListInfo *info = m_player->getBuildList(); info;
             info = info->getNext()) {
          info->setAutomaticBuild(true);
        }
        DjLog("AICoopPlayer: Fallback - Enabled AutomaticBuild for all "
              "structures.");
      }
    }
  }

  // Update build priorities dynamically every frame
  updateBuildPriorities(); // Using Skirmish Scripts or Default

  // 1. Auto-build structures (using base AI logic with our priority system)
  // Force the build flag because we skip AIPlayer::update which normally sets
  // it
  m_readyToBuildStructure = true;
  processBaseBuilding();

  // 2. Manage idle dozers
  autoManageIdleDozers();

  // 3. Manage idle units
  autoManageIdleUnits();

  // 4. Auto-defend base (disabled - needs fix for NULL pointer)
  // autoDefendBase();
}

void AICoopPlayer::autoManageIdleDozers() {
  // Only check every 10 seconds to avoid spam
  if (TheGameLogic->getFrame() % 300 != 0) {
    return;
  }

  int idleDozerCount = 0;

  // Iterate through player's teams to find dozers
  Player::PlayerTeamList::const_iterator it;
  for (it = m_player->getPlayerTeams()->begin();
       it != m_player->getPlayerTeams()->end(); ++it) {
    for (DLINK_ITERATOR<Team> iter = (*it)->iterate_TeamInstanceList();
         !iter.done(); iter.advance()) {
      Team *team = iter.cur();
      if (!team)
        continue;

      for (DLINK_ITERATOR<Object> objIter = team->iterate_TeamMemberList();
           !objIter.done(); objIter.advance()) {
        Object *obj = objIter.cur();
        if (!obj)
          continue;

        // Check if dozer
        if (!obj->isKindOf(KINDOF_DOZER))
          continue;

        // Check if idle
        AIUpdateInterface *ai = obj->getAIUpdateInterface();
        if (!ai || !ai->isIdle())
          continue;

        idleDozerCount++;
      }
    }
  }

  if (idleDozerCount > 0) {
    DjLog("AICoopPlayer: Found %d idle dozers for player %d", idleDozerCount,
          m_player->getPlayerIndex());
  }
}

void AICoopPlayer::autoManageIdleUnits() {
  // Only check every 10 seconds
  if (TheGameLogic->getFrame() % 300 != 0) {
    return;
  }

  Coord3D baseCenter;
  if (!getBaseCenter(&baseCenter)) {
    return;
  }

  int idleUnitCount = 0;

  // Iterate through player's teams
  Player::PlayerTeamList::const_iterator it;
  for (it = m_player->getPlayerTeams()->begin();
       it != m_player->getPlayerTeams()->end(); ++it) {
    for (DLINK_ITERATOR<Team> iter = (*it)->iterate_TeamInstanceList();
         !iter.done(); iter.advance()) {
      Team *team = iter.cur();
      if (!team)
        continue;

      for (DLINK_ITERATOR<Object> objIter = team->iterate_TeamMemberList();
           !objIter.done(); objIter.advance()) {
        Object *obj = objIter.cur();
        if (!obj)
          continue;

        // Check if combat unit (not structure, not dozer)
        if (!obj->isKindOf(KINDOF_CAN_ATTACK))
          continue;
        if (obj->isKindOf(KINDOF_DOZER))
          continue;
        if (obj->isKindOf(KINDOF_STRUCTURE))
          continue;

        // Check if idle
        AIUpdateInterface *ai = obj->getAIUpdateInterface();
        if (!ai || !ai->isIdle())
          continue;

        idleUnitCount++;

        // Send unit to guard base position
        ai->aiGuardPosition(&baseCenter, GUARDMODE_NORMAL, CMD_FROM_AI);
      }
    }
  }

  if (idleUnitCount > 0) {
    DjLog("AICoopPlayer: Sent %d idle units to guard base", idleUnitCount);
  }
}

void AICoopPlayer::autoDefendBase() {
  // Check every 5 seconds
  if (TheGameLogic->getFrame() % 150 != 0) {
    return;
  }

  Coord3D baseCenter;
  if (!getBaseCenter(&baseCenter)) {
    return;
  }

  // Find nearest enemy threat using TheAI->findClosestEnemy
  Object *threat = TheAI->findClosestEnemy(NULL, 300.0f, AI::CAN_ATTACK, NULL);

  if (!threat) {
    return;
  }

  // Check if threat is near our base
  Real dx = threat->getPosition()->x - baseCenter.x;
  Real dy = threat->getPosition()->y - baseCenter.y;
  Real distSq = dx * dx + dy * dy;

  if (distSq > 300.0f * 300.0f) {
    return; // Too far, not a threat
  }

  int defenderCount = 0;

  // Send idle units to attack threat
  Player::PlayerTeamList::const_iterator it;
  for (it = m_player->getPlayerTeams()->begin();
       it != m_player->getPlayerTeams()->end(); ++it) {
    for (DLINK_ITERATOR<Team> iter = (*it)->iterate_TeamInstanceList();
         !iter.done(); iter.advance()) {
      Team *team = iter.cur();
      if (!team)
        continue;

      for (DLINK_ITERATOR<Object> objIter = team->iterate_TeamMemberList();
           !objIter.done(); objIter.advance()) {
        Object *obj = objIter.cur();
        if (!obj)
          continue;

        if (!obj->isKindOf(KINDOF_CAN_ATTACK))
          continue;
        if (obj->isKindOf(KINDOF_STRUCTURE))
          continue;

        AIUpdateInterface *ai = obj->getAIUpdateInterface();
        if (!ai || !ai->isIdle())
          continue;

        // Attack the threat
        ai->aiAttackObject(threat, 0, CMD_FROM_AI);
        defenderCount++;

        if (defenderCount >= 5) {
          DjLog("AICoopPlayer: Sent %d defenders to attack threat %d",
                defenderCount, threat->getID());
          return;
        }
      }
    }
  }

  if (defenderCount > 0) {
    DjLog("AICoopPlayer: Sent %d defenders to attack threat %d", defenderCount,
          threat->getID());
  }
}

//=============================================================================
// Helper Functions for Build Management
//=============================================================================

Object *AICoopPlayer::findStructure(const char *namePattern) {
  if (!namePattern)
    return NULL;

  Player::PlayerTeamList::const_iterator it;
  for (it = m_player->getPlayerTeams()->begin();
       it != m_player->getPlayerTeams()->end(); ++it) {
    for (DLINK_ITERATOR<Team> iter = (*it)->iterate_TeamInstanceList();
         !iter.done(); iter.advance()) {
      Team *team = iter.cur();
      if (!team)
        continue;

      for (DLINK_ITERATOR<Object> objIter = team->iterate_TeamMemberList();
           !objIter.done(); objIter.advance()) {
        Object *obj = objIter.cur();
        if (!obj)
          continue;

        if (!obj->isKindOf(KINDOF_STRUCTURE))
          continue;

        const ThingTemplate *tmpl = obj->getTemplate();
        if (tmpl && strstr(tmpl->getName().str(), namePattern)) {
          return obj;
        }
      }
    }
  }
  return NULL;
}

int AICoopPlayer::countStructures(const char *namePattern) {
  if (!namePattern)
    return 0;

  int count = 0;
  Player::PlayerTeamList::const_iterator it;
  for (it = m_player->getPlayerTeams()->begin();
       it != m_player->getPlayerTeams()->end(); ++it) {
    for (DLINK_ITERATOR<Team> iter = (*it)->iterate_TeamInstanceList();
         !iter.done(); iter.advance()) {
      Team *team = iter.cur();
      if (!team)
        continue;

      for (DLINK_ITERATOR<Object> objIter = team->iterate_TeamMemberList();
           !objIter.done(); objIter.advance()) {
        Object *obj = objIter.cur();
        if (!obj)
          continue;

        if (!obj->isKindOf(KINDOF_STRUCTURE))
          continue;

        const ThingTemplate *tmpl = obj->getTemplate();
        if (tmpl && strstr(tmpl->getName().str(), namePattern)) {
          count++;
        }
      }
    }
  }
  return count;
}

int AICoopPlayer::countStructuresUnderConstruction(const char *namePattern) {
  if (!namePattern)
    return 0;

  int count = 0;
  Player::PlayerTeamList::const_iterator it;
  for (it = m_player->getPlayerTeams()->begin();
       it != m_player->getPlayerTeams()->end(); ++it) {
    for (DLINK_ITERATOR<Team> iter = (*it)->iterate_TeamInstanceList();
         !iter.done(); iter.advance()) {
      Team *team = iter.cur();
      if (!team)
        continue;

      for (DLINK_ITERATOR<Object> objIter = team->iterate_TeamMemberList();
           !objIter.done(); objIter.advance()) {
        Object *obj = objIter.cur();
        if (!obj)
          continue;

        if (!obj->isKindOf(KINDOF_STRUCTURE))
          continue;
        if (!obj->getStatusBits().test(OBJECT_STATUS_UNDER_CONSTRUCTION))
          continue;

        const ThingTemplate *tmpl = obj->getTemplate();
        if (tmpl && strstr(tmpl->getName().str(), namePattern)) {
          count++;
        }
      }
    }
  }
  return count;
}

//=============================================================================
// Build Priority Management
//=============================================================================

void AICoopPlayer::initializeBuildPriorities() {
  // Called once when BuildList is first loaded
  // Set initial AutomaticBuild flags based on structure type

  for (BuildListInfo *info = m_player->getBuildList(); info;
       info = info->getNext()) {
    AsciiString name = info->getTemplateName();

    // Power Plant and Supply Center: Always start enabled
    if (strstr(name.str(), "PowerPlant") ||
        strstr(name.str(), "SupplyCenter") ||
        strstr(name.str(), "SupplyDropZone")) {
      info->setAutomaticBuild(true);
    }
    // Everything else: Start disabled, will be enabled progressively
    else {
      info->setAutomaticBuild(false);
    }
  }

  DjLog("AICoopPlayer: Build priorities initialized (progressive system)");
}

void AICoopPlayer::updateBuildPriorities() {
  // Called every frame to dynamically update AutomaticBuild flags
  // Hybrid Logic:
  // 1. Power Plants: Smart management to fix 0/0 deadlock AND prevent spam (6
  // PPs).
  // 2. Others: Enable All to let Engine/AI prerequisites handle the flow.

  Bool hasSufficientPower = m_player->getEnergy()->hasSufficientPower();
  int powerPlantCount = countStructures("PowerPlant");
  int powerPlantBuilding = countStructuresUnderConstruction("PowerPlant");

  for (BuildListInfo *info = m_player->getBuildList(); info;
       info = info->getNext()) {
    AsciiString name = info->getTemplateName();

    if (strstr(name.str(), "PowerPlant")) {
      // Fix 1: Deadlock (0/0 power is "sufficient") -> Force build if count
      // < 1. Fix 2: Spam (6 PPs) -> Only build if power is actually needed.
      // Logic: Build if (No PPs exist) OR (Power is Low).
      Bool shouldBuild =
          (powerPlantCount + powerPlantBuilding) < 1 || !hasSufficientPower;

      info->setAutomaticBuild(shouldBuild);
    } else {
      // For everything else (Barracks, Supply, Factories), just enable them.
      // The Engine's "isBuildable" check (prerequisites) will prevent
      // out-of-order building.
      info->setAutomaticBuild(true);
    }
  }
}

//-------------------------------------------------------------------------------------------------
