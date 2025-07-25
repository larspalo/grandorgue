/*
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2025 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#ifndef GOMETRONOME_H
#define GOMETRONOME_H

#include "control/GOElementCreator.h"
#include "control/GOLabelControl.h"
#include "sound/GOSoundStateHandler.h"

#include "GOSaveableObject.h"
#include "GOTimerCallback.h"

class GOMidiEvent;
class GORank;
class GOOrganController;

class GOMetronome : private GOTimerCallback,
                    private GOSoundStateHandler,
                    private GOSaveableObject,
                    public GOElementCreator {
public:
  enum {
    ID_METRONOME_ON = 0,
    ID_METRONOME_MEASURE_P1,
    ID_METRONOME_MEASURE_M1,
    ID_METRONOME_BEAT_P1,
    ID_METRONOME_BEAT_M1,
    ID_METRONOME_BEAT_P10,
    ID_METRONOME_BEAT_M10,
  };
  static const ButtonDefinitionEntry *const P_BUTTON_DEFS;

private:
  GOOrganController *m_OrganController;
  unsigned m_BPM;
  unsigned m_MeasureLength;
  unsigned m_Pos;
  bool m_Running;
  GOLabelControl m_BPMDisplay;
  GOLabelControl m_MeasureDisplay;
  GORank *m_rank;
  unsigned m_StopID;

  void HandleTimer() override;

  void ButtonStateChanged(int id, bool newState) override;

  void AbortPlayback() override;
  void PreparePlayback() override;

  void Save(GOConfigWriter &cfg) override;

  void StartTimer();
  void StopTimer();
  void UpdateState();
  void UpdateBPM(int val);
  void UpdateMeasure(int val);

public:
  GOMetronome(GOOrganController *organController);
  virtual ~GOMetronome();

  void Load(GOConfigReader &cfg) override;

  GOEnclosure *GetEnclosure(const wxString &name, bool is_panel) override;
  GOLabelControl *GetLabelControl(const wxString &name, bool is_panel) override;
};

#endif
