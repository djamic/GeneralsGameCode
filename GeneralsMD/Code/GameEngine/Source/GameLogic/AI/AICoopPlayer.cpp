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
#include "Common/BuildAssistant.h"
#include "Common/NameKeyGenerator.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/Team.h"
#include "Common/Thing.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/WellKnownKeys.h"
#include "GameLogic/AI.h"         // For TheAI and Pathfinding
#include "GameLogic/AIPathfind.h" // For Path/PathNode
#include "GameLogic/AIPlayer.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Object.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/SidesList.h"
#include "PreRTS.h" // Must be first

// #include "PreRTS.h" // MOVED TO TOP

#include "Common/DjDebug.h"
#include "GameClient/ControlBar.h"
#include "GameLogic/Module/ContainModule.h"    // For Internet Center contain
#include "GameLogic/Module/ProductionUpdate.h" // For hacker production
#include "GameLogic/TerrainLogic.h"            // For Waypoint class
#include <algorithm>                           // For std::sort
#include <new>                                 // For placement new
#include <vector>                              // For capture system

AICoopPlayer::AICoopPlayer(Player *p) : AISkirmishPlayer(p) {
  DjLog("AICoopPlayer created for player %d (%ls) Side: %s",
        p->getPlayerIndex(), p->getPlayerDisplayName().str(),
        p->getSide().str());
  m_skirmishScriptsLoaded = false;

  // Initialize waypoint system (NEW)
  m_airPatrolPath = NULL;
  m_airPatrolInitialized = false;

  // Initialize Hacker Barracks System (2026-01-26)
  m_hackerBarracksID = INVALID_ID;
  m_hackerBarracksBuilding = false;
  m_lastHackerProductionFrame = 0;
  m_hackerBarracksBuildPos.x = 0.0f;
  m_hackerBarracksBuildPos.y = 0.0f;
  m_hackerBarracksBuildPos.z = 0.0f;

  // Initialize Tech Building Capture System (2026-01-29)
  m_lastCaptureCheckFrame = 0;
  // m_buildingToSoldier map is auto-initialized empty

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

  // TECH BUILDING CAPTURE (Auto-Capture)
  // Run this periodically to capture oil derricks, etc.
  autoCaptureTechBuildings();
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
  if (TheGameLogic->getFrame() % 1800 == 0) {
    DjLog("AICoopPlayer::assistHumanPlayer - Assisting player %d",
          m_player->getPlayerIndex());
  }

  // TECH BUILDING CAPTURE (Auto-Capture for Human Assist Mode)
  autoCaptureTechBuildings();

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

            // Debug Log for high-interest teams
            if (strstr(teamName.str(), "Comanche") ||
                strstr(teamName.str(), "Helix") ||
                strstr(teamName.str(), "Mig") ||
                strstr(teamName.str(), "Raptor")) {
              DjLog("AICoopPlayer: Dedicated Air Team LOADED: %s",
                    teamName.str());
            }

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
  // 4e. Hacker Management System (NEW - 2026-01-25)
  autoManageHackers();
  // 4f. Tech Building Capture (NEW - 2026-01-28)
  autoCaptureTechBuildings();

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
  // Performance optimization: Check every 30 frames (approx 1 second)
  // Increased from 15 to reduce command frequency
  if (TheGameLogic->getFrame() % 30 != 0) {
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

      // We need a representative unit to check proximity
      Object *representative = NULL;

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
          const ThingTemplate *tmpl = obj->getTemplate();
          if (tmpl && (strstr(tmpl->getName().str(), "Overlord") ||
                       strstr(tmpl->getName().str(), "Emperor"))) {
            isOverlord = true;
          }

          if (isOverlord) {
            representative = obj;
            break;
          }

          if (!representative)
            representative = obj;
        }
      }

      if (!representative)
        continue;

      // Search for enemy near the representative (just to check if threat
      // exists) Radius 250 is standard vision/aggro range
      Object *enemy =
          TheAI->findClosestEnemy(representative, 250.0f, AI::CAN_ATTACK, NULL);

      if (enemy) {
        // Enemy sighted! Each unit attacks its OWN closest enemy
        int commandsIssued = 0;

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

          // FIX #1: Skip if unit is already attacking
          // This prevents interrupting an ongoing attack
          if (ai->isAttacking()) {
            continue;
          }

          // FIX #6: Per-unit cooldown check (2026-01-25)
          // Skip if this unit received an attack command recently
          ObjectID attackerID = attacker->getID();
          UnsignedInt currentFrame = TheGameLogic->getFrame();
          std::map<ObjectID, UnsignedInt>::iterator cooldownIt =
              m_lastGuardCommandFrame.find(attackerID);
          if (cooldownIt != m_lastGuardCommandFrame.end()) {
            if (currentFrame - cooldownIt->second <
                GUARD_COMMAND_COOLDOWN_FRAMES) {
              continue; // Still in cooldown, skip this unit
            }
          }

          // FIX #7: Each unit finds its OWN closest enemy (2026-01-25)
          // This prevents units from running through enemy lines
          Object *myEnemy =
              TheAI->findClosestEnemy(attacker, 250.0f, AI::CAN_ATTACK, NULL);

          if (myEnemy) {
            // Attack my closest enemy, not the team's enemy
            ai->aiAttackObject(myEnemy, INT_MAX, CMD_FROM_AI);

            // Update cooldown map
            m_lastGuardCommandFrame[attackerID] = currentFrame;
            commandsIssued++;
          }
        }

        // FIX #5: Only log when commands were actually issued
        if (combatLog && commandsIssued > 0) {
          Real distSq = 0.0f;
          const Coord3D *p1 = representative->getPosition();
          const Coord3D *p2 = enemy->getPosition();
          if (p1 && p2)
            distSq = (p1->x - p2->x) * (p1->x - p2->x) +
                     (p1->y - p2->y) * (p1->y - p2->y);
          fprintf(
              combatLog,
              "Frame %u: Team %d issued %d ATTACK commands against Enemy %u "
              "(DistSq: "
              "%.2f)\n",
              TheGameLogic->getFrame(), team->getID(), commandsIssued,
              enemy->getID(), distSq);
        }
      }
    }
  }
  if (combatLog)
    fclose(combatLog);

  // Periodically clean up the cooldown map (every 300 frames = 10 seconds)
  if (TheGameLogic->getFrame() % 300 == 0) {
    cleanupGuardCooldownMap();
  }
}

//=============================================================================
// Guard Cooldown Map Cleanup
//=============================================================================

