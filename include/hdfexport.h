#ifndef WX_PRECOMP
    #include "wx/wx.h"
    #include "wx/log.h"
#endif

#include "wx/image.h"
#include "wx/imaglist.h"
#include "wx/treectrl.h"
#include "wx/frame.h"

#include <string>
#include <iostream>
#include <map>
#include <hdf5.h>
#include <H5Cpp.h>
#include <wx/spinctrl.h>
#include <wx/progdlg.h>

class MyFrame;
class NwbData;

const std::string defaultDir{"/home/robin/Dropbox/Science/Recordings/OpenEphys"};

enum class CtrlIDs
{
	kTreeCtrl,
	kOpen,
	kAbout,
	kQuit,
	kExport,
	kPSD,
	kSpectrogram,
	kStartTime,
	kEndTime,
	kStartChannel,
	kEndChannel,
	kSplitIntoTetrodes,
	kSaveEEGOnly
};

class MyApp : public wxApp
{
public:
	MyApp() {};
	
	bool OnInit() wxOVERRIDE;

private:

};

class MyTreeItemData : public wxTreeItemData
{
public:
	MyTreeItemData(const wxString & desc) : m_desc(desc) {};

	void ShowInfo(wxTreeCtrl * tree);
	wxString const & GetDesc() const { return m_desc; }

private:
	wxString m_desc;
};

class MyTreeCtrl : public wxTreeCtrl
{
private:
	MyFrame * m_parent;
public:
	MyTreeCtrl() {};
	MyTreeCtrl(wxWindow * parent, const wxWindowID id,
				const wxPoint & pos, const wxSize & sz,
				long style);
	virtual ~MyTreeCtrl() {};

	void OnGetInfo(wxTreeEvent & event);
	void OnItemRightClick(wxTreeEvent & event); // export from here
	void AddItemsToTree(const std::map<std::string, std::string> items); // add here kinda...

private:
	void AddItemsRecursively(const wxTreeItemId & idParent, const std::map<std::string, std::string> items, std::string hdftype); // ... but really add here!
	void OnSelectionChanged(wxTreeEvent &);
};

struct ExportParams {
	unsigned int m_start_channel;
	unsigned int m_end_channel;
	unsigned int m_start_time;
	unsigned int m_end_time;
	bool m_split_into_tetrodes = false;
	bool m_save_eeg_only = false;
};

class MyFrame : public wxFrame
{
public:
	MyFrame(const wxString & title, int x, int y, int w, int h);
	virtual ~MyFrame();

	void OnOpen(wxCommandEvent & event);
	void OnQuit(wxCommandEvent & event);
	void OnAbout(wxCommandEvent & event);
	void OnSaveSelected(wxCommandEvent & event) {};
	void UpdateControls(const std::string &);
	void GetDataSetInfo(const std::string &, const std::string &);

private:
	void CreateTree(long style) {};
	wxSpinCtrl * createSpinAndAddToSizer(wxWindow * parent, wxSizer * sizer, const wxString & label, wxWindowID id);
	wxSpinCtrlDouble * createDoubleSpinAndAddToSizer(wxWindow * parent, wxSizer * sizer, const wxString & label, wxWindowID id);
	wxButton * createButtonAndAddToSizer(wxWindow * parent, wxSizer * sizer, const wxString & label, wxWindowID id);
	wxCheckBox * createCheckBoxAndAddToSizer(wxWindow * parent, wxSizer *, const wxString & label, wxWindowID);
	void OnSpinEvent(wxSpinEvent &);
	void OnSpinDoubleEvent(wxSpinDoubleEvent &);
	void OnButtonEvent(wxCommandEvent &);
	void OnCheckEvent(wxCommandEvent &);
	ExportParams getExportParams();
	void setExportParams(const ExportParams &);
	wxBoxSizer * m_sizer;
	wxPanel * m_panel;
	MyTreeCtrl * m_treeCtrl;
	wxTextCtrl * m_textCtrl;
	wxProgressDialog * m_prog;
	NwbData * m_nwb_data;
	std::string m_filename = "";
	std::string m_pathname = "";
	double m_start_time;
	double m_end_time;
	unsigned int m_start_channel;
	unsigned int m_end_channel;
};
