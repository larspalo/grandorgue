/*
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2025 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#ifndef GOBUTTONCONTROL_H
#define GOBUTTONCONTROL_H

#include <wx/string.h>

#include "midi/GOMidiReceiver.h"
#include "midi/GOMidiSender.h"
#include "midi/GOMidiShortcutReceiver.h"
#include "midi/objects/GOMidiObject.h"
#include "sound/GOSoundStateHandler.h"

#include "GOControl.h"
#include "GOEventHandler.h"
#include "GOSaveableObject.h"

class GOConfigReader;
class GOConfigWriter;
class GOMidiEvent;
class GOMidiMap;
class GOOrganModel;

class GOButtonControl : public GOControl,
                        private GOEventHandler,
                        public GOSaveableObject,
                        public GOSoundStateHandler,
                        public GOMidiObject {
private:
  GOMidiMap &r_MidiMap;

protected:
  GOOrganModel &r_OrganModel;
  GOMidiReceiver m_midi;
  GOMidiSender m_sender;
  GOMidiShortcutReceiver m_shortcut;
  bool m_Pushbutton;
  bool m_Displayed;
  wxString m_Name;
  bool m_Engaged;
  bool m_DisplayInInvertedState;
  bool m_ReadOnly;
  bool m_IsPiston;

  void ProcessMidi(const GOMidiEvent &event) override;
  void HandleKey(int key) override;

  void Save(GOConfigWriter &cfg) override;

  void AbortPlayback() override;
  void PreparePlayback() override;
  void PrepareRecording() override;

  GOMidiReceiverBase *GetMidiReceiver() override {
    return IsReadOnly() ? nullptr : &m_midi;
  }

  GOMidiSender *GetMidiSender() override { return &m_sender; }

  GOMidiShortcutReceiver *GetMidiShortcutReceiver() override {
    return IsReadOnly() ? nullptr : &m_shortcut;
  }

public:
  GOButtonControl(
    GOOrganModel &organModel,
    GOMidiReceiverType midi_type,
    bool pushbutton,
    bool isPiston = false);
  void Init(GOConfigReader &cfg, const wxString &group, const wxString &name);
  void Load(GOConfigReader &cfg, const wxString &group);
  bool IsDisplayed();
  void SetDisplayed(bool displayed) { m_Displayed = displayed; }
  bool IsReadOnly() const { return m_ReadOnly; }
  const wxString &GetName() const { return m_Name; }
  bool IsPiston() const { return m_IsPiston; }

  virtual void Push();
  virtual void SetButtonState(bool on);
  virtual void Display(bool onoff);
  bool IsEngaged() const;
  bool DisplayInverted() const;
  void SetElementID(int id);
  void SetShortcutKey(unsigned key);
  void SetPreconfigIndex(unsigned index);

  const wxString &GetMidiName() const override { return GetName(); }

  wxString GetElementStatus() override;
  std::vector<wxString> GetElementActions() override;
  void TriggerElementActions(unsigned no) override;
};

#endif