void AICoopPlayer::cleanupGuardCooldownMap() {
  UnsignedInt currentFrame = TheGameLogic->getFrame();

  // Remove entries that are too old (older than 2x cooldown period)
  // This prevents the map from growing unbounded with dead unit IDs
  std::map<ObjectID, UnsignedInt>::iterator it =
      m_lastGuardCommandFrame.begin();
  while (it != m_lastGuardCommandFrame.end()) {
    if (currentFrame - it->second > GUARD_COMMAND_COOLDOWN_FRAMES * 2) {
      it = m_lastGuardCommandFrame.erase(it);
    } else {
      ++it;
    }
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

      // PRIORITIZE SUPPLY CENTER: Basic economy and prereq for
      // Airfield/WarFactory
      if (money > 2000) {
        bool isSupply = strstr(name.str(), "SupplyCenter");
        if (isSupply) {
          int supplyCount = countStructures("SupplyCenter");
          int supplyBuilding = countStructuresUnderConstruction("SupplyCenter");
          if ((supplyCount + supplyBuilding) == 0) {
            if (TheGameLogic->getFrame() % 300 == 0) {
              DjLog("AICoopPlayer: Strategy - Prioritizing SupplyCenter %s "
                    "(Money > 2000)",
                    name.str());
            }
            // It's already set to AutomaticBuild=true by the default else
            // block, but this confirms our intent and ensures we track it.
          }
        }
      }

      // PRIORITIZE AIRFIELD: If we have money, build Airfield ASAP to enable
      // helicopters.
      if (money > 3000) {
        bool isAirfield =
            strstr(name.str(), "Airfield") || strstr(name.str(), "Helipad");
        if (isAirfield) {
          // Check if we already have one
          int airCount =
              countStructures("Airfield") + countStructures("Helipad");
          int airBuilding = countStructuresUnderConstruction("Airfield") +
                            countStructuresUnderConstruction("Helipad");

          if ((airCount + airBuilding) == 0) {
            // None found! Prioritize this build!
            // Note: 'markPriorityBuild()' is not available on BuildListInfo
            // directly in this scope easily? Actually it is:
            // info->markPriorityBuild() usually exists or accessible via
            // specific helpers AISkirmishPlayer uses it. Let's check
            // BuildListInfo in BuildAssistant.h or simply assume logic.

            // Wait, AISkirmishPlayer uses: m_player->addToPriorityBuildList...
            // info->isPriorityBuild() is a check.

            // Let's assume we can set it via scripts or just ensure it's
            // automatic. Since we are in Assist mode, we rely on standard build
            // loop picking it up. To force it, we might need a stronger signal,
            // but for now ensure it is AUTOMATIC. It IS automatic (set above).

            // Let's Log it.
            if (TheGameLogic->getFrame() % 300 == 0) {
              DjLog("AICoopPlayer: Strategy - Prioritizing Airfield %s (Money "
                    "> 3000)",
                    name.str());
            }
            // If we could 'force' it, we would. But 'AutomaticBuild=true' is
            // usually enough IF the build assistant reaches it.
          }
        }
      }
    }

    // CRITICAL FIX: Ensure SupplyCenter is in the build list.
    // If it's missing, the Dozer can't build it, and Airfield stays blocked.
    // We check this periodically (e.g. Frame 200, then rarely) to ensure
    // injection happens.
    if (TheGameLogic->getFrame() == 200 ||
        (TheGameLogic->getFrame() % 900 == 0)) {
      bool hasSupplyInList = false;
      for (BuildListInfo *info = m_player->getBuildList(); info;
           info = info->getNext()) {
        if (strstr(info->getTemplateName().str(), "SupplyCenter")) {
          hasSupplyInList = true;
          break;
        }
      }

      if (!hasSupplyInList) {
        // It's missing! Force inject it.
        DjLog("AICoopPlayer: CRITICAL - SupplyCenter missing from Build List "
              "for %s. Injecting it!",
              m_player->getPlayerDisplayName().str());

        // Determine template name based on side
        AsciiString supplyName;
        if (strstr(m_player->getSide().str(), "China"))
          supplyName = "ChinaSupplyCenter";
        else if (strstr(m_player->getSide().str(), "America"))
          supplyName = "AmericaSupplyCenter";
        else if (strstr(m_player->getSide().str(), "GLA"))
          supplyName = "GLASupplyStash"; // Or Chem_GLASupplyStash, etc.

        if (!supplyName.isEmpty()) {
          const ThingTemplate *t = TheThingFactory->findTemplate(supplyName);
          if (t) {
            // Create new info.
            // BuildListInfo::operator new is protected. Duplicate existing
            // valid item.
            BuildListInfo *validItem = m_player->getBuildList();
            if (validItem) {
              BuildListInfo *newInfo = validItem->duplicate();

              // Reset and Configure
              newInfo->setTemplateName(supplyName);
              newInfo->setBuildingName(supplyName);
              newInfo->setAutomaticBuild(true);
              newInfo->setNextBuildList(NULL);
              newInfo->setInitiallyBuilt(false);
              newInfo->setNumRebuilds(BuildListInfo::UNLIMITED_REBUILDS);

              // Use Smart Placement (Spiral Search)
              Coord3D bestLoc;
              if (findValidBuildLocation(t, &bestLoc)) {
                newInfo->setLocation(bestLoc);
                DjLog("AICoopPlayer: Injected SupplyCenter at VALID location "
                      "(%.1f, %.1f)",
                      bestLoc.x, bestLoc.y);
              } else {
                DjLog("AICoopPlayer: WARN - Smart Placement failed! Defaulting "
                      "to offset.");
                Coord3D baseCenter;
                if (getBaseCenter(&baseCenter)) {
                  baseCenter.x += 300.0f;
                  baseCenter.y += 300.0f;
                  newInfo->setLocation(baseCenter);
                }
              }

              // Append to end
              BuildListInfo *last = m_player->getBuildList();
              if (last) {
                while (last->getNext())
                  last = last->getNext();
                last->setNextBuildList(newInfo);
              }
              DjLog("AICoopPlayer: Injected %s into build list.",
                    supplyName.str());
            } else {
              DjLog("AICoopPlayer: Cannot inject %s - BuildList empty!",
                    supplyName.str());
            }
          } else {
            DjLog("AICoopPlayer: Failed to find template %s for injection.",
                  supplyName.str());
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

//=============================================================================
// Smart Placement Logic (NEW)
//=============================================================================

//=============================================================================
// Hacker Management System (NEW - 2026-01-25)
// Auto-manages hackers: gathers to safe zone, starts hacking, produces more
//=============================================================================

Object *AICoopPlayer::findInternetCenter() {
  // Find player's Internet Center using correct API
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

        if (obj->isKindOf(KINDOF_FS_INTERNET_CENTER)) {
          return obj;
        }
      }
    }
  }
  return NULL;
}

//=============================================================================
// Weighted Scoring Hacker Placement v2 (2026-01-25)
// S = (W1 × Dthreat) + (W2 × Dborder) + (W3 × Ddefense)
//=============================================================================

void AICoopPlayer::getEnemyBaseCenter(Coord3D *outPos) {
  // Get current enemy from parent class
  Player *enemy = getAiEnemy();
  if (!enemy) {
    outPos->x = 2000.0f;
    outPos->y = 2000.0f;
    outPos->z = 0.0f;
    return;
  }

  // Try to get enemy's base center
  Region2D bounds;
  getPlayerStructureBounds(&bounds, enemy->getPlayerIndex());
  outPos->x = bounds.lo.x + bounds.width() / 2;
  outPos->y = bounds.lo.y + bounds.height() / 2;
  outPos->z = 0.0f;
}

Real AICoopPlayer::distanceToMapBorder(const Coord3D &pos) {
  // Get map bounds
  Region3D mapBounds;
  TheTerrainLogic->getMaximumPathfindExtent(&mapBounds);

  // Calculate distance to each border
  Real distLeft = pos.x - mapBounds.lo.x;
  Real distRight = mapBounds.hi.x - pos.x;
  Real distBottom = pos.y - mapBounds.lo.y;
  Real distTop = mapBounds.hi.y - pos.y;

  // Return minimum distance to any border
  Real minDist = distLeft;
  if (distRight < minDist)
    minDist = distRight;
  if (distBottom < minDist)
    minDist = distBottom;
  if (distTop < minDist)
    minDist = distTop;

  return minDist;
}

Object *AICoopPlayer::findNearestDefenseStructure(const Coord3D &pos) {
  Object *nearest = NULL;
  Real nearestDistSq = 99999999.0f;

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

        // Check if it's a defense structure
        if (!obj->isKindOf(KINDOF_STRUCTURE))
          continue;

        // Check for defense-type structures
        const ThingTemplate *tmpl = obj->getTemplate();
        if (!tmpl)
          continue;
        const char *name = tmpl->getName().str();

        // China: Bunker, Gatling Cannon
        // USA: Patriot
        // GLA: Tunnel Network, Stinger Site
        if (!strstr(name, "Bunker") && !strstr(name, "Gatling") &&
            !strstr(name, "Patriot") && !strstr(name, "Tunnel") &&
            !strstr(name, "Stinger") && !strstr(name, "Defense")) {
          continue;
        }

        const Coord3D *objPos = obj->getPosition();
        if (!objPos)
          continue;

        Real dx = pos.x - objPos->x;
        Real dy = pos.y - objPos->y;
        Real distSq = dx * dx + dy * dy;

        if (distSq < nearestDistSq) {
          nearestDistSq = distSq;
          nearest = obj;
        }
      }
    }
  }

  return nearest;
}

