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

	// treeSizer->Add(buttonSizer);

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

void MyFrame::OnSize(wxSizeEvent & event)
{
	if ( m_treeCtrl && m_textCtrl )
		Resize();

	event.Skip();
}

void MyFrame::Resize()
{
	// wxSize size = GetClientSize();
	// m_treeCtrl->SetSize(0, 0, size.x, size.y *2/3);
	// m_textCtrl->SetSize(0, 2*size.y/3, size.x, size.y/3);
}

void MyFrame::OnQuit(wxCommandEvent & WXUNUSED(event))
{
	if ( hdf_file ) {
		hdf_file->close();
		delete hdf_file;
	}
	Close(true);
}

void MyFrame::OnAbout(wxCommandEvent & WXUNUSED(event))
{
	wxMessageBox(wxT("HDF5 Export\n")
                 wxT("(c) Robin Hayman 2017"),
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

    m_filename = openFileDialog.GetPath().ToStdString();

    std::cout << "Reading from file: " << m_filename << std::endl;

	hdf_file = new H5::H5File(m_filename, H5F_ACC_RDONLY);
	H5::Group group = hdf_file->openGroup("/");
	std::map<std::string, std::string> hdfpaths = getPaths(m_filename);
	m_treeCtrl->AddItemsToTree(hdfpaths);

	wxLogTextCtrl * logWindow = new wxLogTextCtrl(m_textCtrl);
	delete wxLog::SetActiveTarget(logWindow);
}

void MyFrame::GetDataSetInfo(const std::string & pathToDataSet, const std::string & outputfname) {
	if ( hdf_file ) {
		if ( hdf_file->nameExists(pathToDataSet) ) {
			H5::DataSet dataset = hdf_file->openDataSet(pathToDataSet);
			hsize_t sz = dataset.getStorageSize();
			H5T_class_t type_class = dataset.getTypeClass();
			if ( type_class == H5T_INTEGER ) {
				H5::IntType inttype = dataset.getIntType();
				size_t precision = inttype.getPrecision();
				H5::DataSpace dataspace = dataset.getSpace();
				int rank = dataspace.getSimpleExtentNdims();
				hsize_t dims_out[2];
				int ndims = dataspace.getSimpleExtentDims(dims_out, NULL);
				int nSamples = dims_out[0];
				int nChannels = dims_out[1];
				// Get the parameters specified in the controls
				ExportParams params = getExportParams();
				hsize_t start_sample = (hsize_t)params.m_start_time;
				hsize_t end_sample = (hsize_t)params.m_end_time;
				hsize_t start_channel = (hsize_t)params.m_start_channel;
				hsize_t end_channel = (hsize_t)params.m_end_channel;
				hsize_t sample_block_inc = 30000;

				std::cout << std::endl << nChannels << " channels were recorded over " << nSamples/3e4 << " seconds (" << nSamples << " samples)" << std::endl;
				/*
				The buffer the data is read into - if sample_block_inc = 3e4 and
				nChannels = 64 then this buffer is about 3Mb big
				*/
				int16_t data_out[sample_block_inc][end_channel-start_channel];
				/*
				dimsm:
				an array that determines the dimensions used for the memory dataspace (see below)
				*/
				hsize_t dimsm[2];
				dimsm[0] = sample_block_inc;
				dimsm[1] = end_channel-start_channel;
				/*
				offset:
				determines the starting position in the dataspace
				offset[0] = sample, offset[1] = channel
				*/
				hsize_t offset[2];
				offset[0] = start_sample;
				offset[1] = start_channel;
				/*
				stride:
				An array that allows you to sample elements along a dimension.
				For example, a stride of one (or NULL) will select every element along a dimension,
				a stride of two will select every other element,
				and a stride of three will select an element after every two elements
				*/
				hsize_t stride[2];
				stride[0] = 1;
				stride[1] = 1;
				/*
				block:
				An array that determines the size of the element block selected from a dataspace.
				If the block size is one or NULL then the block size is a single element in that dimension.
				*/
				hsize_t block[2];
				block[0] = 1;
				block[1] = 1;
				/*
				count
				An array that determines how many blocks to select from the dataspace in each dimension.
				If the block size for a dimension is one then the count is the number of elements along that dimension.
				*/
				hsize_t count[2]; // count
				count[0] = sample_block_inc;
				count[1] = end_channel-start_channel;
				H5::DataSpace memspace(2, dimsm, NULL);
				// Progress dialog window
				wxProgressDialog prog{wxString("Exporting data"), wxString("Exporting .dat file... "), static_cast<int>((end_sample-start_sample)/SAMPLE_RATE), this};
    			int inc = 0;
				// Open the output file to write into
				std::ofstream outfile(outputfname, std::ifstream::out);
				int _end, _start;
				_end = (int)end_sample;
				_start = (int)start_sample;
				int _range = _end-_start;
				// TODO: Fix Dataspace out of range error when getting to the end of the file / end_sample
				for (int iSample = start_sample; iSample < end_sample; iSample+=sample_block_inc) {
					offset[0] = iSample;
					if ( (iSample + sample_block_inc) > end_sample ) {
						hsize_t small_sample_block_inc = end_sample - iSample;
						hsize_t small_count[2];
						small_count[0] = sample_block_inc;
						small_count[1] = end_channel-start_channel;
						int16_t small_data_chunk[sample_block_inc][end_channel-start_channel];
						dataspace.selectHyperslab(H5S_SELECT_SET, small_count, offset, stride, block);
						dataset.read(small_data_chunk, H5::PredType::NATIVE_INT16, memspace, dataspace);
						outfile.write(reinterpret_cast<char*>(&small_data_chunk), sizeof(small_data_chunk));
					}
					else {
						dataspace.selectHyperslab(H5S_SELECT_SET, count, offset, stride, block);
						dataset.read(data_out, H5::PredType::NATIVE_INT16, memspace, dataspace);
						outfile.write(reinterpret_cast<char*>(&data_out), sizeof(data_out));
					}
					prog.Update(inc);
					++inc;
				}
				std::cout << "Data written to \n";
				outfile.close();
			}
			if ( type_class == H5T_FLOAT ) {
				std::cout << "H5T_FLOAT" << std::endl;
				H5::FloatType floattype = dataset.getFloatType();
				size_t precision = floattype.getPrecision();
				std::cout << "precision " << precision << std::endl;
				H5::DataSpace dataspace = dataset.getSpace();
				int rank = dataspace.getSimpleExtentNdims();
				hsize_t dims_out[rank];
				int ndims = dataspace.getSimpleExtentDims(dims_out, NULL);
				std::cout << "rank " << rank << " dimensions " << (unsigned long)(dims_out[0]) << std::endl;
			}
			if (  type_class == H5T_STRING ) {
				std::cout << "H5T_STRING" << std::endl;
			}
			if ( type_class == H5T_BITFIELD ) {
				std::cout << "H5T_BITFIELD" << std::endl;
			}
		}
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

void MyFrame::OnExport(wxCommandEvent & evt) {
	wxTreeItemId id = m_treeCtrl->GetFocusedItem();
	std::string itemName = m_treeCtrl->GetItemText(id).ToStdString();

	wxFileDialog saveFileDialog(this, ("Save .dat file"), defaultDir, "",
        "dat files(*.dat)|*.dat",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;

    std::string outputfname = saveFileDialog.GetPath().ToStdString();
	GetDataSetInfo(itemName, outputfname);
}

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
		// Open the file dialog
		wxFileDialog saveFileDialog(this, ("Save .dat file"), defaultDir, "",
        "dat files(*.dat)|*.dat",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	    if (saveFileDialog.ShowModal() == wxID_CANCEL)
	        return;

	    std::string outputfname = saveFileDialog.GetPath().ToStdString();
		GetDataSetInfo(itemName, outputfname);
    }
}

void MyFrame::UpdateControls(const std::string & pathToDataSet) {
	if ( hdf_file ) {
		if ( hdf_file->nameExists(pathToDataSet) ) {
			H5::DataSet dataset = hdf_file->openDataSet(pathToDataSet);
			H5::DataSpace dataspace = dataset.getSpace();
			hsize_t dims_out[2];
			int ndims = dataspace.getSimpleExtentDims(dims_out, NULL);
			int nSamples = dims_out[0];
			int nChannels = dims_out[1];
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