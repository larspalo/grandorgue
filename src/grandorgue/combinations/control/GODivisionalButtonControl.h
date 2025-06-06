/*
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2025 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#ifndef GODIVISIONALBUTTONCONTROL_H
#define GODIVISIONALBUTTONCONTROL_H

#include <wx/string.h>

#include "combinations/model/GODivisionalCombination.h"
#include "control/GOPushbuttonControl.h"

class GOConfigReader;
class GOOrganModel;

/**
 * A divisional combination button for a manual. It is used only for odf-defined
 * (old style) divisionals and not for setter (banked) divisionals
 */
class GODivisionalButtonControl : public GOPushbuttonControl {
private:
  GOOrganModel &r_OrganModel;

  // A manual the divisional button belongs to
  unsigned m_ManualN;
  // An index of divisionals in this manual
  unsigned m_DivisionalIndex;
  // A combination for this divisional
  GODivisionalCombination m_combination;

public:
  GODivisionalButtonControl(
    GOOrganModel &organModel,
    unsigned manualNumber,
    unsigned divisionalIndex,
    const GOMidiObjectContext *pContext);

  GODivisionalCombination &GetCombination() { return m_combination; }

  void Init(
    GOConfigReader &cfg, const wxString &group, const wxString &name) override;

  void Load(GOConfigReader &cfg, const wxString &group) override;

  void LoadCombination(GOConfigReader &cfg) override;
  void Save(GOConfigWriter &cfg) override;

  void Push() override;
};

#endif /* GODIVISIONALBUTTONCONTROL_H */