Real AICoopPlayer::evaluateHackerPosition(const Coord3D &pos,
                                          const Coord3D &baseRef) {
  // Weights
  const Real W_OPPOSITE = 3.0f; // HIGHEST: Being on opposite side from enemy!
  const Real W_AIR = 1.5f;      // Air corridor distance
  const Real W_BORDER = 0.5f;   // Border proximity
  const Real W_DEFENSE = 0.8f;  // Defense proximity

  // Get enemy base center
  Coord3D enemyBase;
  getEnemyBaseCenter(&enemyBase);

  //=========================================================================
  // D_OPPOSITE: CRITICAL - Position should be on OPPOSITE side from enemy
  //=========================================================================
  // Vector from base to enemy
  Real toEnemyX = enemyBase.x - baseRef.x;
  Real toEnemyY = enemyBase.y - baseRef.y;
  Real toEnemyLen = sqrtf(toEnemyX * toEnemyX + toEnemyY * toEnemyY);

  // Vector from base to candidate position
  Real toPosX = pos.x - baseRef.x;
  Real toPosY = pos.y - baseRef.y;
  Real toPosLen = sqrtf(toPosX * toPosX + toPosY * toPosY);

  Real oppositeScore = 0.0f;
  if (toEnemyLen > 1.0f && toPosLen > 1.0f) {
    // Normalize vectors
    toEnemyX /= toEnemyLen;
    toEnemyY /= toEnemyLen;
    toPosX /= toPosLen;
    toPosY /= toPosLen;

    // Dot product: -1 = opposite, +1 = same direction
    Real dot = toEnemyX * toPosX + toEnemyY * toPosY;

    // Convert: -1 -> +200 (opposite = BEST), +1 -> 0 (same direction = BAD)
    oppositeScore = (1.0f - dot) * 100.0f * W_OPPOSITE;
  }

  //=========================================================================
  // D_AIR: Distance from air corridor (perpendicular)
  //=========================================================================
  Real airDirX = baseRef.x - enemyBase.x;
  Real airDirY = baseRef.y - enemyBase.y;
  Real airLen = sqrtf(airDirX * airDirX + airDirY * airDirY);

  Real airDist = 0.0f;
  if (airLen > 1.0f) {
    airDirX /= airLen;
    airDirY /= airLen;
    Real toPointX = pos.x - enemyBase.x;
    Real toPointY = pos.y - enemyBase.y;
    airDist = fabsf(toPointX * (-airDirY) + toPointY * airDirX);
  }
  Real airScore = airDist * W_AIR;

  //=========================================================================
  // D_BORDER: Distance to map border (closer = better)
  //=========================================================================
  Real borderDist = distanceToMapBorder(pos);
  Real borderScore = 0.0f;
  if (borderDist < 150.0f) {
    borderScore = (150.0f - borderDist) * W_BORDER;
  }

  //=========================================================================
  // D_DEFENSE: Distance to nearest defense structure
  //=========================================================================
  Real defenseScore = 0.0f;
  Object *defense = findNearestDefenseStructure(pos);
  if (defense) {
    const Coord3D *defPos = defense->getPosition();
    if (defPos) {
      Real dx = pos.x - defPos->x;
      Real dy = pos.y - defPos->y;
      Real defenseDist = sqrtf(dx * dx + dy * dy);
      if (defenseDist < 250.0f) {
        defenseScore = (250.0f - defenseDist) * W_DEFENSE;
      }
    }
  }

  Real totalScore = oppositeScore + airScore + borderScore + defenseScore;
  return totalScore;
}

Coord3D AICoopPlayer::findSafeHackingPosition() {
  Coord3D safePos = m_baseCenter;

  // Find nearest barracks as reference base (for expansion support)
  Coord3D baseRef = m_baseCenter;
  Object *nearestBarracks = findNearestBarracks(m_baseCenter);
  if (nearestBarracks) {
    const Coord3D *barracksPos = nearestBarracks->getPosition();
    if (barracksPos) {
      baseRef = *barracksPos;
    }
  }

  // Get base radius (from AISkirmishPlayer)
  Real radius = m_baseRadius + 60.0f; // Slightly outside base

  // Generate 8 candidate positions around base reference (every 45 degrees)
  Coord3D candidates[8];
  Real bestScore = -99999.0f;
  int bestIdx = 0;

  for (int i = 0; i < 8; i++) {
    Real angle = i * (PI / 4.0f); // 0, 45, 90, 135, 180, 225, 270, 315 degrees
    candidates[i].x = baseRef.x + cosf(angle) * radius;
    candidates[i].y = baseRef.y + sinf(angle) * radius;
    candidates[i].z = baseRef.z;

    Real score = evaluateHackerPosition(candidates[i], baseRef);

    if (score > bestScore) {
      bestScore = score;
      bestIdx = i;
    }
  }

  safePos = candidates[bestIdx];

  // Log the decision
  if (TheGameLogic->getFrame() % 300 == 0) {
    DjLog("AICoopPlayer: Best hacker position at (%.0f, %.0f) score=%.1f "
          "baseRef=(%.0f,%.0f)",
          safePos.x, safePos.y, bestScore, baseRef.x, baseRef.y);
  }

  return safePos;
}

void AICoopPlayer::produceHackersIfNeeded() {
  // Only for China faction
  AsciiString sideName = m_player->getSide();
  if (!strstr(sideName.str(), "China")) {
    return;
  }

  // Log every 20 seconds (placeholder - full production logic TODO)
  if (TheGameLogic->getFrame() % 600 == 0) {
    DjLog("AICoopPlayer: Hacker production check (manual production only for "
          "now).");
  }
}

void AICoopPlayer::autoManageHackers() {
  // Only manage every 60 frames (2 seconds)
  if (TheGameLogic->getFrame() % 60 != 0) {
    return;
  }

  // Produce hackers from dedicated barracks (builds barracks if needed)
  produceHackersFromDedicatedBarracks();

  // Find Internet Center if it exists
  Object *internetCenter = findInternetCenter();

  // Find safe hacking position (fallback if no Internet Center)
  Coord3D safePos = findSafeHackingPosition();

  // Iterate all player's hackers using correct API
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

        // Must be a hacker
        if (!obj->isKindOf(KINDOF_MONEY_HACKER)) {
          continue;
        }

        AIUpdateInterface *ai = obj->getAIUpdateInterface();
        if (!ai)
          continue;

        // PRIORITY: Check for nearby threats - escape immediately!
        const Coord3D *hackerPos = obj->getPosition();
        if (hackerPos && isHackerInDanger(obj, 200.0f)) {
          // Find threat position for escape direction
          Object *threat =
              TheAI->findClosestEnemy(obj, 200.0f, AI::CAN_ATTACK, NULL);
          if (threat) {
            const Coord3D *threatPos = threat->getPosition();
            if (threatPos) {
              Coord3D escapePos = findEscapePosition(obj, *threatPos);
              ai->aiMoveToPosition(&escapePos, CMD_FROM_AI);
              DjLog("AICoopPlayer: Hacker %u ESCAPING from threat at "
                    "(%.0f,%.0f)!",
                    obj->getID(), threatPos->x, threatPos->y);
              continue; // Skip other actions - escape is priority
            }
          }
        }

        // Skip if not idle (already doing something)
        if (!ai->isIdle()) {
          continue;
        }

        // Skip if already in Internet Center
        if (obj->getContainedBy() != NULL) {
          continue;
        }

        // Check cooldown (prevent spam)
        ObjectID hackerID = obj->getID();
        UnsignedInt currentFrame = TheGameLogic->getFrame();
        std::map<ObjectID, UnsignedInt>::iterator cooldownIt =
            m_lastGuardCommandFrame.find(hackerID);
        if (cooldownIt != m_lastGuardCommandFrame.end()) {
          if (currentFrame - cooldownIt->second < 180) { // 6 second cooldown
            continue;
          }
        }

        // If Internet Center exists and has space, enter it
        if (internetCenter) {
          ContainModuleInterface *contain = internetCenter->getContain();
          if (contain) {
            int freeSlots =
                contain->getContainMax() - contain->getContainCount();
            if (freeSlots > 0) {
              ai->aiEnter(internetCenter, CMD_FROM_AI);
              m_lastGuardCommandFrame[hackerID] = currentFrame;
              DjLog("AICoopPlayer: Sending Hacker %u to Internet Center "
                    "(slots: %d)",
                    hackerID, freeSlots);
              continue;
            }
          }
        }

        // No Internet Center or full - go to safe position and hack
        if (hackerPos) {
          Real dx = hackerPos->x - safePos.x;
          Real dy = hackerPos->y - safePos.y;
          Real distSq = dx * dx + dy * dy;

          if (distSq > 22500.0f) { // More than 150 units away - move closer
            // Move to safe position first
            ai->aiMoveToPosition(&safePos, CMD_FROM_AI);
            m_lastGuardCommandFrame[hackerID] = currentFrame;
            DjLog(
                "AICoopPlayer: Moving Hacker %u to safe position (dist: %.1f)",
                hackerID, sqrtf(distSq));
          } else {
            // Close enough (within 100 units) - start hacking
            ai->aiHackInternet(CMD_FROM_AI);
            m_lastGuardCommandFrame[hackerID] = currentFrame;
            DjLog("AICoopPlayer: Hacker %u starting to hack at safe position",
                  hackerID);
          }
        }
      }
    }
  }
}

