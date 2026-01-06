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

  // Helper functions for build management
  Object *findStructure(const char *namePattern);
  int countStructures(const char *namePattern);
  int countStructuresUnderConstruction(const char *namePattern);

  // Build priority management
  void initializeBuildPriorities();
  void updateBuildPriorities();

  // Track if we have loaded the Skirmish AI scripts/teams for the human player
  Bool m_skirmishScriptsLoaded;
};
