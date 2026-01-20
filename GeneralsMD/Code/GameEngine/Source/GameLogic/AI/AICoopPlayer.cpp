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
#include "GameClient/ControlBar.h"
#include "GameLogic/TerrainLogic.h" // For Waypoint class
#include <new>                      // For placement new

AICoopPlayer::AICoopPlayer(Player *p) : AISkirmishPlayer(p) {
  DjLog("AICoopPlayer created for player %d (%ls) Side: %s",
        p->getPlayerIndex(), p->getPlayerDisplayName().str(),
        p->getSide().str());
  m_skirmishScriptsLoaded = false;

  // Initialize waypoint system (NEW)
  m_airPatrolPath = NULL;
  m_airPatrolInitialized = false;

  // Clear combat optimization log
  FILE *f = fopen("d:\\djcc_combat.txt", "w");
  if (f) {
    fprintf(f, "Combat Optimization Log Started for Player %d\n",
            p->getPlayerIndex());
    fclose(f);
  }
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
  if (TheGameLogic->getFrame() % 1800 == 0) {
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
  // Use doBaseBuilding() which handles timing checks correctly
  DjLog("AICoopPlayer::assistHumanPlayer - BEFORE doBaseBuilding: "
        "m_structureTimer=%d, m_readyToBuildStructure=%d",
        m_structureTimer, m_readyToBuildStructure ? 1 : 0);
  doBaseBuilding();

  // 2. Manage idle dozers
  autoManageIdleDozers();

  // 3. Manage idle units
  autoManageIdleUnits();

  // 4. Auto-defend base
  // autoDefendBase(); // Still disabled as per original code

  // 4b. Auto-engage enemies with combat teams
  autoManageCombatTeams();
  // 4c. Auto-upgrade Overlords
  autoManageOverlordUpgrades();
  // 4d. Manage aircraft patrol (NEW)
  manageAirUnits();

  // 5. Unit Production and Team Management
  // This drives the AI's ability to build units using the Skirmish Teams loaded
  // earlier.
  if (m_skirmishScriptsLoaded) {
    // Ensure the "AI" (acting for human) is allowed to build units
    if (!m_player->getCanBuildUnits()) {
      m_player->setCanBuildUnits(true);
    }

    // Standard AI update cycle for teams and upgrades
    checkReadyTeams();     // Start teams that are ready
    checkQueuedTeams();    // Check if queued teams are finished
    doTeamBuilding();      // Decide what team to build next
    doUpgradesAndSkills(); // Purchase upgrades or general abilities
  }
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

        // DISABLE: Do not send idle units back to base.
        // This interferes with manual commands and capture missions (e.g. Oil
        // Derricks). The AI should leave them where they are (holding ground).
        // ai->aiGuardPosition(&baseCenter, GUARDMODE_NORMAL, CMD_FROM_AI);
      }
    }
  }

  if (idleUnitCount > 0) {
    // Log finding them, but don't move them.
    // DjLog("AICoopPlayer: Found %d idle units (not recalling)",
    // idleUnitCount);
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

void AICoopPlayer::autoManageCombatTeams() {
  // Performance optimization: Check every 15 frames (approx 0.5 seconds)
  if (TheGameLogic->getFrame() % 15 != 0) {
    return;
  }

  // LOGGING: Detailed combat analysis for optimization
  FILE *combatLog = fopen("d:\\djcc_combat.txt", "a");

  // Iterate through all teams of this player
  Player::PlayerTeamList::const_iterator it;
  for (it = m_player->getPlayerTeams()->begin();
       it != m_player->getPlayerTeams()->end(); ++it) {

    for (DLINK_ITERATOR<Team> iter = (*it)->iterate_TeamInstanceList();
         !iter.done(); iter.advance()) {
      Team *team = iter.cur();
      if (!team || !team->isActive())
        continue;

      // Skip base defense teams or non-combat teams if necessary
      // But user wants "moving teams", so we generally check all active teams.

      // We need a representative unit to check proximity
      Object *representative = NULL;

      // First pass: Find a representative and check if ANYONE is already
      // fighting
      // Bool isAlreadyFighting = false;

      for (DLINK_ITERATOR<Object> objIter = team->iterate_TeamMemberList();
           !objIter.done(); objIter.advance()) {
        Object *obj = objIter.cur();
        if (!obj || obj->isEffectivelyDead())
          continue;

        if (obj->isKindOf(KINDOF_CAN_ATTACK) &&
            !obj->isKindOf(KINDOF_STRUCTURE)) {

          // Representative Logic:
          // 1. If we find an Overlord class, it IMMEDIATELY becomes the
          // representative (High Priority).
          // 2. Otherwise, take the first combat unit we find.

          bool isOverlord = false;
          // Check for Overlord/Emperor by name or kindof (using name pattern
          // for safety) Emperor is usually "ChinaTankEmperor", Overlord is
          // "ChinaTankOverlord"
          const ThingTemplate *tmpl = obj->getTemplate();
          if (tmpl && (strstr(tmpl->getName().str(), "Overlord") ||
                       strstr(tmpl->getName().str(), "Emperor"))) {
            isOverlord = true;
          }

          if (isOverlord) {
            representative = obj;
            // We found our heavy tank anchor. The search for a "better"
            // representative is done. We continue iterating just to ensure we
            // didn't miss anything else if needed, but 'representative' will
            // stick to this Overlord unless we find... another Overlord?
            // Actually, the first Overlord is fine.
            break;
          }

          if (!representative)
            representative = obj;
        }
      }

      if (!representative)
        continue;

      // Search for enemy near the representative
      // Radius 250 is standard vision/aggro range
      Object *enemy =
          TheAI->findClosestEnemy(representative, 250.0f, AI::CAN_ATTACK, NULL);

      if (enemy) {
        // LOGGING
        if (combatLog) {
          Real distSq = 0.0f;
          const Coord3D *p1 = representative->getPosition();
          const Coord3D *p2 = enemy->getPosition();
          if (p1 && p2)
            distSq = (p1->x - p2->x) * (p1->x - p2->x) +
                     (p1->y - p2->y) * (p1->y - p2->y);
          fprintf(combatLog,
                  "Frame %u: Team %d guarding loc against Enemy %u (DistSq: "
                  "%.2f)\n",
                  TheGameLogic->getFrame(), team->getID(), enemy->getID(),
                  distSq);
        }

        // Enemy sighted! GUARD!
        // Issue guard command to all capable members
        for (DLINK_ITERATOR<Object> attackIter = team->iterate_TeamMemberList();
             !attackIter.done(); attackIter.advance()) {
          Object *attacker = attackIter.cur();
          if (!attacker || attacker->isEffectivelyDead())
            continue;
          if (!attacker->isKindOf(KINDOF_CAN_ATTACK) ||
              attacker->isKindOf(KINDOF_STRUCTURE))
            continue;
          if (attacker->isKindOf(KINDOF_DOZER))
            continue;

          AIUpdateInterface *ai = attacker->getAIUpdateInterface();
          if (!ai)
            continue;

          // Guard the position of the representative (front line).
          // This ensures the entire team moves to support the engagement,
          // preventing rear units from staying idle.

          // Fix for "stuttering":
          // If already guarding this location (or very close), DO NOT re-issue
          // command. Re-issuing causes the unit to stop, think, and restart
          // pathfinding.
          bool alreadyGuarding = false;
          if (ai->getGuardTargetType() == GUARDTARGET_LOCATION) {
            const Coord3D *currentGuardPos = ai->getGuardLocation();
            const Coord3D *targetPos = representative->getPosition();
            if (currentGuardPos && targetPos) {
              Real dX = currentGuardPos->x - targetPos->x;
              Real dY = currentGuardPos->y - targetPos->y;
              // If within 20 units (very close), consider it the same order
              if ((dX * dX + dY * dY) < 400.0f) {
                alreadyGuarding = true;
              }
            }
          }

          if (!alreadyGuarding) {
            ai->aiGuardPosition(representative->getPosition(), GUARDMODE_NORMAL,
                                CMD_FROM_AI);
          }
        }
      }
    }
  }
  if (combatLog)
    fclose(combatLog);
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

//=============================================================================
// Reactive Defense Logic
//=============================================================================

AICoopPlayer::ThreatType AICoopPlayer::assessGlobalThreat() {
  int infantryCount = 0;
  int vehicleCount = 0;
  int airCount = 0;
  int structureCount = 0;

  // Scan all enemies
  for (Int i = 0; i < ThePlayerList->getPlayerCount(); i++) {
    Player *other = ThePlayerList->getNthPlayer(i);
    // Skip self and allies
    if (other == m_player ||
        m_player->getRelationship(other->getDefaultTeam()) == ALLIES) {
      continue;
    }

    // Iterate through enemy teams/objects
    Player::PlayerTeamList::const_iterator it;
    for (it = other->getPlayerTeams()->begin();
         it != other->getPlayerTeams()->end(); ++it) {
      for (DLINK_ITERATOR<Team> iter = (*it)->iterate_TeamInstanceList();
           !iter.done(); iter.advance()) {
        Team *team = iter.cur();
        if (!team)
          continue;

        for (DLINK_ITERATOR<Object> objIter = team->iterate_TeamMemberList();
             !objIter.done(); objIter.advance()) {
          Object *obj = objIter.cur();
          if (!obj || obj->isEffectivelyDead())
            continue;

          if (obj->isKindOf(KINDOF_INFANTRY))
            infantryCount++;
          else if (obj->isKindOf(KINDOF_VEHICLE))
            vehicleCount++;
          else if (obj->isKindOf(KINDOF_AIRCRAFT))
            airCount++;
          else if (obj->isKindOf(KINDOF_STRUCTURE))
            structureCount++;
        }
      }
    }
  }

  // Determine dominant threat (simple heuristic)
  if (airCount > 5)
    return THREAT_AIR;
  if (vehicleCount > 5)
    return THREAT_VEHICLE;
  if (infantryCount > 10)
    return THREAT_INFANTRY;
  if (structureCount > 0)
    return THREAT_STRUCTURE;

  return THREAT_NONE;
}

void AICoopPlayer::updateBuildPriorities() {
  // Called every frame to dynamically update AutomaticBuild flags
  // Hybrid Logic:
  // 1. Power Plants: Smart management to fix 0/0 deadlock AND prevent spam.
  // 2. Reactive Tech: Unlock tech structures based on threat and money.
  // 3. Others: Enable All to let Engine/AI prerequisites handle the flow.

  Bool hasSufficientPower = m_player->getEnergy()->hasSufficientPower();
  int powerPlantCount = countStructures("PowerPlant");
  int powerPlantBuilding = countStructuresUnderConstruction("PowerPlant");

  // Reactive Defense Check (every 30 frames to save perf)
  ThreatType currentThreat = THREAT_NONE;
  if (TheGameLogic->getFrame() % 30 == 0) {
    currentThreat = assessGlobalThreat();
  }

  int money = m_player->getMoney()->countMoney();

  for (BuildListInfo *info = m_player->getBuildList(); info;
       info = info->getNext()) {
    AsciiString name = info->getTemplateName();

    if (strstr(name.str(), "PowerPlant")) {
      // Logic: Build if (No PPs exist) OR (Power is Low).
      Bool shouldBuild =
          (powerPlantCount + powerPlantBuilding) < 1 || !hasSufficientPower;
      info->setAutomaticBuild(shouldBuild);
    } else {
      // Default: Enable everything
      info->setAutomaticBuild(true);

      // REACTIVE LOGIC: Prioritize Tech Centers if money is high and threat
      // exists
      if (money > 4000) {
        bool isTech =
            strstr(name.str(), "PropagandaCenter") ||
            strstr(name.str(), "StrategyCenter") ||
            strstr(name.str(), "Palace") ||
            strstr(
                name.str(),
                "InternetCenter"); // GLA Black Market equivalent logic usually

        if (isTech && currentThreat == THREAT_VEHICLE) {
          // We ensure it's enabled (it is), but we could log it.
          // For now, the existing logic enables it.
          // Future refinement: Disable 'Barracks' if we desperately need this
          // Tech? Not implemented yet to avoid stalling.
          if (TheGameLogic->getFrame() % 900 == 0) {
            DjLog("AICoopPlayer: Reactive Defense - Rushing Tech %s to counter "
                  "Vehicle Threat.",
                  name.str());
          }
        }
      }
    }
  }
}

//=============================================================================
// Overlord Auto-Upgrade Logic
//=============================================================================
void AICoopPlayer::autoManageOverlordUpgrades() {
  // Check every 30 frames (1 second) to save performance
  if (TheGameLogic->getFrame() % 30 != 0) {
    return;
  }

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
        if (!obj || obj->isEffectivelyDead())
          continue;

        // Identify Overlord or Emperor
        const ThingTemplate *tmpl = obj->getTemplate();
        bool isOverlord = false;
        if (tmpl && (strstr(tmpl->getName().str(), "Overlord") ||
                     strstr(tmpl->getName().str(), "Emperor"))) {
          isOverlord = true;
        }

        if (!isOverlord)
          continue;

        // UPGRADE LOGIC
        // Priority 1: Propaganda Tower (Healing)
        // Priority 2: Gattling Cannon (Anti-Air/Infantry)
        // Priority 3: Bunker (Infantry transport - rarely used by AI unless
        // scripted)

        AIUpdateInterface *ai = obj->getAIUpdateInterface();
        if (!ai)
          continue;

        // Check funds
        int money = m_player->getMoney()->countMoney();
        if (money < 2000)
          continue; // Save money for production

        // Command Button names for China Overlord
        // "Command_UpgradeChinaOverlordPropagandaTower"
        // "Command_UpgradeChinaOverlordGattlingCannon"

        // We attempt to purchase Propaganda Tower first.
        // Since we cannot easily check "Is Upgraded" without iterating Upgrade
        // module, we heavily rely on the fact that "useCommandButton" will
        // fail/do nothing if already upgraded or invalid. We just Spam the
        // request every few seconds.

        if (money < 2000)
          continue; // Minimum funds check

        // Try to purchase Propaganda Tower upgrade
        if (TheGameLogic->getFrame() % 60 ==
            (obj->getID() % 60)) { // Slightly randomized timing
          if (TheControlBar) {
            const CommandButton *cbProp = TheControlBar->findCommandButton(
                "Command_UpgradeChinaOverlordPropagandaTower");
            if (cbProp) {
              // Check if we already have the upgrade to avoid redundant
              // commands
              const UpgradeTemplate *upg = cbProp->getUpgradeTemplate();
              if (!upg || !obj->hasUpgrade(upg)) {
                obj->doCommandButton(cbProp, CMD_FROM_AI);
              }
            }
          }
        }
        // If we have LOTS of money, try Gattling too (if Prop tower fails or
        // is built?) actually, Overlord can only have ONE. So we should stick
        // to Propaganda Tower as the best default for "Anchor" logic.
      }
    }
  }
}