//=============================================================================
// Enhanced Hacker Placement v3 (2026-01-25)
// - Nearest Barracks as reference
// - Threat detection and escape
// - Higher air corridor priority
//=============================================================================

Object *AICoopPlayer::findNearestBarracks(const Coord3D &pos) {
  Object *nearest = NULL;
  Real nearestDistSq = 99999999.0f;

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

        if (!obj->isKindOf(KINDOF_STRUCTURE))
          continue;

        const ThingTemplate *tmpl = obj->getTemplate();
        if (!tmpl)
          continue;
        const char *name = tmpl->getName().str();

        // Find Barracks structures
        if (!strstr(name, "Barracks")) {
          continue;
        }

        const Coord3D *objPos = obj->getPosition();
        if (!objPos)
          continue;

        Real dx = pos.x - objPos->x;
        Real dy = pos.y - objPos->y;
        Real distSq = dx * dx + dy * dy;

        if (distSq < nearestDistSq) {
          nearestDistSq = distSq;
          nearest = obj;
        }
      }
    }
  }

  return nearest;
}

Bool AICoopPlayer::isHackerInDanger(Object *hacker, Real radius) {
  if (!hacker)
    return false;

  const Coord3D *hackerPos = hacker->getPosition();
  if (!hackerPos)
    return false;

  // Find closest enemy within radius
  Object *threat =
      TheAI->findClosestEnemy(hacker, radius, AI::CAN_ATTACK, NULL);

  if (threat) {
    DjLog("AICoopPlayer: Hacker %u in DANGER! Enemy nearby.", hacker->getID());
    return true;
  }

  return false;
}

Coord3D AICoopPlayer::findEscapePosition(Object *hacker,
                                         const Coord3D &threatPos) {
  Coord3D escapePos = m_baseCenter; // Default: run to base center

  const Coord3D *hackerPos = hacker->getPosition();
  if (!hackerPos)
    return escapePos;

  // Calculate direction away from threat
  Real dirX = hackerPos->x - threatPos.x;
  Real dirY = hackerPos->y - threatPos.y;
  Real len = sqrtf(dirX * dirX + dirY * dirY);

  if (len > 1.0f) {
    dirX /= len;
    dirY /= len;
  } else {
    // Threat at same position - run towards base center
    dirX = m_baseCenter.x - hackerPos->x;
    dirY = m_baseCenter.y - hackerPos->y;
    len = sqrtf(dirX * dirX + dirY * dirY);
    if (len > 1.0f) {
      dirX /= len;
      dirY /= len;
    }
  }

  // Run 150 units away from threat
  escapePos.x = hackerPos->x + dirX * 150.0f;
  escapePos.y = hackerPos->y + dirY * 150.0f;
  escapePos.z = hackerPos->z;

  return escapePos;
}

//=============================================================================
// Dedicated Hacker Barracks System (2026-01-26)
//=============================================================================

Object *AICoopPlayer::getHackerBarracks() {
  if (m_hackerBarracksID != INVALID_ID) {
    Object *barracks = TheGameLogic->findObjectByID(m_hackerBarracksID);
    if (barracks && !barracks->isEffectivelyDead()) {
      // Barracks tirik va mavjud
      return barracks;
    }
    // Barracks buzilgan yoki yo'q - ID ni tozalash
    m_hackerBarracksID = INVALID_ID;
    m_hackerBarracksBuilding = false;
  }
  return NULL;
}

Int AICoopPlayer::countHackers() {
  Int count = 0;

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

        if (obj->isKindOf(KINDOF_MONEY_HACKER)) {
          count++;
        }
      }
    }
  }

  return count;
}

Bool AICoopPlayer::buildHackerBarracks() {
  // Faqat China uchun
  AsciiString sideName = m_player->getSide();
  if (!strstr(sideName.str(), "China")) {
    return false;
  }

  // Allaqachon bor yoki qurilmoqda
  if (getHackerBarracks() != NULL || m_hackerBarracksBuilding) {
    return false;
  }

  // Xavfsiz joyni topish (hacker pozitsiyasi yaqinida)
  Coord3D buildPos = findSafeHackingPosition();
  // Biroz offset qo'shish (barracks hackerlardan uzoqroq bo'lsin)
  buildPos.x += 60.0f;
  buildPos.y += 30.0f;

  // Build list ga qo'shish
  m_player->addToPriorityBuildList(AsciiString("ChinaBarracks"), &buildPos,
                                   0.0f);
  m_hackerBarracksBuilding = true;
  m_hackerBarracksBuildPos = buildPos; // Pozitsiyani saqlash!

  DjLog("AICoopPlayer: Building dedicated Hacker Barracks at (%.0f, %.0f)",
        buildPos.x, buildPos.y);
  return true;
}

void AICoopPlayer::produceHackersFromDedicatedBarracks() {
  // Faqat China uchun
  AsciiString sideName = m_player->getSide();
  if (!strstr(sideName.str(), "China")) {
    return;
  }

  // Har 3 soniyada tekshirish
  UnsignedInt currentFrame = TheGameLogic->getFrame();
  if (currentFrame - m_lastHackerProductionFrame < 90) {
    return;
  }

  // Hacker barracks bormi?
  Object *hackerBarracks = getHackerBarracks();
  if (!hackerBarracks) {
    // Yo'q - qurishni boshlash (agar qurilmayotgan bo'lsa)
    if (!m_hackerBarracksBuilding) {
      buildHackerBarracks();
    }

    // Ikkinchi barracksni hacker barracks sifatida tanlash
    // (Birinchisi - asosiy infantry, ikkinchisi - hackerlar uchun)
    if (m_hackerBarracksBuilding) {
      std::vector<Object *> barrackslist;

      // Barcha barrackslarni yig'ish
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
            if (!obj->isKindOf(KINDOF_STRUCTURE))
              continue;

            const ThingTemplate *tmpl = obj->getTemplate();
            if (!tmpl)
              continue;

            // Barracks ekanini tekshirish
            if (strstr(tmpl->getName().str(), "Barracks")) {
              barrackslist.push_back(obj);
            }
          }
        }
      }

      // Agar 2 yoki undan ko'p barracks bo'lsa, ikkinchisini tanlash
      if (barrackslist.size() >= 2) {
        Object *hackerBarr = barrackslist[1]; // Ikkinchi barracks (index 1)
        m_hackerBarracksID = hackerBarr->getID();
        m_hackerBarracksBuilding = false;
        const Coord3D *pos = hackerBarr->getPosition();
        DjLog("AICoopPlayer: Found Hacker Barracks (2nd of %d) ID=%u at "
              "(%.0f,%.0f)",
              (int)barrackslist.size(), m_hackerBarracksID, pos ? pos->x : 0.0f,
              pos ? pos->y : 0.0f);
      } else {
        // Hali faqat bitta barracks bor - kutish
        DjLog("AICoopPlayer: Waiting for 2nd barracks (current count: %d)",
              (int)barrackslist.size());
      }
    }
    return;
  }

  // Production interface
  ProductionUpdateInterface *production =
      hackerBarracks->getProductionUpdateInterface();
  if (!production)
    return;

  // Navbatda nimadur bormi?
  if (production->getProductionCount() > 0) {
    return; // Allaqachon production qilmoqda
  }

  // Pul asosida production boshqarish:
  // - Pul >= 60000 bo'lsa TO'XTAT (yetarli pul bor)
  // - Pul < 20000 bo'lsa DAVOM ET (pul kerak)
  int currentMoney = m_player->getMoney()->countMoney();
  if (currentMoney >= 60000) {
    return; // Pul yetarli - hacker kerak emas
  }

  // Hacker uchun minimal pul (625 + buffer)
  if (currentMoney < 700) {
    return; // Hacker sotib olishga pul yetmaydi
  }

  // Hackerlar soni tekshirish (maksimal limit yo'q - pul bilan boshqariladi)

  // Hacker template topish
  const ThingTemplate *hackerTemplate =
      TheThingFactory->findTemplate(AsciiString("ChinaInfantryHacker"));
  if (!hackerTemplate) {
    DjLog("AICoopPlayer: ERROR - ChinaInfantryHacker template not found!");
    return;
  }

  // Production qilish
  ProductionID prodID = production->requestUniqueUnitID();
  if (production->queueCreateUnit(hackerTemplate, prodID)) {
    m_lastHackerProductionFrame = currentFrame;
    int hackerCount = countHackers();
    DjLog(
        "AICoopPlayer: Producing Hacker %d (money: %d) from dedicated barracks",
        hackerCount + 1, currentMoney);
  }
}

