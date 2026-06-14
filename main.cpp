#include <wx/wx.h>
#include "model.hpp"
#include "view.hpp"

class VileApp : public wxApp {
    FileSystemModel m_model;

public:
    bool OnInit() override {
        auto *frame = new MainFrame("VileFile File Manager (v1.0)", &m_model);
        frame->setupColumns();
        frame->initializeTree(FileSystemModel::getRootPath());
        frame->Show(true);
        return true;
    }
};
wxIMPLEMENT_APP(VileApp);
