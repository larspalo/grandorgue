/*
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2025 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#include "GOOrganModel.h"

#include <wx/intl.h>

#include "combinations/control/GOGeneralButtonControl.h"
#include "config/GOConfig.h"
#include "config/GOConfigReader.h"
#include "control/GOPistonControl.h"
#include "midi/objects/GOMidiObjectContext.h"
#include "modification/GOModificationListener.h"

#include "GODivisionalCoupler.h"
#include "GOEnclosure.h"
#include "GOManual.h"
#include "GORank.h"
#include "GOReferencingObject.h"
#include "GOSwitch.h"
#include "GOTremulant.h"
#include "GOWindchest.h"

static const GOMidiObjectContext MIDI_CONTEXT_ENCLOSURES(
  wxT("enclosures"), _("enclosures"));
static const GOMidiObjectContext MIDI_CONTEXT_GENERALS(
  wxT("generals"), _("generals"));
static const GOMidiObjectContext MIDI_CONTEXT_MANUALS(
  wxT("manuals"), _("manuals"));
static const GOMidiObjectContext MIDI_CONTEXT_RANKS(wxT("ranks"), _("ranks"));
static const GOMidiObjectContext MIDI_CONTEXT_SWITCHES(
  wxT("switches"), _("switches"));
static const GOMidiObjectContext MIDI_CONTEXT_TREMULANTS(
  wxT("tremulants"), _("tremulants"));
static const wxString WX_OBJ_NUM_FMT = wxT("%03u");

GOOrganModel::GOOrganModel(GOConfig &config)
  : m_config(config),
    m_GeneralTemplate(*this),
    m_DivisionalsStoreIntermanualCouplers(false),
    m_DivisionalsStoreIntramanualCouplers(false),
    m_DivisionalsStoreTremulants(false),
    m_GeneralsStoreDivisionalCouplers(false),
    m_CombinationsStoreNonDisplayedDrawstops(false),
    m_RootPipeConfigNode(nullptr, this, nullptr),
    m_OrganModelModified(false),
    m_FirstManual(0),
    m_ODFManualCount(0),
    m_ODFRankCount(0) {
  RegisterCombinationButtonSet(this);
}

GOOrganModel::~GOOrganModel() {}

unsigned GOOrganModel::GetRecorderElementID(const wxString &name) {
  return m_config.GetMidiMap().GetElementByString(name);
}

static const wxString WX_ORGAN = wxT("Organ");

static void set_name_for_context(GOMidiObject *pObj, unsigned n) {
  pObj->SetNameForContext(wxString::Format(WX_OBJ_NUM_FMT, n));
}

void GOOrganModel::Load(GOConfigReader &cfg) {
  m_OrganName = cfg.ReadStringTrim(ODFSetting, WX_ORGAN, wxT("ChurchName"));
  m_DivisionalsStoreIntermanualCouplers = cfg.ReadBoolean(
    ODFSetting, WX_ORGAN, wxT("DivisionalsStoreIntermanualCouplers"));
  m_DivisionalsStoreIntramanualCouplers = cfg.ReadBoolean(
    ODFSetting, WX_ORGAN, wxT("DivisionalsStoreIntramanualCouplers"));
  m_DivisionalsStoreTremulants
    = cfg.ReadBoolean(ODFSetting, WX_ORGAN, wxT("DivisionalsStoreTremulants"));
  m_GeneralsStoreDivisionalCouplers = cfg.ReadBoolean(
    ODFSetting, WX_ORGAN, wxT("GeneralsStoreDivisionalCouplers"));
  m_CombinationsStoreNonDisplayedDrawstops = cfg.ReadBoolean(
    ODFSetting, WX_ORGAN, wxT("CombinationsStoreNonDisplayedDrawstops"), false);

  unsigned NumberOfWindchestGroups = cfg.ReadInteger(
    ODFSetting, WX_ORGAN, wxT("NumberOfWindchestGroups"), 1, 999);

  m_RootPipeConfigNode.Load(cfg, WX_ORGAN, wxEmptyString);
  m_windchests.resize(0);
  for (unsigned i = 0; i < NumberOfWindchestGroups; i++)
    m_windchests.push_back(new GOWindchest(*this));

  m_ODFManualCount
    = cfg.ReadInteger(ODFSetting, WX_ORGAN, wxT("NumberOfManuals"), 1, 16) + 1;
  m_FirstManual
    = cfg.ReadBoolean(ODFSetting, WX_ORGAN, wxT("HasPedals")) ? 0 : 1;

  m_manuals.resize(0);
  m_manuals.resize(m_FirstManual); // Add empty slot for pedal, if necessary
  for (unsigned int i = m_FirstManual; i < m_ODFManualCount; i++)
    m_manuals.push_back(new GOManual(*this, i, &MIDI_CONTEXT_MANUALS));

  for (unsigned i = m_ODFManualCount, l = i + 4; i < l; i++)
    m_manuals.push_back(new GOManual(*this, i, &MIDI_CONTEXT_MANUALS));

  unsigned NumberOfEnclosures
    = cfg.ReadInteger(ODFSetting, WX_ORGAN, wxT("NumberOfEnclosures"), 0, 999);

  m_enclosures.resize(0);
  for (unsigned i = 0; i < NumberOfEnclosures; i++) {
    GOEnclosure *pEnclosure = new GOEnclosure(*this);
    unsigned num = i + 1;

    pEnclosure->SetContext(&MIDI_CONTEXT_ENCLOSURES);
    set_name_for_context(pEnclosure, num);
    pEnclosure->Load(cfg, wxString::Format(wxT("Enclosure%03u"), i + 1));
    m_enclosures.push_back(pEnclosure);
  }

  // Switches must be loaded before manuals because manuals reference to
  // switches
  unsigned NumberOfSwitches
    = cfg.ReadInteger(ODFSetting, WX_ORGAN, wxT("NumberOfSwitches"), 0, 999, 0);
  m_switches.resize(0);
  for (unsigned i = 0; i < NumberOfSwitches; i++) {
    GOSwitch *pSwitch = new GOSwitch(*this);
    unsigned num = i + 1;

    set_name_for_context(pSwitch, num);
    pSwitch->Load(cfg, wxString::Format(wxT("Switch%03d"), num));
    m_switches.push_back(pSwitch);
  }

  unsigned NumberOfTremulants
    = cfg.ReadInteger(ODFSetting, WX_ORGAN, wxT("NumberOfTremulants"), 0, 999);
  for (unsigned i = 0; i < NumberOfTremulants; i++) {
    GOTremulant *pTremulant = new GOTremulant(*this);
    unsigned num = i + 1;

    pTremulant->SetContext(&MIDI_CONTEXT_TREMULANTS);
    set_name_for_context(pTremulant, num);
    pTremulant->Load(cfg, wxString::Format(wxT("Tremulant%03u"), num), num);
    m_tremulants.push_back(pTremulant);
  }

  for (unsigned i = 0; i < NumberOfWindchestGroups; i++)
    m_windchests[i]->Load(
      cfg, wxString::Format(wxT("WindchestGroup%03d"), i + 1), i);

  m_ODFRankCount = cfg.ReadInteger(
    ODFSetting, WX_ORGAN, wxT("NumberOfRanks"), 0, 999, false);
  for (unsigned i = 0; i < m_ODFRankCount; i++) {
    GORank *pRank = new GORank(*this);
    unsigned num = i + 1;

    pRank->SetContext(&MIDI_CONTEXT_RANKS);
    set_name_for_context(pRank, num);
    pRank->Load(cfg, wxString::Format(wxT("Rank%03d"), num), -1);
    m_ranks.push_back(pRank);
  }

  // Switches must be loaded before manuals because manuals reference to
  // switches
  for (unsigned int i = m_FirstManual; i < m_ODFManualCount; i++)
    m_manuals[i]->Load(cfg, wxString::Format(wxT("Manual%03d"), i));

  unsigned min_key = 0xff, max_key = 0;
  for (unsigned i = GetFirstManualIndex(); i < GetODFManualCount(); i++) {
    GOManual *manual = GetManual(i);
    if ((unsigned)manual->GetFirstLogicalKeyMIDINoteNumber() < min_key)
      min_key = manual->GetFirstLogicalKeyMIDINoteNumber();
    if (
      manual->GetFirstLogicalKeyMIDINoteNumber() + manual->GetLogicalKeyCount()
      > max_key)
      max_key = manual->GetFirstLogicalKeyMIDINoteNumber()
        + manual->GetLogicalKeyCount();
  }
  for (unsigned i = GetODFManualCount(); i <= GetManualAndPedalCount(); i++)
    GetManual(i)->Init(
      cfg,
      wxString::Format(wxT("SetterFloating%03d"), i - GetODFManualCount() + 1),
      min_key,
      max_key - min_key);

  unsigned NumberOfReversiblePistons = cfg.ReadInteger(
    ODFSetting, WX_ORGAN, wxT("NumberOfReversiblePistons"), 0, 32);
  m_pistons.resize(0);
  for (unsigned i = 0; i < NumberOfReversiblePistons; i++) {
    m_pistons.push_back(new GOPistonControl(*this));
    m_pistons[i]->Load(
      cfg, wxString::Format(wxT("ReversiblePiston%03d"), i + 1));
  }

  unsigned NumberOfDivisionalCouplers = cfg.ReadInteger(
    ODFSetting, WX_ORGAN, wxT("NumberOfDivisionalCouplers"), 0, 8);
  m_DivisionalCoupler.resize(0);
  for (unsigned i = 0; i < NumberOfDivisionalCouplers; i++) {
    m_DivisionalCoupler.push_back(new GODivisionalCoupler(*this));
    m_DivisionalCoupler[i]->Load(
      cfg, wxString::Format(wxT("DivisionalCoupler%03d"), i + 1));
  }

  for (unsigned i = 0; i < m_enclosures.size(); i++)
    m_enclosures[i]->SetElementId(
      GetRecorderElementID(wxString::Format(wxT("E%d"), i)));

  for (unsigned i = 0; i < m_switches.size(); i++) {
    GOSwitch *pSwitch = m_switches[i];
    int switchManualIdx = pSwitch->GetAssociatedManualN();
    const GOMidiObjectContext *pSwitchContext = switchManualIdx >= 0
      ? m_manuals[switchManualIdx]->GetSwitchesContext()
      : nullptr;
    unsigned num
      = switchManualIdx >= 0 ? pSwitch->GetIndexInManual() + 1 : i + 1;

    pSwitch->SetContext(pSwitchContext);
    set_name_for_context(pSwitch, num);
    pSwitch->SetElementId(
      GetRecorderElementID(wxString::Format(wxT("S%d"), i)));
  }

  for (unsigned i = 0; i < m_tremulants.size(); i++)
    m_tremulants[i]->SetElementId(
      GetRecorderElementID(wxString::Format(wxT("T%d"), i)));

  for (GOReferencingObject *pObj : GetReferencingObjects())
    pObj->ResolveReferences();
}

void GOOrganModel::LoadCmbButtons(GOConfigReader &cfg) {
  // Generals
  unsigned NumberOfGenerals
    = cfg.ReadInteger(ODFSetting, WX_ORGAN, wxT("NumberOfGenerals"), 0, 99);

  m_GeneralTemplate.InitGeneral();
  m_generals.resize(0);
  for (unsigned i = 0; i < NumberOfGenerals; i++) {
    GOGeneralButtonControl *pGeneral = new GOGeneralButtonControl(*this, false);

    pGeneral->SetContext(&MIDI_CONTEXT_GENERALS);
    pGeneral->Load(cfg, wxString::Format(wxT("General%03d"), i + 1));
    m_generals.push_back(pGeneral);
  }

  // Divisionals
  for (unsigned i = m_FirstManual; i < m_manuals.size(); i++)
    m_manuals[i]->LoadDivisionals(cfg);
}

void GOOrganModel::SetOrganModelModified(bool modified) {
  if (modified != m_OrganModelModified)
    m_OrganModelModified = modified;
  if (modified)
    m_ModificationProxy.OnIsModifiedChanged(modified);
}

void GOOrganModel::UpdateTremulant(GOTremulant *tremulant) {
  for (unsigned i = 0; i < m_windchests.size(); i++)
    m_windchests[i]->UpdateTremulant(tremulant);
}

void GOOrganModel::UpdateVolume() {
  for (unsigned i = 0; i < m_windchests.size(); i++)
    m_windchests[i]->UpdateVolume();
}

unsigned GOOrganModel::AddWindchest(GOWindchest *windchest) {
  m_windchests.push_back(windchest);
  return m_windchests.size();
}

unsigned GOOrganModel::GetManualAndPedalCount() {
  if (!m_manuals.size())
    return 0;
  return m_manuals.size() - 1;
}

unsigned GOOrganModel::GetODFManualCount() { return m_ODFManualCount; }

unsigned GOOrganModel::GetFirstManualIndex() { return m_FirstManual; }

GOManual *GOOrganModel::GetManual(unsigned index) { return m_manuals[index]; }

unsigned GOOrganModel::GetStopCount() {
  unsigned cnt = 0;
  for (unsigned i = m_FirstManual; i < m_manuals.size(); i++)
    cnt += m_manuals[i]->GetStopCount();
  return cnt;
}

unsigned GOOrganModel::GetCouplerCount() {
  unsigned cnt = 0;
  for (unsigned i = m_FirstManual; i < m_manuals.size(); i++)
    cnt += m_manuals[i]->GetCouplerCount();
  return cnt;
}

unsigned GOOrganModel::GetODFCouplerCount() {
  unsigned cnt = 0;
  for (unsigned i = m_FirstManual; i < m_manuals.size(); i++)
    cnt += m_manuals[i]->GetODFCouplerCount();
  return cnt;
}

GOEnclosure *GOOrganModel::GetEnclosureElement(unsigned index) {
  return m_enclosures[index];
}

unsigned GOOrganModel::GetEnclosureCount() { return m_enclosures.size(); }

unsigned GOOrganModel::AddEnclosure(GOEnclosure *enclosure) {
  m_enclosures.push_back(enclosure);
  return m_enclosures.size() - 1;
}

int GOOrganModel::FindSwitchByName(const wxString &name) const {
  int resIndex = -1;

  for (unsigned l = m_switches.size(), i = 0; i < l; i++)
    if (m_switches[i]->GetName() == name) {
      resIndex = i;
      break;
    }
  return resIndex;
}

int GOOrganModel::FindTremulantByName(const wxString &name) const {
  int resIndex = -1;

  for (unsigned l = m_tremulants.size(), i = 0; i < l; i++)
    if (m_tremulants[i]->GetName() == name) {
      resIndex = i;
      break;
    }
  return resIndex;
}

GORank *GOOrganModel::GetRank(unsigned index) { return m_ranks[index]; }

unsigned GOOrganModel::GetODFRankCount() { return m_ODFRankCount; }

void GOOrganModel::AddRank(GORank *rank) {
  rank->SetContext(&MIDI_CONTEXT_RANKS);
  m_ranks.push_back(rank);
}

unsigned GOOrganModel::GetNumberOfReversiblePistons() {
  return m_pistons.size();
}

GOPistonControl *GOOrganModel::GetPiston(unsigned index) {
  return m_pistons[index];
}

int GOOrganModel::FindDivisionalCouplerByName(const wxString &name) const {
  int resIndex = -1;

  for (unsigned l = m_DivisionalCoupler.size(), i = 0; i < l; i++)
    if (m_DivisionalCoupler[i]->GetName() == name) {
      resIndex = i;
      break;
    }
  return resIndex;
}

unsigned GOOrganModel::GetGeneralCount() { return m_generals.size(); }

GOGeneralButtonControl *GOOrganModel::GetGeneral(unsigned index) {
  return m_generals[index];
}

void GOOrganModel::FillCoupledManualsForDivisional(
  unsigned startManual, std::set<unsigned> &manualSet) const {
  // protection against infinite recursion
  if (manualSet.insert(startManual).second) {
    // walk on the couplers
    for (const GODivisionalCoupler *coupler : m_DivisionalCoupler)
      for (auto otherManual : coupler->GetCoupledManuals(startManual))
        FillCoupledManualsForDivisional(otherManual, manualSet);
  }
}

std::set<unsigned> GOOrganModel::GetCoupledManualsForDivisional(
  unsigned startManual) const {
  std::set<unsigned> res;

  FillCoupledManualsForDivisional(startManual, res);
  return res;
}

void GOOrganModel::UpdateAllButtonsLight(
  GOButtonControl *buttonToLight, int manualIndexOnlyFor) {
  if (manualIndexOnlyFor < 0)
    for (GOGeneralButtonControl *pGeneral : m_generals)
      updateOneButtonLight(pGeneral, buttonToLight);
}