//=============================================================================
// Tech Building Capture System (IDEAL REWRITE - 2026-01-29)
// Building-centric: each building maps to exactly one soldier
// Proper tracking, cleanup, and edge case handling
//=============================================================================

// Helper struct for global assignment sorting
// Moved outside function to support legacy compiler template instantiation
struct CaptureCandidate {
  Object *soldier;
  Object *building;
  Real distSq;
};

// Comparator for std::sort (Global scope)
bool compareCandidates(const CaptureCandidate &a, const CaptureCandidate &b) {
  return a.distSq < b.distSq;
}

Real calculatePathLength(Path *path) {
  if (!path || !path->getFirstNode())
    return 99999999.0f;
  Real dist = 0.0f;
  PathNode *node = path->getFirstNode();
  while (node && node->getNext()) {
    PathNode *next = node->getNext();
    const Coord3D *p1 = node->getPosition();
    const Coord3D *p2 = next->getPosition();
    Real dx = p2->x - p1->x;
    Real dy = p2->y - p1->y;
    dist += sqrtf(dx * dx + dy * dy);
    node = next;
  }
  return dist;
}

void AICoopPlayer::autoCaptureTechBuildings() {
  UnsignedInt currentFrame = TheGameLogic->getFrame();

  // MONITOR & CLEANUP: Run frequently (every 15 frames = 0.5s)
  if (currentFrame % 15 == 0) {
    cleanupCaptureTracking();
    monitorCaptureProgress();
  }

  // ASSIGNMENT: Run rarely (every 90 frames = 3.0s)
  if (currentFrame - m_lastCaptureCheckFrame < 90) {
    return;
  }
  m_lastCaptureCheckFrame = currentFrame;

  // 1. Validation & Setup
  Player *neutralPlayer = ThePlayerList->getNeutralPlayer();
  if (!neutralPlayer)
    return;

  // 2. Collect Neutral Buildings
  // 2. Collect Neutral Buildings (Global Scan)
  // (2026-01-31) Switch to Global Object List to bypass any Team/Visibility
  // filtering which prevents Shrouded buildings from being found.
  std::vector<Object *> neutralBuildings;
  Object *obj = TheGameLogic->getFirstObject();
  while (obj) {
    if (obj->isKindOf(KINDOF_TECH_BUILDING) && !obj->isEffectivelyDead() &&
        obj->getControllingPlayer() &&
        obj->getControllingPlayer() == ThePlayerList->getNeutralPlayer()) {
      neutralBuildings.push_back(obj);

      // DIAGNOSTIC (2026-01-31): Log found buildings to verify Shroud
      // visibility if ((TheGameLogic->getFrame() % 150) == 0) {
      //   DjLog("AICoopPlayer: SCAN Building[%d] Found at (%.0f, %.0f)",
      //         obj->getID(), obj->getPosition()->x, obj->getPosition()->y);
      // }
    }
    obj = obj->getNextObject();
  }

  if (neutralBuildings.empty()) {
    // Clean up any stale assignments if no buildings left?
    // existing cleanupCaptureTracking handles dead/captured buildings.
    return;
  }

  // 3. Collect Potential Soldiers
  std::vector<Object *> potentialSoldiers;
  Player::PlayerTeamList::const_iterator pit;
  for (pit = m_player->getPlayerTeams()->begin();
       pit != m_player->getPlayerTeams()->end(); ++pit) {
    for (DLINK_ITERATOR<Team> tIter = (*pit)->iterate_TeamInstanceList();
         !tIter.done(); tIter.advance()) {
      Team *team = tIter.cur();
      if (!team)
        continue;
      for (DLINK_ITERATOR<Object> oIter = team->iterate_TeamMemberList();
           !oIter.done(); oIter.advance()) {
        Object *obj = oIter.cur();
        if (!obj || obj->isEffectivelyDead())
          continue;
        if (!obj->isKindOf(KINDOF_INFANTRY))
          continue;
        if (obj->isKindOf(KINDOF_NO_GARRISON))
          continue; // Can't capture

        // BLACKLIST CHECK (2026-01-31)
        if (m_soldierBlacklist.find(obj->getID()) != m_soldierBlacklist.end())
          continue;

        // STRICT IDLE CHECK (RELAXED 2026-01-31)
        // AIUpdateInterface *ai = obj->getAIUpdateInterface();
        // if (!ai || !ai->isIdle()) {
        //   continue;
        // }

        if (obj->isKindOf(KINDOF_MONEY_HACKER))
          continue;
        if (obj->isKindOf(KINDOF_HERO))
          continue;

        // CRITICAL FIX: Filter out soldiers inside vehicles/bunkers
        if (obj->isContained())
          continue;

        potentialSoldiers.push_back(obj);
      }
    }
  }

  // Also include soldiers currently in m_buildingToSoldier (if they are valid
  // and not contained) Actually, they should be found in the loop above IF
  // they are valid. The loop above iterates ALL team members. If
  // m_buildingToSoldier has someone, they are still on a team. So
  // potentialSoldiers contains EVERYONE we can use.

  if (neutralBuildings.empty()) {
    return;
  }

  // LOG SUMMARY (2026-01-31): Diagnostic counts
  if ((TheGameLogic->getFrame() % 150) == 0) {
    DjLog("AICoopPlayer: CAPTURE SUMMARY - Found %d Buildings. Available "
          "Soldiers: %d. Assigned Active: %d",
          neutralBuildings.size(), potentialSoldiers.size(),
          m_buildingToSoldier.size());
  }

  if (potentialSoldiers.empty())
    return;

  // 4. Generate Candidate List
  std::vector<CaptureCandidate> candidates;
  candidates.reserve(neutralBuildings.size() * potentialSoldiers.size());

  for (size_t i = 0; i < neutralBuildings.size(); ++i) {
    Object *b = neutralBuildings[i];
    const Coord3D *bPos = b->getPosition();
    if (!bPos)
      continue;

    for (size_t j = 0; j < potentialSoldiers.size(); ++j) {
      Object *s = potentialSoldiers[j];
      const Coord3D *sPos = s->getPosition();
      if (!sPos)
        continue;

      Real dx = bPos->x - sPos->x;
      Real dy = bPos->y - sPos->y;
      Real distSq = dx * dx + dy * dy;

      // PATHFINDING UPDATE (2026-01-31)
      // If within "reasonable" range (e.g. 1500 units), calculate TRUE walking
      // distance using pathfinder. This solves the "cliff/wall" problem.
      if (distSq < 2250000.0f) { // 1500 * 1500
        Path *path = TheAI->pathfinder()->findGroundPath(sPos, bPos, 0, false);
        if (path) {
          Real pathLen = calculatePathLength(path);
          distSq = pathLen * pathLen;
          deleteInstance(path); // Release path memory using MemoryPool
        } else {
          // FALLBACK (2026-01-31): If path fails (e.g. Shroud), use Air Dist
          // with penalty. We multiply distSq by 4.0 (effectively doubling the
          // distance). If truly unreachable, the Blacklist system will catch
          // the stalling soldier.
          distSq *= 4.0f;
        }
      }

      CaptureCandidate cand;
      cand.soldier = s;
      cand.building = b;
      cand.distSq = distSq;
      candidates.push_back(cand);
    }
  }

  // 5. Sort Candidates (Closest First)
  std::sort(candidates.begin(), candidates.end(), compareCandidates);

  // 6. Greedy Assignment
  std::set<ObjectID> usedSoldiers;
  std::set<ObjectID> usedBuildings;
  std::map<ObjectID, ObjectID> newAssignments; // BuildingID -> SoldierID

  // STABILITY FIX (2026-01-31):
  // Preserve existing assignments to prevent "stopping/swapping" mid-path.
  // If a soldier is already assigned and valid, LOCK HIM IN.
  std::map<ObjectID, ObjectID>::iterator existIt = m_buildingToSoldier.begin();
  while (existIt != m_buildingToSoldier.end()) {
    ObjectID bID = existIt->first;
    ObjectID sID = existIt->second;
    Object *b = TheGameLogic->findObjectByID(bID);
    Object *s = TheGameLogic->findObjectByID(sID);

    // If valid paring, keep it!
    if (b && s && !s->isEffectivelyDead() && !b->isEffectivelyDead()) {
      newAssignments[bID] = sID;
      usedBuildings.insert(bID);
      usedSoldiers.insert(sID);
    }
    ++existIt;
  }

  std::vector<CaptureCandidate>::iterator cit;
  for (cit = candidates.begin(); cit != candidates.end(); ++cit) {
    Object *s = cit->soldier;
    Object *b = cit->building;
    ObjectID sID = s->getID();
    ObjectID bID = b->getID();

    if (usedSoldiers.find(sID) != usedSoldiers.end())
      continue; // Soldier taken
    if (usedBuildings.find(bID) != usedBuildings.end())
      continue; // Building taken

    // Assign!
    newAssignments[bID] = sID;
    usedSoldiers.insert(sID);
    usedBuildings.insert(bID);
  }

  // 7. Execution & Diffing
  // A. Detect "Lost Jobs" (Soldiers who were assigned but are not in new
  // plan) They should be STOPPED.
  std::map<ObjectID, ObjectID>::iterator oldIt = m_buildingToSoldier.begin();
  while (oldIt != m_buildingToSoldier.end()) {
    ObjectID oldBID = oldIt->first;
    ObjectID oldSID = oldIt->second;

    bool keep = false;
    if (newAssignments.find(oldBID) != newAssignments.end()) {
      if (newAssignments[oldBID] == oldSID) {
        keep = true; // Still the best man for the job
      }
    }

    if (!keep) {
      // He was fired or replaced. Stop him.
      Object *firedSoldier = TheGameLogic->findObjectByID(oldSID);
      if (firedSoldier && !firedSoldier->isEffectivelyDead()) {
        AIUpdateInterface *ai = firedSoldier->getAIUpdateInterface();
        if (ai) {
          ai->setAttitude(ATTITUDE_NORMAL);
          ai->aiIdle(CMD_FROM_AI); // STOP COMMAND
          DjLog("AICoopPlayer: STOP Soldier[%d] (Replaced/Unneeded)", oldSID);
        }
      }
      // Log swap if replaced
      if (newAssignments.find(oldBID) != newAssignments.end()) {
        DjLog("AICoopPlayer: SWAP for Building[%d]: Soldier[%d] -> Soldier[%d]",
              oldBID, oldSID, newAssignments[oldBID]);
      }

      oldIt = m_buildingToSoldier.erase(oldIt);
    } else {
      ++oldIt;
    }
  }

  // B. Execute New Assignments (including Refreshes)
  std::map<ObjectID, ObjectID>::iterator newIt;
  for (newIt = newAssignments.begin(); newIt != newAssignments.end(); ++newIt) {
    ObjectID bID = newIt->first;
    ObjectID sID = newIt->second;

    m_buildingToSoldier[bID] = sID; // Update map

    Object *s = TheGameLogic->findObjectByID(sID);
    Object *b = TheGameLogic->findObjectByID(bID);

    if (s && b) {
      // EJECT FROM TEAM (Script Protection)
      if (s->getTeam() != m_player->getDefaultTeam()) {
        s->setTeam(m_player->getDefaultTeam());
      }

      AIUpdateInterface *ai = s->getAIUpdateInterface();
      if (ai) {
        // Determine if we need to issue command.
        // Since this runs every 15 frames, issuing it constantly is OK-ish,
        // but we can check if he is already doing it?
        // Actually, "Refresh" is desired to fight against other queued
        // orders.
        ai->setAttitude(ATTITUDE_PASSIVE);

        // ============================================================
        // INITIAL CAPTURE ASSIGNMENT (tryToCaptureTechBuildings)
        // ============================================================
        DjLog(">>> [INITIAL ASSIGN] Soldier[%d] -> Building[%d] START <<<",
              s->getID(), b->getID());

        Bool captureSent = FALSE;
        if (TheControlBar && s->getTemplate()) {
          AsciiString csName = s->getTemplate()->friend_getCommandSetString();
          DjLog("    [STEP 1/4] Getting CommandSet: '%s'", csName.str());

          const CommandSet *cs = TheControlBar->findCommandSet(csName);

          if (!cs) {
            DjLog("    [STEP 1/4] ❌ FAIL: CommandSet '%s' NOT FOUND!",
                  csName.str());
          } else {
            DjLog("    [STEP 1/4] ✅ SUCCESS: CommandSet found");

            // Dump all available buttons
            DjLog("    [STEP 2/4] Scanning CommandButtons in '%s':",
                  csName.str());
            int buttonCount = 0;
            for (int i = 0; i < MAX_COMMANDS_PER_SET; ++i) {
              const CommandButton *cb = cs->getCommandButton(i);
              if (cb) {
                DjLog("              [%d] '%s'", i, cb->getName().str());
                buttonCount++;
              }
            }
            DjLog("              Total buttons found: %d", buttonCount);

            // Search for capture button (using substring for faction-specific
            // names)
            DjLog("    [STEP 3/4] Searching for button containing "
                  "'CaptureBuilding'...");
            for (int i = 0; i < MAX_COMMANDS_PER_SET; ++i) {
              const CommandButton *cb = cs->getCommandButton(i);
              // CRITICAL FIX: Use strstr for substring match (faction-specific
              // names) Examples: Command_ChinaInfantryRedGuardCaptureBuilding,
              // Command_USAInfantryRangerCaptureBuilding
              if (cb &&
                  strstr(cb->getName().str(), "CaptureBuilding") != NULL) {
                DjLog("    [STEP 3/4] ✅ FOUND at index [%d]!", i);
                DjLog("    [STEP 4/4] Executing capture command via "
                      "doCommandButtonAtObject...");

                // CRITICAL FIX: Use Object::doCommandButtonAtObject instead of
                // aiDoCommand(AICMD_COMMANDBUTTON_OBJ).
                // The AI dispatch only handles COMBATDROP, but
                // Object::doCommandButtonAtObject properly handles
                // GUI_COMMAND_SPECIAL_POWER which is what CaptureBuilding uses.
                // CRITICAL FIX: Use CMD_FROM_SCRIPT to force execution!
                // CMD_FROM_AI causes canUseSpecialPower check which silently
                // fails.
                s->doCommandButtonAtObject(cb, b, CMD_FROM_SCRIPT);
                captureSent = TRUE;

                DjLog("    [STEP 4/4] ✅ SUCCESS: Capture command sent "
                      "(SpecialPower)!");
                DjLog(">>> [INITIAL ASSIGN] Soldier[%d] -> Building[%d] ✅ "
                      "COMPLETE <<<\n",
                      s->getID(), b->getID());
                break;
              }
            }

            if (!captureSent) {
              DjLog("    [STEP 3/4] ❌ FAIL: 'Command_CaptureBuilding' NOT "
                    "FOUND in buttons!");
            }
          }
        } else {
          DjLog("    [STEP 1/4] ❌ FAIL: TheControlBar or Template is NULL");
        }

        if (!captureSent) {
          DjLog("    [FALLBACK] Using aiEnter() instead...");
          ai->aiEnter(b, CMD_FROM_AI);
          DjLog(">>> [INITIAL ASSIGN] Soldier[%d] -> Building[%d] ⚠️ FALLBACK "
                "USED <<<\n",
                s->getID(), b->getID());
        }
      }
    }
  }

  if (!newAssignments.empty()) {
    // Optional concise summary
    // DjLog("AICoopPlayer: Global Assign: %d pairings",
    // newAssignments.size());
  }
}

