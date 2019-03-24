#include "hdfexport.h"
#include "scanHDF5.hpp"
#include <fstream>
#include <wx/progdlg.h>

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
	if ( !wxApp::OnInit() )
		return false;

	MyFrame * frame = new MyFrame(wxT("NWB Export"), 50, 50, 800, 600);

	frame->Show(true);

	return true;
}

MyFrame::MyFrame(const wxString& title, int x, int y, int w, int h)
       : wxFrame((wxFrame *)NULL, wxID_ANY, title, wxPoint(x, y), wxSize(w, h)),
         m_treeCtrl(NULL), m_textCtrl(NULL)
{
	SetBackgroundColour(*wxWHITE);

	// Menus
	wxMenu * file_menu = new wxMenu;

	file_menu->Append((int)CtrlIDs::kOpen, wxT("&Open\tCtrl-o"));
	file_menu->AppendSeparator();
	file_menu->Append((int)CtrlIDs::kAbout, wxT("&About"));
	file_menu->Append((int)CtrlIDs::kQuit, wxT("&Quit\tCtrl-q"));

	// add the menus to the menubar
	wxMenuBar *menu_bar = new wxMenuBar;
    menu_bar->Append(file_menu, wxT("&File"));
    SetMenuBar(menu_bar);

    m_panel = new wxPanel(this);
    m_sizer = new wxBoxSizer(wxVERTICAL);

    wxSizer * treeSizer = new wxBoxSizer(wxHORIZONTAL);
	m_treeCtrl = new MyTreeCtrl(m_panel, (int)CtrlIDs::kTreeCtrl, wxDefaultPosition,
								wxSize(750, 300), 1);
    treeSizer->Add(m_treeCtrl, wxSizerFlags().Expand());
	treeSizer->AddSpacer(10);

	wxSizer * buttonSizer = new wxBoxSizer(wxVERTICAL);
	// Start & end time
	wxSizer * timeSizer = new wxBoxSizer(wxHORIZONTAL);
	wxSpinCtrlDouble * scd;
	scd = createDoubleSpinAndAddToSizer(m_panel, timeSizer, wxString("Start time(s):"), (int)CtrlIDs::kStartTime);
	scd->SetIncrement(1.0/SAMPLE_RATE);
	scd->SetDigits(5);
	scd = createDoubleSpinAndAddToSizer(m_panel, timeSizer, wxString("End time(s):"), (int)CtrlIDs::kEndTime);
	scd->SetIncrement(1.0/SAMPLE_RATE);
	scd->SetDigits(5);
	// start and end channel
	wxSizer * channelSizer = new wxBoxSizer(wxHORIZONTAL);
	createSpinAndAddToSizer(m_panel, channelSizer, wxString("Start channel:"), (int)CtrlIDs::kStartChannel);
	createSpinAndAddToSizer(m_panel, channelSizer, wxString("End channel"), (int)CtrlIDs::kEndChannel);

	buttonSizer->Add(timeSizer, wxSizerFlags().Expand());
	buttonSizer->AddSpacer(10);
	buttonSizer->Add(channelSizer, wxSizerFlags().Expand());
	buttonSizer->AddSpacer(10);
	// export button
	createButtonAndAddToSizer(m_panel, buttonSizer, wxString("Export"), (int)CtrlIDs::kExport);

    m_sizer->Add(treeSizer, wxSizerFlags().Expand());
    m_sizer->AddSpacer(10);
    // create the text panel at the bottom
	m_textCtrl = new wxTextCtrl(m_panel, wxID_ANY, wxT(""), wxDefaultPosition,
								wxSize(300, 200), wxTE_MULTILINE | wxSUNKEN_BORDER);
	wxSizer * textSizer = new wxBoxSizer(wxHORIZONTAL);
	textSizer->Add(m_textCtrl, wxSizerFlags().Expand());
	textSizer->Add(buttonSizer);
	m_sizer->Add(textSizer, wxSizerFlags().Expand());

	// create a status bar
    CreateStatusBar(2);

	// Bind events to functions
	Bind(wxEVT_COMMAND_MENU_SELECTED, &MyFrame::OnOpen, this, (int)CtrlIDs::kOpen);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &MyFrame::OnAbout, this, (int)CtrlIDs::kAbout);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &MyFrame::OnQuit, this, (int)CtrlIDs::kQuit);

	m_panel->SetSizer(m_sizer);

}

