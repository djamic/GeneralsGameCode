
// [NEW] Helper to get the correct Skirmish AI build list for this faction
BuildListInfo *AICoopPlayer::getAISideBuildList() {
  AsciiString sideName = m_player->getSide();
  AsciiString fullAiName = "Skirmish";
  fullAiName.concat(sideName);

  SidesInfo *aiSideInfo = TheSidesList->findSkirmishSideInfo(fullAiName);
  if (aiSideInfo) {
    return aiSideInfo->getBuildList();
  }

  // Fallback: If no AI side found, return player's list (better than nothing)
  return m_player->getBuildList();
}