void AICoopPlayer::cleanupCaptureTracking() {
  // Remove entries where: soldier died, building died, building captured, or
  // soldier became idle
  Int removedCount = 0;

  std::map<ObjectID, ObjectID>::iterator it = m_buildingToSoldier.begin();
  while (it != m_buildingToSoldier.end()) {
    ObjectID buildingID = it->first;
    ObjectID soldierID = it->second;

    Object *building = TheGameLogic->findObjectByID(buildingID);
    Object *soldier = TheGameLogic->findObjectByID(soldierID);

    Bool remove = false;

    // Case 1: Soldier is dead
    if (!soldier || soldier->isEffectivelyDead()) {
      remove = true;
    }
    // Case 2: Building is dead
    else if (!building || building->isEffectivelyDead()) {
      remove = true;
    }
    // Case 3: Building was captured (no longer neutral)
    else if (building->getControllingPlayer() !=
             ThePlayerList->getNeutralPlayer()) {
      remove = true;
    }
    // Case 4: Soldier is contained in something (e.g. Transport) but NOT the
    // building
    else if (soldier->isContained() && soldier->getContainedBy() != building) {
      remove = true;
      DjLog(
          "AICoopPlayer: FAILURE Soldier[%d] CONTAINED in Object[%d] (not "
          "target)",
          soldierID,
          (soldier->getContainedBy() ? soldier->getContainedBy()->getID() : 0));
    }
    // NOTE: Removed idle check - isIdle() returns true while walking!

    if (remove) {
      if (!soldier || soldier->isEffectivelyDead()) {
        DjLog("AICoopPlayer: FAILURE Soldier[%d] DIED en route to Building[%d]",
              soldierID, buildingID);
      } else if (building && building->getControllingPlayer() !=
                                 ThePlayerList->getNeutralPlayer()) {
        DjLog("AICoopPlayer: SUCCESS! Soldier[%d] CAPTURED Building[%d]",
              soldierID, buildingID);

        // SUCCESS: Reset attitude to AGGRESSIVE to defend the new building
        if (soldier && !soldier->isEffectivelyDead()) {
          AIUpdateInterface *ai = soldier->getAIUpdateInterface();
          if (ai)
            ai->setAttitude(ATTITUDE_AGGRESSIVE);
        }
      }

      it = m_buildingToSoldier.erase(it);
      removedCount++;
    } else {
      // STATUS UPDATE: Log if soldier is close or starting capture
      if (soldier && building) {
        const Coord3D *spos = soldier->getPosition();
        const Coord3D *bpos = building->getPosition();
        if (spos && bpos) {
          Real dx = bpos->x - spos->x;
          Real dy = bpos->y - spos->y;
          Real dist = sqrtf(dx * dx + dy * dy);

          // LOG: Started Capturing (Inside or very close)
          if (soldier->isContained() || dist < 15.0f) {
            // Only log this once ideally, but here we log it periodically while
            // inside. To avoid spam, using mod check on frame
            if ((TheGameLogic->getFrame() % 90) == 0) {
              DjLog("AICoopPlayer: ARRIVED/CAPTURING Soldier[%d] at "
                    "Building[%d] (Contained: %d, Dist: %.1f)",
                    soldierID, buildingID, soldier->isContained(), dist);
            }
          }
          // LOG: En Route (Moving)
          else if ((TheGameLogic->getFrame() % 150) == 0) { // Every 5 seconds
            DjLog("AICoopPlayer: STATUS Soldier[%d] at (%.0f, %.0f) -> "
                  "Building[%d] at (%.0f, %.0f) (Dist: %.1f)",
                  soldierID, spos->x, spos->y, buildingID, bpos->x, bpos->y,
                  dist);

            // STALLED CHECK: If soldier is IDLE but far away, re-issue command
            AIUpdateInterface *ai = soldier->getAIUpdateInterface();
            if (ai && ai->isIdle() && dist > 20.0f) {

              // BLACKLIST LOGIC (2026-01-31)
              m_failedCaptureAttempts[soldierID]++;
              if (m_failedCaptureAttempts[soldierID] > 5) {
                DjLog(
                    "AICoopPlayer: BLACKLISTING Soldier[%d] (Stalled 5+ times)",
                    soldierID);
                m_soldierBlacklist.insert(soldierID);
                // ai->stop(); // Not available in AIUpdateInterface
                if (ai)
                  ai->setAttitude(ATTITUDE_PASSIVE);

                // Release assignment
                // We are inside iteration, but 'it' has already advanced in the
                // calling loop (monitorCaptureProgress) Wait, this is
                // 'autoCaptureTechBuildings' loop! 'it' is iterating
                // m_buildingToSoldier.
                it = m_buildingToSoldier.erase(it);
                removedCount++;
                continue;
              }

              DjLog("AICoopPlayer: WARNING Soldier[%d] STALLED at dist %.1f! "
                    "(SpecialAbilityUpdate should handle this - skipping "
                    "aiEnter)",
                    soldierID, dist);

              // Eject from any scripted team to prevent script interference
              if (soldier->getTeam() != m_player->getDefaultTeam()) {
                soldier->setTeam(m_player->getDefaultTeam());
              }

              ai->setAttitude(ATTITUDE_PASSIVE); // Re-enforce passive
              // CRITICAL FIX (2026-02-01): DO NOT call aiEnter!
              // Tech Buildings don't have CONTAIN module - aiEnter gets
              // rejected! SpecialAbilityUpdate handles capture via
              // approachTarget() - just wait. ai->aiEnter(building,
              // CMD_FROM_AI);
            }
          }
        }
      }
      ++it;
    }
  }

  if (removedCount > 0) {
    DjLog("AICoopPlayer: Cleanup removed %d stale capture entries",
          removedCount);
  }
}