//=============================================================================
// Aircraft Waypoint Management (NEW)
//=============================================================================

void AICoopPlayer::initializeAirPatrol() {
  if (m_airPatrolInitialized) {
    return; // Already initialized
  }

  DjLog("AICoopPlayer: Initializing Air Patrol Waypoints...");

  // Get base center for patrol path
  Coord3D baseCenter;
  if (!getBaseCenter(&baseCenter)) {
    DjLog("AICoopPlayer: No base center found, skipping air patrol "
          "initialization.");
    return;
  }

  // Create patrol waypoints in a circle around the base
  Real patrolRadius = 500.0f;
  Real airHeight = 100.0f; // Fly above ground

  // Use stack-allocated Coord3D structs (Waypoint copies the data)
  Coord3D loc1, loc2, loc3, loc4;

  loc1.x = baseCenter.x;
  loc1.y = baseCenter.y + patrolRadius;
  loc1.z = baseCenter.z + airHeight;

  loc2.x = baseCenter.x + patrolRadius;
  loc2.y = baseCenter.y;
  loc2.z = baseCenter.z + airHeight;

  loc3.x = baseCenter.x;
  loc3.y = baseCenter.y - patrolRadius;
  loc3.z = baseCenter.z + airHeight;

  loc4.x = baseCenter.x - patrolRadius;
  loc4.y = baseCenter.y;
  loc4.z = baseCenter.z + airHeight;

  // Create waypoints using PLACEMENT NEW to bypass MemoryPool!
  // We allocate memory from the global heap, then construct the object there.
  Int baseIDInt = (m_player->getPlayerIndex() * 10000) + 9000;

  void *mem1 = ::operator new(sizeof(Waypoint));
  Waypoint *wp1 =
      new (mem1) Waypoint(static_cast<WaypointID>(baseIDInt + 1),
                          "AirPatrol_North", &loc1, "AirDefense", "", "", true);

  void *mem2 = ::operator new(sizeof(Waypoint));
  Waypoint *wp2 =
      new (mem2) Waypoint(static_cast<WaypointID>(baseIDInt + 2),
                          "AirPatrol_East", &loc2, "AirDefense", "", "", true);

  void *mem3 = ::operator new(sizeof(Waypoint));
  Waypoint *wp3 =
      new (mem3) Waypoint(static_cast<WaypointID>(baseIDInt + 3),
                          "AirPatrol_South", &loc3, "AirDefense", "", "", true);

  void *mem4 = ::operator new(sizeof(Waypoint));
  Waypoint *wp4 =
      new (mem4) Waypoint(static_cast<WaypointID>(baseIDInt + 4),
                          "AirPatrol_West", &loc4, "AirDefense", "", "", true);

  // Link them in a circular path: 1->2->3->4->1
  wp1->addLink(wp2);
  wp2->addLink(wp3);
  wp3->addLink(wp4);
  wp4->addLink(wp1);

  m_airPatrolPath = wp1; // Start from North waypoint
  m_airPatrolInitialized = true;

  DjLog("AICoopPlayer: Air Patrol Path created with 4 waypoints around base "
        "(%.1f, %.1f).",
        baseCenter.x, baseCenter.y);
}