MyFrame::~MyFrame()
{
	delete wxLog::SetActiveTarget(NULL);
}


void MyFrame::OnQuit(wxCommandEvent & WXUNUSED(event))
{
	if ( m_nwb_data ) {
		delete m_nwb_data;
	}
	Close(true);
}

void MyFrame::OnAbout(wxCommandEvent & WXUNUSED(event))
{
	wxMessageBox(wxT("HDF5 Export\n")
                 wxT("(c) Robin Hayman 2018"),
                 wxT("About export HDF5"),
                 wxOK | wxICON_INFORMATION, this);
}

void MyFrame::OnOpen(wxCommandEvent & WXUNUSED(event))
{
	wxFileDialog openFileDialog(this, ("Open nwb file"), defaultDir, "",
        "nwb files(*.nwb)|*.nwb",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    m_pathname = openFileDialog.GetPath().BeforeLast('/').ToStdString();
    m_filename = openFileDialog.GetPath().ToStdString();

    m_textCtrl->AppendText(wxString("Reading from file: \n") + wxString(m_filename));

	m_nwb_data = new NwbData(m_filename);
	std::map<std::string, std::string> hdfpaths = m_nwb_data->getPaths(m_filename);
	m_treeCtrl->AddItemsToTree(hdfpaths);

	wxLogTextCtrl * logWindow = new wxLogTextCtrl(m_textCtrl);
	delete wxLog::SetActiveTarget(logWindow);
}

void MyFrame::GetDataSetInfo(const std::string & pathToDataSet, const std::string & outputfname) {
	if ( m_nwb_data ) {
		ExportParams params = getExportParams();
		int nSamples, nChannels;
		m_nwb_data->getDataSpaceDimensions(pathToDataSet, nSamples, nChannels);
		m_textCtrl->AppendText(wxString(std::to_string(nChannels)) + wxString(" channels were recorded over ") +
			wxString(std::to_string(nSamples/3e4)) + wxString(" seconds (") + wxString(std::to_string(nSamples)) + wxString(" samples)"));
		wxProgressDialog prog{wxString("Exporting data"), wxString("Exporting .dat file... "), static_cast<int>((params.m_end_time-params.m_start_time)), this};
		m_nwb_data->ExportData(pathToDataSet, outputfname, params, prog);
    }
}

ExportParams MyFrame::getExportParams() {
	// Grab some values about what ranges to export from the various controls
	wxWindowList & windows = m_panel->GetChildren();
    wxWindowList::iterator iter;
    unsigned int start_channel, end_channel;
    double start_time, end_time;
    for (iter = windows.begin(); iter != windows.end(); ++iter)
    {
        wxWindow * win = *iter;
        wxWindowID id = win->GetId();
        if ( id == (int)CtrlIDs::kStartTime )
        {
        	wxSpinCtrlDouble * scd = (wxSpinCtrlDouble*)win;
        	start_time = scd->GetValue();
        }
        if ( id == (int)CtrlIDs::kEndTime ) {
        	wxSpinCtrlDouble * scd = (wxSpinCtrlDouble*)win;
        	end_time = scd->GetValue();
        }
        if ( id == (int)CtrlIDs::kStartChannel ) {
        	wxSpinCtrl * sc = (wxSpinCtrl*)win;
        	start_channel = sc->GetValue();
        }
        if ( id == (int)CtrlIDs::kEndChannel ) {
        	wxSpinCtrl * sc = (wxSpinCtrl*)win;
        	end_channel = sc->GetValue();
        }
    }
    ExportParams params;
    params.m_start_channel = start_channel;
    params.m_end_channel = end_channel;
    params.m_start_time = (unsigned int)(start_time * SAMPLE_RATE);
    params.m_end_time = (unsigned int)(end_time * SAMPLE_RATE);
    return params;
}

void MyFrame::setExportParams(const ExportParams & params) {
	wxWindowList & windows = m_panel->GetChildren();
    wxWindowList::iterator iter;
    for (iter = windows.begin(); iter != windows.end(); ++iter)
    {
        wxWindow * win = *iter;
        wxWindowID id = win->GetId();
        if ( id == (int)CtrlIDs::kStartTime )
        {
        	wxSpinCtrlDouble * scd = (wxSpinCtrlDouble*)win;
        	scd->SetRange(params.m_start_time / (double)SAMPLE_RATE, params.m_end_time / (double)SAMPLE_RATE);
        	scd->SetValue(params.m_start_time / (double)SAMPLE_RATE);
        }
        if ( id == (int)CtrlIDs::kEndTime ) {
        	wxSpinCtrlDouble * scd = (wxSpinCtrlDouble*)win;
        	scd->SetRange(params.m_start_time / (double)SAMPLE_RATE, params.m_end_time / (double)SAMPLE_RATE);
        	scd->SetValue(params.m_end_time / (double)SAMPLE_RATE);
        }
        if ( id == (int)CtrlIDs::kStartChannel ) {
        	wxSpinCtrl * sc = (wxSpinCtrl*)win;
        	sc->SetRange(params.m_start_channel, params.m_end_channel);
        	sc->SetValue(params.m_start_channel);
        }
        if ( id == (int)CtrlIDs::kEndChannel ) {
        	wxSpinCtrl * sc = (wxSpinCtrl*)win;
        	sc->SetRange(params.m_start_channel, params.m_end_channel);
        	sc->SetValue(params.m_end_channel);
        }
    }
}

// void MyFrame::OnExport(wxCommandEvent & evt) {
// 	wxTreeItemId id = m_treeCtrl->GetFocusedItem();
// 	std::string itemName = m_treeCtrl->GetItemText(id).ToStdString();

//     wxString longName = wxString(m_filename);
//     wxString suggestedName = longName.AfterLast('/');
//     suggestedName = suggestedName.BeforeLast('.');
//     suggestedName = suggestedName +  ".dat";
//     // m_pathname set in OnOpen()
//     std::cout << "m_pathname " << m_pathname << std::endl;
// 	wxFileDialog saveFileDialog(this, ("Save .dat file"), wxString(m_pathname), suggestedName,
//         "dat files(*.dat)|*.dat",
//         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
//     if (saveFileDialog.ShowModal() == wxID_CANCEL)
//         return;

//     std::string outputfname = saveFileDialog.GetPath().ToStdString();
// 	GetDataSetInfo(itemName, outputfname);
// }

wxSpinCtrl * MyFrame::createSpinAndAddToSizer(wxWindow * parent, wxSizer * sizer, const wxString & label, wxWindowID id)
{
    wxSpinCtrl * sc = new wxSpinCtrl(parent, id, label, wxDefaultPosition, wxDefaultSize, 0, 0, 70, 1);
    sc->Bind(wxEVT_SPINCTRL, &MyFrame::OnSpinEvent, this, id);
    wxBoxSizer * s1 = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText * txt = new wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, wxSize(60,20));
    s1->Add(txt, wxSizerFlags().Border(wxLEFT | wxRIGHT, 5).Expand());
    s1->Add(sc, wxSizerFlags().Border(wxLEFT | wxRIGHT, 5));
    sizer->Add(s1);
    return sc;
}

