#include <wx/wx.h>

class VileApp : public wxApp {
public:
  bool OnInit() override {
    wxFrame *f = new wxFrame(nullptr, wxID_ANY, "VileFile", wxDefaultPosition,
                             wxSize(900, 600));
    f->Show(true);
    return true;
  }
};
wxIMPLEMENT_APP(VileApp);