void AICoopPlayer::manageAirUnits() {
  // Check every 60 frames (2 seconds)
  if (TheGameLogic->getFrame() % 60 != 0) {
    return;
  }

  // Initialize patrol path if needed
  if (!m_airPatrolInitialized) {
    initializeAirPatrol();
    if (!m_airPatrolInitialized) {
      return; // Init failed
    }
  }

  // Find all aircraft and assign patrol if idle
  Player::PlayerTeamList::const_iterator it;
  for (it = m_player->getPlayerTeams()->begin();
       it != m_player->getPlayerTeams()->end(); ++it) {

    for (DLINK_ITERATOR<Team> teamIter = (*it)->iterate_TeamInstanceList();
         !teamIter.done(); teamIter.advance()) {
      Team *team = teamIter.cur();
      if (!team)
        continue;

      for (DLINK_ITERATOR<Object> objIter = team->iterate_TeamMemberList();
           !objIter.done(); objIter.advance()) {
        Object *obj = objIter.cur();
        if (!obj || obj->isEffectivelyDead())
          continue;

        // Filter: Only aircraft
        if (!obj->isKindOf(KINDOF_AIRCRAFT))
          continue;

        AIUpdateInterface *ai = obj->getAIUpdateInterface();
        if (!ai)
          continue;

        // If idle, send on patrol
        if (ai->isIdle()) {
          DjLog("AICoopPlayer: Assigning air patrol to Aircraft %d (%s)",
                obj->getID(), obj->getTemplate()->getName().str());
          ai->aiFollowWaypointPathAsTeam(m_airPatrolPath, CMD_FROM_AI);
        }
      }
    }
  }
}

