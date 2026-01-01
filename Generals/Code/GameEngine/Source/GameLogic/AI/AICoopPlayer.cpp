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
#include "GameLogic/AIPlayer.h" // Explicitly include base
#include "GameLogic/GameLogic.h"
#include "GameLogic/ScriptEngine.h"
#include "PreRTS.h"

#include "Common/DjDebug.h"

AICoopPlayer::AICoopPlayer(Player *p) : AISkirmishPlayer(p) {
  // Log creation for verification
  AsciiString msg = "AICoopPlayer created for ";
  msg.concat(p->getSide());
  msg.concat(p->getSide());
  TheScriptEngine->AppendDebugMessage(msg, false);

  DjLog("AICoopPlayer created for player side: %ls",
        p->getPlayerDisplayName().str());
}

AICoopPlayer::~AICoopPlayer() {}

void AICoopPlayer::update() {
  DjLog("AICoopPlayer::update - Frame %d", TheGameLogic->getFrame());
  // Run standard Skirmish AI logic which handles base building, unit
  // production, etc.
  AISkirmishPlayer::update();

  // Add coop-specific behavior
  attemptCoopBehavior();
}

Player *AICoopPlayer::findHumanAlly() {
  for (Int i = 0; i < ThePlayerList->getPlayerCount(); i++) {
    Player *other = ThePlayerList->getNthPlayer(i);
    // Explicitly use 'this' to help compiler resolve protected member
    if (other != m_player && other->getPlayerType() == PLAYER_HUMAN) {
      // Check if we are allies
      if (m_player->getRelationship(other->getDefaultTeam()) == ALLIES) {
        DjLog("AICoopPlayer::findHumanAlly - Found ally: %ls",
              other->getPlayerDisplayName().str());
        return other;
      }
    }
  }
  return NULL;
}

void AICoopPlayer::attemptCoopBehavior() {
  Player *humanAlly = findHumanAlly();
  if (!humanAlly) {
    // DjLog("AICoopPlayer::attemptCoopBehavior - No human ally found.");
    return;
  }

  DjLog("AICoopPlayer::attemptCoopBehavior - Cooperating with %ls",
        humanAlly->getPlayerDisplayName().str());

  // Simple example: Defend ally if they are under attack
  // Note: In a real implementation this would check frames, distances, etc.
  // m_attackedFrame and m_attackedBy are available in Player class.

  // This serves as the hook to add complex logic.
}

void AICoopPlayer::acquireEnemy() {
  // Enhancing enemy acquisition to care about Ally's enemies
  Player *humanAlly = findHumanAlly();
  if (humanAlly) {
    // Logic to prefer enemies attacking the human could go here
    // For now, we fallback to standard logic but with awareness that we HAVE an
    // ally
  }

  AISkirmishPlayer::acquireEnemy();
}