wxSpinCtrlDouble * MyFrame::createDoubleSpinAndAddToSizer(wxWindow * parent, wxSizer * sizer, const wxString & label, wxWindowID id)
{
    wxSpinCtrlDouble * sc = new wxSpinCtrlDouble(parent, id, label, wxDefaultPosition, wxDefaultSize, 0, 0, 30000, 0, 1/10.0);
    sc->Bind(wxEVT_SPINCTRLDOUBLE, &MyFrame::OnSpinDoubleEvent, this, id);
    wxBoxSizer * s1 = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText * txt = new wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, wxSize(60,20));
    s1->Add(txt, wxSizerFlags().Border(wxLEFT | wxRIGHT, 5).Expand());
    s1->Add(sc, wxSizerFlags().Border(wxLEFT | wxRIGHT, 5));
    sizer->Add(s1);
    return sc;
}

wxButton * MyFrame::createButtonAndAddToSizer(wxWindow * parent, wxSizer * sizer, const wxString & label, wxWindowID id)
{
    wxButton * btn = new wxButton(parent, id, label, wxDefaultPosition, wxDefaultSize, 0);
    btn->Bind(wxEVT_BUTTON, &MyFrame::OnButtonEvent, this, id);
    wxSizerItem * item = sizer->Add(btn, 0, wxLEFT | wxRIGHT, 5);
    item->SetId(id);
    sizer->Add(0, 2, wxGROW);
    return btn;
}