// Watchdog: Verify soldier state and self-heal lost commands
void AICoopPlayer::monitorCaptureProgress() {
  std::map<ObjectID, ObjectID>::iterator it = m_buildingToSoldier.begin();
  while (it != m_buildingToSoldier.end()) {
    ObjectID bID = it->first;
    ObjectID sID = it->second;
    it++; // Safe advance

    Object *soldier = TheGameLogic->findObjectByID(sID);
    Object *building = TheGameLogic->findObjectByID(bID);

    if (!soldier || !building)
      continue;

    AIUpdateInterface *ai = soldier->getAIUpdateInterface();
    if (!ai)
      continue;

    // 1. Goal Verification (Self-Healing)
    Object *currentGoal = ai->getGoalObject();

    // CRITICAL FIX (2026-02-01): Skip recovery if soldier is ACTIVELY
    // capturing! During SpecialAbility execution (capture in progress), the
    // engine sets OBJECT_STATUS_IS_USING_ABILITY. Don't re-issue commands
    // during this phase!
    Bool isUsingAbility = soldier->testStatus(OBJECT_STATUS_IS_USING_ABILITY);

    // DIAGNOSTIC: Log status to understand why check is not working
    if ((TheGameLogic->getFrame() % 30) == 0) {
      DjLog("AICoopPlayer: DIAG Soldier[%d] IS_USING_ABILITY=%d Goal=%s", sID,
            isUsingAbility ? 1 : 0,
            currentGoal ? (currentGoal == building ? "BUILDING" : "OTHER")
                        : "NULL");
    }

    if (isUsingAbility) {
      // Soldier is actively capturing - do NOT interrupt!
      continue;
    }

    if (currentGoal != building) {

      // SHROUD FIX (2026-01-31):
      // If the building is Shrouded/Fogged, 'aiEnter' fails (Goal becomes
      // NULL). We must 'Move' (Scout) there first. CRITICAL: Check this BEFORE
      // Blacklist logic so we don't penalize scouting!
      ObjectShroudStatus shroud =
          building->getShroudedStatus(m_player->getPlayerIndex());
      if (shroud == OBJECTSHROUD_SHROUDED || shroud == OBJECTSHROUD_FOGGED) {
        if ((TheGameLogic->getFrame() % 60) == 0) {
          DjLog("AICoopPlayer: Soldier[%d] Target[%d] is SHROUDED (letting "
                "SpecialAbilityUpdate handle)",
                sID, bID);
        }
        // CRITICAL FIX (2026-02-02): DO NOT call aiMoveToPosition!
        // ============================================================
        // ROOT CAUSE OF ENDLESS RESTART:
        // After INITIAL ASSIGN issues capture command, SpecialAbilityUpdate
        // starts handling the soldier via approachTarget().
        // Calling aiMoveToPosition here OVERRIDES the SpecialAbilityUpdate's
        // movement and resets its progress!
        //
        // SpecialAbilityUpdate::approachTarget() already calls:
        //   ai->aiMoveToObject(target, CMD_FROM_AI);
        // We should NOT interfere with additional move commands.
        // ============================================================
        // OLD CODE THAT RESET PROGRESS - DO NOT ENABLE:
        // ai->aiMoveToPosition(building->getPosition(), CMD_FROM_AI);
        continue; // SKIP BLACKLIST LOGIC & SKIP FAIL COUNT
      }

      // ============================================================
      // CRITICAL FIX (2026-02-01): THROTTLE MONITOR RECOVERY
      // ============================================================
      // ROOT CAUSE: Every doCommandButtonAtObject call triggers
      // initiateIntentToDoSpecialPower which:
      //   1. Calls aiIdle() - CLEARS soldier's goal
      //   2. Resets m_targetID, m_packingState, etc.
      // This COMPLETELY RESETS SpecialAbilityUpdate progress!
      //
      // If we call recovery every frame, the soldier never gets to
      // approachTarget() because his progress resets each time.
      //
      // SOLUTION: Only allow recovery every 300 frames (5 seconds).
      // This gives SpecialAbilityUpdate time to work.
      // ============================================================
      if ((TheGameLogic->getFrame() % 300) != 0) {
        // Not time for recovery check yet - let SpecialAbilityUpdate work
        continue;
      }

      // AGGRESSIVE FIX: Ensure he belongs to us
      if (soldier->getTeam() != m_player->getDefaultTeam()) {
        soldier->setTeam(m_player->getDefaultTeam());
      }
      ai->setAttitude(ATTITUDE_PASSIVE);

      // ============================================================
      // MONITOR & RECOVERY (monitorCaptureProgress)
      // ============================================================
      DjLog(">>> [MONITOR] Soldier[%d] -> Building[%d] RECOVERY START <<<", sID,
            bID);

      // 1. Try CommandButton 'CaptureBuilding' FIRST
      Bool captureSent = FALSE;
      bool hasCaptureAbility = false;

      // Access CommandSet via ControlBar (Global)
      if (TheControlBar && soldier->getTemplate()) {
        AsciiString csName =
            soldier->getTemplate()->friend_getCommandSetString();
        DjLog("    [MONITOR-1] CommandSet: '%s'", csName.str());

        const CommandSet *cs = TheControlBar->findCommandSet(csName);

        if (cs) {
          DjLog("    [MONITOR-2] Searching for button containing "
                "'CaptureBuilding'...");
          for (int i = 0; i < MAX_COMMANDS_PER_SET; ++i) {
            const CommandButton *cb = cs->getCommandButton(i);
            // CRITICAL FIX: Use strstr for substring match
            if (cb && strstr(cb->getName().str(), "CaptureBuilding") != NULL) {
              // CRITICAL FIX (2026-02-01): DO NOT RE-ISSUE CAPTURE COMMAND!
              // ============================================================
              // ROOT CAUSE OF ENDLESS RESTART CYCLE:
              // Every doCommandButtonAtObject call triggers:
              //   initiateIntentToDoSpecialPower (line 518 in
              //   SpecialAbilityUpdate)
              //     → aiIdle(CMD_FROM_AI)  ← CLEARS GOAL!
              //     → m_targetID = RESET
              //     → m_packingState = RESET
              // This COMPLETELY RESETS SpecialAbilityUpdate progress!
              //
              // INITIAL ASSIGN already issued the command.
              // SpecialAbilityUpdate handles the rest via approachTarget().
              // We should NOT interfere by re-issuing commands!
              // ============================================================
              DjLog("    [MONITOR-2] ✅ FOUND CaptureBuilding button (NOT "
                    "re-issuing to avoid reset)");

              // OLD CODE THAT CAUSED RESET - DO NOT ENABLE:
              // soldier->doCommandButtonAtObject(cb, building,
              // CMD_FROM_SCRIPT);

              // captureSent = TRUE;  // Don't mark as sent since we're not
              // sending
              hasCaptureAbility = true;
              DjLog("    [MONITOR-3] ℹ️ Soldier has capture ability - letting "
                    "SpecialAbilityUpdate work");
              break;
            }
          }

          if (!captureSent) {
            DjLog(
                "    [MONITOR-2] ❌ FAIL: 'Command_CaptureBuilding' NOT FOUND");
          }
        } else {
          DjLog("    [MONITOR-1] ❌ FAIL: CommandSet '%s' not found",
                csName.str());
        }
      } else {
        DjLog("    [MONITOR-1] ❌ FAIL: TheControlBar or Template is NULL");
      }

      if (captureSent) {
        // SUCCESS: We issued a real capture command.
        // Reset failure count so he doesn't get blacklisted while capturing.
        m_failedCaptureAttempts[sID] = 0;
        DjLog(">>> [MONITOR] Soldier[%d] -> Building[%d] ✅ RECOVERED <<<\n",
              sID, bID);
        continue; // Skip the rest (Blacklist/Fallback)
      }

      // 2. BLACKLIST LOGIC (Only if CommandButton failed/not found)
      m_failedCaptureAttempts[sID]++;
      DjLog("    [MONITOR-4] ⚠️ Incrementing failure count: %d/5",
            m_failedCaptureAttempts[sID]);

      if (m_failedCaptureAttempts[sID] > 5) {
        DjLog("    [MONITOR-4] 🚫 BLACKLISTING Soldier[%d] (Failed 5+ times)",
              sID);
        m_soldierBlacklist.insert(sID);
        if (ai)
          ai->setAttitude(ATTITUDE_PASSIVE);
        m_buildingToSoldier.erase(bID);
        DjLog(">>> [MONITOR] Soldier[%d] -> Building[%d] 🚫 BLACKLISTED <<<\n",
              sID, bID);
        continue;
      }

      // 3. Fallback to aiEnter (for non-capture units or Bunkers)
      if (!hasCaptureAbility) {
        DjLog("    [FALLBACK] Soldier[%d] has NO CAPTURE BUTTON. Trying "
              "aiEnter...",
              sID);
      }
      // CRITICAL FIX (2026-02-01): DO NOT call aiEnter!
      // Tech Buildings don't have CONTAIN module - aiEnter gets rejected!
      // This causes soldier to reposition and restart capture in endless loop.
      // SpecialAbilityUpdate handles capture - just wait.
      // ai->aiEnter(building, CMD_FROM_AI);
      continue;
    }

    // 2. Stalled Idle Check
    const Coord3D *spos = soldier->getPosition();
    const Coord3D *bpos = building->getPosition();
    Real dist = 0.0f;
    if (spos && bpos) {
      Real dx = bpos->x - spos->x;
      Real dy = bpos->y - spos->y;
      dist = sqrtf(dx * dx + dy * dy);
    }

    if (dist > 15.0f && ai->isIdle()) {
      DjLog("AICoopPlayer: TRACKER ERROR Soldier[%d] STALLED (Idle at %.1f). "
            "(SpecialAbilityUpdate should handle - skipping aiEnter)",
            sID, dist);
      ai->setAttitude(ATTITUDE_PASSIVE);
      // CRITICAL FIX (2026-02-01): DO NOT call aiEnter!
      // Tech Buildings don't have CONTAIN module - aiEnter gets rejected!
      // ai->aiEnter(building, CMD_FROM_AI);
      continue;
    }

    // 3. Status Log (Every 2 seconds)
    if ((TheGameLogic->getFrame() % 60) == 0 && dist > 15.0f) {
      DjLog("AICoopPlayer: TRACKER Soldier[%d] EN ROUTE to [%d] (Dist: %.1f)",
            sID, bID, dist);
    }
  }
}
