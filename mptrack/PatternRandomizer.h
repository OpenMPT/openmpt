#pragma once

class CViewPattern;
class CPatternRandomizerGUI;

class CPatternRandomizer
{
public:
	CPatternRandomizer(CViewPattern* viewPat);
	~CPatternRandomizer(void);
	bool showGUI(void);
	bool isGUIVisible(void);

private:
	CPatternRandomizerGUI* m_pRandomizerGUI;
	CViewPattern* m_pViewPattern;
	
	bool m_bRandomizeNotes, m_bRandomizeInstruments,
		 m_bRandomizeVolCmds, m_bRandomizeEffects;

};