void MyFrame::OnSpinEvent(wxSpinEvent & evt) {

}

void MyFrame::OnSpinDoubleEvent(wxSpinDoubleEvent & evt) {

}

void MyFrame::OnButtonEvent(wxCommandEvent & evt) {
	wxButton * btn = (wxButton*)evt.GetEventObject();
    int id = btn->GetId();
    if ( id == (int)CtrlIDs::kExport ) {
    	wxTreeItemId id = m_treeCtrl->GetFocusedItem();
		std::string itemName = m_treeCtrl->GetItemText(id).ToStdString();
        wxString longName = wxString(m_filename);
        wxString suggestedName = longName.AfterLast('/');
        suggestedName = suggestedName.BeforeLast('.');
        suggestedName = suggestedName +  ".dat";
		// Open the file dialog
		wxFileDialog saveFileDialog(this, ("Save .dat file"), wxString(m_pathname), suggestedName,
        "dat files(*.dat)|*.dat",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	    if (saveFileDialog.ShowModal() == wxID_CANCEL)
	        return;

	    std::string outputfname = saveFileDialog.GetPath().ToStdString();
		GetDataSetInfo(itemName, outputfname);
    }
}

void MyFrame::UpdateControls(const std::string & pathToDataSet) {
	if ( m_nwb_data ) {
		int nSamples, nChannels;
		m_nwb_data->getDataSpaceDimensions(pathToDataSet, nSamples, nChannels);
		ExportParams params;
		params.m_start_channel = 0;
		params.m_end_channel = nChannels;
		params.m_start_time = 0.0;
		if ( pathToDataSet.find("processor") != std::string::npos )
			params.m_end_time = nSamples;
		else
			params.m_end_time = nSamples;
		setExportParams(params);
	}
}

/*
----------------     MyTreeCtrl class   ------------------
*/

MyTreeCtrl::MyTreeCtrl(wxWindow *parent, const wxWindowID id,
                       const wxPoint& pos, const wxSize& size,
                       long style)
          : wxTreeCtrl(parent, id, pos, size, style)
{
	m_parent = (MyFrame*)parent->GetParent();
	Bind(wxEVT_TREE_SEL_CHANGED, &MyTreeCtrl::OnSelectionChanged, this, (int)CtrlIDs::kTreeCtrl);
}

void MyTreeCtrl::OnSelectionChanged(wxTreeEvent & evt) {

	wxTreeItemId id = GetFocusedItem();
	std::string focusedItemStr = GetItemText(id).ToStdString();
	m_parent->UpdateControls(focusedItemStr);
}

void MyTreeCtrl::OnGetInfo(wxTreeEvent & event)
{

}

void MyTreeCtrl::OnItemRightClick(wxTreeEvent & event)
{

}

void MyTreeCtrl::AddItemsToTree(const std::map<std::string, std::string> items)
{
	int image = 1;
	wxTreeItemId rootId = AddRoot(wxT("Datasets"),
                                  image, image,
                                  new MyTreeItemData(wxT("Root item")));
	AddItemsRecursively(rootId, items, "Dataset");
	ExpandAll();
}

void MyTreeCtrl::AddItemsRecursively(const wxTreeItemId & idParent, const std::map<std::string, std::string> items, std::string hdftype)
{
	wxString str;
	for (const auto & i : items )
	{
		if ( i.second == hdftype ) {
			str.Printf(wxT("%s"), wxString(i.first));
			wxTreeItemId id = AppendItem(idParent, str, 1,1, new MyTreeItemData(str));
			SetItemState(id, 0);
		}
	}
}

/*
----------------     MyTreeItemData class   ------------------
*/
void MyTreeItemData::ShowInfo(wxTreeCtrl *tree)
{
	wxLogMessage(wxT("Item '%s':"), m_desc.c_str());
}