/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
////////////////////////////////////////////////////////////////////////////////
//
//  (c) 2001-2003 Electronic Arts Inc.
//
////////////////////////////////////////////////////////////////////////////////

// AICoopPlayer.h
// Computerized opponent that cooperates with a human player.
// Author: [Your Name/Antigravity], December 2025

#pragma once

#include "Common/Player.h"
#include "GameLogic/AIPlayer.h"
#include "GameLogic/AISkirmishPlayer.h"
#include <map>
#include <set>

// Guard command cooldown: 90 frames = ~3 seconds at 30 FPS
static const UnsignedInt GUARD_COMMAND_COOLDOWN_FRAMES = 90;

class AICoopPlayer : public AISkirmishPlayer {
  // MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(AICoopPlayer, "AICoopPlayer")

public:
  // Override memory pool with standard heap allocation to avoid init issues
  void *operator new(size_t s) { return ::operator new(s); }
  void operator delete(void *p) { ::operator delete(p); }

  AICoopPlayer(Player *p);
  virtual ~AICoopPlayer();

  virtual void update();

protected:
  // New methods for coop behavior
  void attemptCoopBehavior();
  Player *findHumanAlly();

  // Override enemy acquisition to prioritize enemies attacking the ally
  virtual void acquireEnemy();

  // AI Assist Mode for Human Players
  void assistHumanPlayer();
  void autoManageIdleDozers();
  void autoManageIdleUnits();
  void autoDefendBase();
  void autoManageCombatTeams();
  void autoManageOverlordUpgrades();

  // Helper functions for build management
  Object *findStructure(const char *namePattern);
  int countStructures(const char *namePattern);
  int countStructuresUnderConstruction(const char *namePattern);

  // Build priority management
  void initializeBuildPriorities();
  void updateBuildPriorities();

  // Reactive Defense
  enum ThreatType {
    THREAT_NONE,
    THREAT_INFANTRY,
    THREAT_VEHICLE,
    THREAT_AIR,
    THREAT_STRUCTURE
  };
  ThreatType assessGlobalThreat();

  // Aircraft Waypoint Management (NEW - for helicopter/aircraft patrol)
  void initializeAirPatrol();
  void manageAirUnits();
  void checkAircraftAmmo();
  Object *findNearestAirfield(Object *aircraft);

  // Hacker Management System (NEW - 2026-01-25)
  void autoManageHackers();
  Coord3D findSafeHackingPosition();
  void produceHackersIfNeeded();
  Object *findInternetCenter();

  // Weighted Scoring Hacker Placement (v2 - 2026-01-25)
  Real evaluateHackerPosition(const Coord3D &pos, const Coord3D &baseRef);
  Real distanceToMapBorder(const Coord3D &pos);
  Object *findNearestDefenseStructure(const Coord3D &pos);
  void getEnemyBaseCenter(Coord3D *outPos);

  // Enhanced Hacker Placement v3 (2026-01-25)
  Object *findNearestBarracks(const Coord3D &pos);
  Bool isHackerInDanger(Object *hacker, Real radius);
  Coord3D findEscapePosition(Object *hacker, const Coord3D &threatPos);

  // Dedicated Hacker Barracks System (2026-01-26)
  ObjectID m_hackerBarracksID;             // Hacker uchun maxsus barracks ID
  Bool m_hackerBarracksBuilding;           // Qurilmoqda flag
  UnsignedInt m_lastHackerProductionFrame; // Oxirgi hacker production vaqti
  Coord3D
      m_hackerBarracksBuildPos; // Qurish buyurilgan pozitsiya (detection uchun)

  Object *getHackerBarracks();                // Hacker barracks ni olish
  Bool buildHackerBarracks();                 // Hacker barracks qurish
  void produceHackersFromDedicatedBarracks(); // Hackerlarni chiqarish
  Int countHackers();                         // Mavjud hackerlar soni

  // Track if we have loaded the Skirmish AI scripts/teams for the human player
  Bool m_skirmishScriptsLoaded;

  // Waypoint system for air patrol (NEW)
  class Waypoint *m_airPatrolPath; // Pointer to first waypoint in patrol chain
  Bool m_airPatrolInitialized;

  // Per-unit guard command cooldown tracking (2026-01-25)
  // Maps ObjectID -> last frame when guard command was issued
  std::map<ObjectID, UnsignedInt> m_lastGuardCommandFrame;

  // Helper to clean up dead units from cooldown map
  void cleanupGuardCooldownMap();

  // Tech Building Capture System (2026-01-29 REWRITE)
  // Building-centric: each building maps to one soldier
  void autoCaptureTechBuildings();
  void cleanupCaptureTracking();

  // Building ID -> Soldier ID (which soldier is assigned to which building)
  std::map<ObjectID, ObjectID> m_buildingToSoldier;
  void monitorCaptureProgress();
  UnsignedInt m_lastCaptureCheckFrame;

  // Blacklist System (2026-01-31)
  std::map<ObjectID, Int> m_failedCaptureAttempts;
  std::set<ObjectID> m_soldierBlacklist;
};