void AICoopPlayer::checkAircraftAmmo() {
  // Check every 30 frames (1 second)
  if (TheGameLogic->getFrame() % 30 != 0) {
    return;
  }

  // NOTE: This is a placeholder. The engine may not expose ammo count directly.
  // For now, we'll use a simple time-based heuristic: if aircraft has been
  // active for awhile, send it back to airfield.

  // TODO: Implement proper ammo checking if engine API supports it
  // For now, this function is a stub for future enhancement
}

Object *AICoopPlayer::findNearestAirfield(Object *aircraft) {
  if (!aircraft)
    return NULL;

  Object *closestAirfield = NULL;
  Real closestDistSq = 999999.0f;

  const Coord3D *aircraftPos = aircraft->getPosition();
  if (!aircraftPos)
    return NULL;

  // Search all player's structures for an Airfield
  Player::PlayerTeamList::const_iterator it;
  for (it = m_player->getPlayerTeams()->begin();
       it != m_player->getPlayerTeams()->end(); ++it) {

    for (DLINK_ITERATOR<Team> teamIter = (*it)->iterate_TeamInstanceList();
         !teamIter.done(); teamIter.advance()) {
      Team *team = teamIter.cur();
      if (!team)
        continue;

      for (DLINK_ITERATOR<Object> objIter = team->iterate_TeamMemberList();
           !objIter.done(); objIter.advance()) {
        Object *obj = objIter.cur();
        if (!obj || obj->isEffectivelyDead())
          continue;

        // Must be a structure
        if (!obj->isKindOf(KINDOF_STRUCTURE))
          continue;

        // Check if it's an Airfield by name
        const ThingTemplate *tmpl = obj->getTemplate();
        if (!tmpl)
          continue;

        const char *name = tmpl->getName().str();
        if (!strstr(name, "Airfield") && !strstr(name, "Helipad")) {
          continue; // Not an airfield
        }

        // Calculate distance
        const Coord3D *airfieldPos = obj->getPosition();
        if (!airfieldPos)
          continue;

        Real dx = aircraftPos->x - airfieldPos->x;
        Real dy = aircraftPos->y - airfieldPos->y;
        Real distSq = dx * dx + dy * dy;

        if (distSq < closestDistSq) {
          closestDistSq = distSq;
          closestAirfield = obj;
        }
      }
    }
  }

  return closestAirfield;
}
