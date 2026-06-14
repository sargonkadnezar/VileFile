#include "view.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <unordered_map>
#include <wx/artprov.h>

namespace fs = std::filesystem;

namespace {
void sortSubDirectories(std::vector<std::string> &dirs) {
  std::sort(dirs.begin(), dirs.end(),
            [](const std::string &a, const std::string &b) {
              return MainFrame::compareNamesCaseInsensitive(
                  std::filesystem::path(a).filename().string(),
                  std::filesystem::path(b).filename().string());
            });
}
} // namespace

wxString MainFrame::detectFileType(const std::string &name, bool isDir) {
  if (isDir)
    return "File Folder";
  std::string ext = fs::path(name).extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  static const std::unordered_map<std::string, wxString> map = {
      {".cpp", "C++ Source"},    {".hpp", "C++ Header"},
      {".h", "C Header"},        {".cc", "C++ Source"},
      {".cxx", "C++ Source"},    {".c", "C Source"},
      {".png", "PNG Image"},     {".jpg", "JPEG Image"},
      {".jpeg", "JPEG Image"},   {".gif", "GIF Image"},
      {".bmp", "BMP Image"},     {".svg", "SVG Image"},
      {".ico", "Icon"},          {".pdf", "PDF Document"},
      {".txt", "Text File"},     {".md", "Markdown"},
      {".log", "Log File"},      {".csv", "CSV File"},
      {".xml", "XML File"},      {".json", "JSON File"},
      {".html", "HTML File"},    {".css", "CSS File"},
      {".js", "JavaScript"},     {".ts", "TypeScript"},
      {".py", "Python Source"},  {".rs", "Rust Source"},
      {".zip", "Archive"},       {".tar", "Archive"},
      {".gz", "Archive"},        {".7z", "Archive"},
      {".mp3", "Audio"},         {".wav", "Audio"},
      {".flac", "Audio"},        {".mp4", "Video"},
      {".mkv", "Video"},         {".avi", "Video"},
      {".ttf", "Font"},          {".otf", "Font"},
      {".sh", "Shell Script"},   {".bash", "Shell Script"},
      {".toml", "TOML File"},    {".yaml", "YAML File"},
      {".yml", "YAML File"},     {".lock", "Lock File"},
  };

  auto it = map.find(ext);
  if (it != map.end())
    return it->second;
  if (!ext.empty())
    return ext.substr(1) + " File";
  return "File";
}

bool MainFrame::compareNamesCaseInsensitive(const std::string &a,
                                            const std::string &b) {
  return wxString(a).CmpNoCase(wxString(b)) < 0;
}

bool MainFrame::compareFileData(const FileData &a, const FileData &b,
                                int sortColumn, bool sortAscending) {
  bool result = false;

  switch (sortColumn) {
  case 0: // Name
  case 2: //
    if (a.isDirectory != b.isDirectory)
      result = a.isDirectory > b.isDirectory;
    else
      result = compareNamesCaseInsensitive(a.name, b.name);
    break;

  case 1: // Size
    if (a.isDirectory != b.isDirectory)
      result = a.isDirectory > b.isDirectory;
    else if (a.isDirectory)
      result = compareNamesCaseInsensitive(a.name, b.name);
    else
      result = a.size < b.size;
    break;

  case 3: // Date
    result = (a.dateModified.empty() ? "9999-99-99" : a.dateModified) <
             (b.dateModified.empty() ? "9999-99-99" : b.dateModified);
    break;

  case 4: // Permissions
    result = a.permissions < b.permissions;
    break;
  }

  return sortAscending ? result : !result;
}

MainFrame::MainFrame(const wxString &title, FileSystemModel *model)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(900, 600)),
      m_model(model) {
  SetMinSize(wxSize(600, 400));

  m_toolbar = CreateToolBar(wxTB_HORIZONTAL | wxTB_TEXT);
  m_backBtn = m_toolbar->AddTool(ID_NAV_BACK, "Back",
                                 wxArtProvider::GetBitmap(wxART_GO_BACK));
  m_forwardBtn = m_toolbar->AddTool(ID_NAV_FORWARD, "Forward",
                                    wxArtProvider::GetBitmap(wxART_GO_FORWARD));
  m_toolbar->AddTool(ID_NAV_UP, "Up",
                     wxArtProvider::GetBitmap(wxART_GO_DIR_UP));
  m_toolbar->AddTool(wxID_REFRESH, "Refresh",
                     wxArtProvider::GetBitmap(wxART_REDO));
  m_toolbar->AddTool(ID_NEW_FOLDER, "New Folder",
                     wxArtProvider::GetBitmap(wxART_NEW_DIR));
  m_toolbar->AddSeparator();
  m_toolbar->AddTool(ID_PERMANENT_DELETE, "Delete",
                     wxArtProvider::GetBitmap(wxART_CROSS_MARK));
  m_toolbar->Realize();

  m_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition,
                                    wxDefaultSize, wxSPLIT_VERTICAL);
  m_splitter->SetMinimumPaneSize(120);
  m_splitter->SetSashGravity(0.3);

  m_tree = new wxTreeCtrl(m_splitter, wxID_ANY, wxDefaultPosition,
                          wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_SINGLE);
  m_list = new wxListCtrl(m_splitter, wxID_ANY, wxDefaultPosition,
                          wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);

  m_splitter->SplitVertically(m_tree, m_list, GetSize().GetWidth() * 0.3);
  m_status = CreateStatusBar(2);
  int widths[] = {-1, 150};
  m_status->SetStatusWidths(2, widths);

  m_imageList = new wxImageList(16, 16);
  m_imageList->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_LIST, wxSize(16, 16)));
  m_imageList->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_LIST, wxSize(16, 16)));
  m_list->SetImageList(m_imageList, wxIMAGE_LIST_SMALL);

  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(m_splitter, 1, wxEXPAND);
  SetSizer(sizer);

  // Events
  m_tree->Bind(wxEVT_TREE_ITEM_EXPANDING, &MainFrame::onTreeExpanding, this);
  m_tree->Bind(wxEVT_TREE_SEL_CHANGED, &MainFrame::onTreeSelChanged, this);
  m_list->Bind(wxEVT_LIST_ITEM_SELECTED, &MainFrame::onListSelected, this);
  m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &MainFrame::onListActivated, this);
  m_list->Bind(wxEVT_LIST_COL_CLICK, &MainFrame::onColumnClick, this);
  m_list->Bind(wxEVT_CHAR_HOOK, &MainFrame::onKeyDown, this);
  this->Bind(wxEVT_CHAR_HOOK, &MainFrame::onKeyDown, this);
  Bind(wxEVT_MENU, &MainFrame::onRefresh, this, wxID_REFRESH);
  Bind(wxEVT_MENU, &MainFrame::onDelete, this, ID_PERMANENT_DELETE);
  Bind(wxEVT_MENU, &MainFrame::onNavigateBack, this, ID_NAV_BACK);
  Bind(wxEVT_MENU, &MainFrame::onNavigateForward, this, ID_NAV_FORWARD);
  Bind(wxEVT_MENU, &MainFrame::onNavigateUp, this, ID_NAV_UP);
  Bind(wxEVT_MENU, &MainFrame::onNewFolder, this, ID_NEW_FOLDER);

  createMenuBar();
  Bind(wxEVT_MENU, &MainFrame::onAbout, this, wxID_ABOUT);
  Bind(wxEVT_MENU, &MainFrame::onExit, this, wxID_EXIT);
  Bind(wxEVT_MENU, &MainFrame::onSortBy, this, wxID_HIGHEST + 10);
  Bind(wxEVT_MENU, &MainFrame::onSortBy, this, wxID_HIGHEST + 11);
  Bind(wxEVT_MENU, &MainFrame::onSortBy, this, wxID_HIGHEST + 12);
  Bind(wxEVT_MENU, &MainFrame::onSortBy, this, wxID_HIGHEST + 13);
  Bind(wxEVT_MENU, &MainFrame::onSortBy, this, wxID_HIGHEST + 14);
}
MainFrame::~MainFrame() { delete m_imageList; }

void MainFrame::createMenuBar() {
  auto *menuBar = new wxMenuBar;
  auto *fileMenu = new wxMenu;
  fileMenu->Append(ID_NEW_FOLDER, "New Folder\tCtrl+N");
  fileMenu->AppendSeparator();
  fileMenu->Append(wxID_EXIT, "Exit\tCtrl+Q");
  menuBar->Append(fileMenu, "&File");
  auto *editMenu = new wxMenu;
  editMenu->Append(ID_PERMANENT_DELETE, "Delete\tDel");
  editMenu->Append(wxID_REFRESH, "Refresh\tF5");
  menuBar->Append(editMenu, "&Edit");
  auto *viewMenu = new wxMenu;
  viewMenu->Append(wxID_HIGHEST + 10, "Sort by Name");
  viewMenu->Append(wxID_HIGHEST + 11, "Sort by Size");
  viewMenu->Append(wxID_HIGHEST + 12, "Sort by Type");
  viewMenu->Append(wxID_HIGHEST + 13, "Sort by Date");
  viewMenu->Append(wxID_HIGHEST + 14, "Sort by Permissions");
  menuBar->Append(viewMenu, "&View");
  auto *toolsMenu = new wxMenu;
  menuBar->Append(toolsMenu, "&Tools");
  auto *helpMenu = new wxMenu;
  helpMenu->Append(wxID_ABOUT, "About VileFile...");
  menuBar->Append(helpMenu, "&Help");

  SetMenuBar(menuBar);
}

void MainFrame::onAbout(wxCommandEvent &) {
  wxMessageBox("VileFile v1.0\n\nA simple cross-platform file manager.\nBuilt with wxWidgets and C++17.",
               "About VileFile", wxOK | wxICON_INFORMATION);
}

void MainFrame::onExit(wxCommandEvent &) {
  Close(true);
}

void MainFrame::onSortBy(wxCommandEvent &event) {
  int id = event.GetId();
  int col = id - (wxID_HIGHEST + 10);
  m_sortAscending = (col == m_sortColumn) ? !m_sortAscending : true;
  m_sortColumn = col;
  updateListView(m_currentFiles, m_currentPath);
}

void MainFrame::setupColumns() {
  m_list->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 300);
  m_list->InsertColumn(1, "Size", wxLIST_FORMAT_RIGHT, 100);
  m_list->InsertColumn(2, "Type", wxLIST_FORMAT_LEFT, 100);
  m_list->InsertColumn(3, "Date Modified", wxLIST_FORMAT_LEFT, 150);
  m_list->InsertColumn(4, "Permissions", wxLIST_FORMAT_LEFT, 110);
}

void MainFrame::pushHistoryEntry(std::vector<std::string> &history,
                                 size_t &pos, bool isNav,
                                 const std::string &path) {
  if (!isNav && (history.empty() || history[pos] != path)) {
    history.resize(pos + 1);
    history.push_back(path);
    pos = history.size() - 1;
  }
}

void MainFrame::navigateTo(const std::string &path) {
  pushHistoryEntry(m_navHistory, m_navPos, m_isHistoryNavigation, path);
  updateListView(m_model->getDirectoryContents(path), path);
  updateToolbarState();
}

void MainFrame::updateToolbarState() {
  m_backBtn->Enable(m_navPos > 0);
  m_forwardBtn->Enable(m_navPos + 1 < m_navHistory.size());
}

void MainFrame::addTreeChild(const wxTreeItemId &parent,
                             const std::string &path) {
  std::string name = fs::path(path).filename().string();
  wxTreeItemId childId =
      m_tree->AppendItem(parent, name, -1, -1, new TreeItemPathData(path));
  m_pathToTreeItem[path] = childId;
  m_tree->AppendItem(childId, "dummy");
}

void MainFrame::initializeTree(const std::string &rootPath) {
  wxTreeItemId rootId =
      m_tree->AddRoot("Root (/)", -1, -1, new TreeItemPathData(rootPath));
  m_pathToTreeItem[rootPath] = rootId;

  auto subDirs = m_model->getSubDirectories(rootPath);
  sortSubDirectories(subDirs);
  for (const auto &subPath : subDirs) {
    addTreeChild(rootId, subPath);
  }
  m_tree->Expand(rootId);
  navigateTo(rootPath);
}

wxString MainFrame::formatSize(uintmax_t bytes, bool isDir) {
  if (isDir)
    return "--";
  if (bytes < 1024)
    return wxString::Format("%llu B",
                            static_cast<unsigned long long>(bytes));
  if (bytes < 1024 * 1024)
    return wxString::Format("%.1f KB",
                            static_cast<unsigned long long>(bytes) / 1024.0);
  if (bytes < 1024UL * 1024 * 1024)
    return wxString::Format("%.1f MB",
                            static_cast<unsigned long long>(bytes) /
                                (1024.0 * 1024.0));
  if (bytes < 1024UL * 1024 * 1024 * 1024)
    return wxString::Format("%.1f GB",
                            static_cast<unsigned long long>(bytes) /
                                (1024.0 * 1024.0 * 1024.0));
  return wxString::Format("%.1f TB",
                          static_cast<unsigned long long>(bytes) /
                              (1024.0 * 1024.0 * 1024.0 * 1024.0));
}

void MainFrame::updateListView(const std::vector<FileData> &files,
                               const std::string &currentPath) {
  m_currentFiles = files;
  m_currentPath = currentPath;

  if (currentPath != "/" && !currentPath.empty()) {
    m_currentFiles.insert(
        m_currentFiles.begin(),
        {"..", fs::path(currentPath).parent_path().string(), true, 0, "", ""});
  }

  size_t startIdx =
      (!m_currentFiles.empty() && m_currentFiles[0].name == "..") ? 1 : 0;
  std::stable_sort(m_currentFiles.begin() + startIdx, m_currentFiles.end(),
                   [this](const FileData &a, const FileData &b) {
                     return compareFileData(a, b, m_sortColumn, m_sortAscending);
                   });

  m_list->Freeze();
  m_list->DeleteAllItems();
  for (size_t i = 0; i < m_currentFiles.size(); ++i) {
    const auto &fd = m_currentFiles[i];
    long idx = m_list->InsertItem(i, fd.name, fd.isDirectory ? 0 : 1);
    m_list->SetItem(idx, 1, formatSize(fd.size, fd.isDirectory));
    m_list->SetItem(idx, 2, detectFileType(fd.name, fd.isDirectory));
    m_list->SetItem(idx, 3, fd.dateModified);
    m_list->SetItem(idx, 4, fd.permissions);
    m_list->SetItemPtrData(idx, static_cast<wxUIntPtr>(i));
  }
  m_list->Thaw();
  m_status->SetStatusText(wxString::Format("VileFile: %s", m_currentPath), 0);
  m_status->SetStatusText(wxString::Format("%zu items", m_currentFiles.size()), 1);
}

void MainFrame::onTreeExpanding(wxTreeEvent &event) {
  wxTreeItemId itemId = event.GetItem();
  auto *data = dynamic_cast<TreeItemPathData *>(m_tree->GetItemData(itemId));
  if (!data)
    return;

  wxTreeItemIdValue cookie;
  wxTreeItemId firstChild = m_tree->GetFirstChild(itemId, cookie);

  if (firstChild.IsOk() && m_tree->GetItemText(firstChild) == "dummy") {
    m_tree->Delete(firstChild);
    auto subDirs = m_model->getSubDirectories(data->GetFullPath());
    sortSubDirectories(subDirs);
    for (const auto &subPath : subDirs) {
      addTreeChild(itemId, subPath);
    }
  }
  event.Skip();
}

void MainFrame::onTreeSelChanged(wxTreeEvent &event) {
  if (auto *data = dynamic_cast<TreeItemPathData *>(
          m_tree->GetItemData(event.GetItem()))) {
    navigateTo(data->GetFullPath());
  }
  event.Skip();
}

void MainFrame::onListSelected(wxListEvent &event) {
  long idx = event.GetIndex();
  if (idx < 0)
    idx = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (idx < 0)
    return;

  size_t fileIndex = static_cast<size_t>(m_list->GetItemData(idx));
  if (fileIndex >= m_currentFiles.size())
    return;

  const auto &fd = m_currentFiles[fileIndex];

  if (fd.name == "..") {
    std::string parent = fs::path(m_currentPath).parent_path().string();
    if (!parent.empty())
      navigateTo(parent);
  } else if (fd.isDirectory) {
    navigateTo(fd.fullPath);
  }
}

void MainFrame::onListActivated(wxListEvent &event) {
  long idx = event.GetIndex();
  if (idx < 0)
    idx = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (idx < 0)
    return;

  size_t fileIndex = static_cast<size_t>(m_list->GetItemData(idx));
  if (fileIndex >= m_currentFiles.size())
    return;

  const auto &fd = m_currentFiles[fileIndex];

  if (!fd.isDirectory && fd.name != "..") {
    wxLaunchDefaultApplication(wxString(fd.fullPath));
  }
}

void MainFrame::onColumnClick(wxListEvent &event) {
  int col = event.GetColumn();
  m_sortAscending = (col == m_sortColumn) ? !m_sortAscending : true;
  m_sortColumn = col;
  updateListView(m_currentFiles, m_currentPath);
}

void MainFrame::onRefresh(wxCommandEvent &event) { doRefresh(); }

void MainFrame::doRefresh() {
  wxTreeItemId selId = m_tree->GetSelection();
  auto *data =
      selId.IsOk()
          ? dynamic_cast<TreeItemPathData *>(m_tree->GetItemData(selId))
          : nullptr;
  if (!data)
    return;

  wxTreeItemIdValue cookie;
  wxTreeItemId child = m_tree->GetFirstChild(selId, cookie);
  while (child.IsOk()) {
    if (auto *childData =
            dynamic_cast<TreeItemPathData *>(m_tree->GetItemData(child))) {
      m_pathToTreeItem.erase(childData->GetFullPath());
    }
    child = m_tree->GetNextChild(selId, cookie);
  }

  m_tree->DeleteChildren(selId);
  m_tree->AppendItem(selId, "dummy");

  if (m_tree->IsExpanded(selId)) {
    m_tree->Collapse(selId);
    m_tree->Expand(selId);
  }

  updateListView(m_model->getDirectoryContents(data->GetFullPath()),
                 data->GetFullPath());
}

void MainFrame::onDelete(wxCommandEvent &event) { deleteSelectedItem(); }

void MainFrame::deleteSelectedItem() {
  long idx = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (idx < 0)
    return;

  size_t fileIndex = static_cast<size_t>(m_list->GetItemData(idx));
  if (fileIndex >= m_currentFiles.size())
    return;

  const auto &fd = m_currentFiles[fileIndex];
  if (fd.name == "..")
    return;

  if (fs::is_symlink(fd.fullPath)) {
    wxMessageBox("Cannot delete symlinks for safety.", "Blocked",
                 wxOK | wxICON_WARNING);
    return;
  }

  wxString msg = "Permanently delete '" + fd.name +
                 (fd.isDirectory ? "' and ALL contents?" : "'?");
  if (wxMessageBox(msg + " This cannot be undone!", "Delete",
                   wxYES_NO | wxICON_WARNING) != wxYES)
    return;

  try {
    fd.isDirectory ? fs::remove_all(fd.fullPath) : fs::remove(fd.fullPath);
  } catch (const fs::filesystem_error &e) {
    wxMessageBox(e.what(), "Error", wxOK | wxICON_ERROR);
    return;
  }
  doRefresh();
}

void MainFrame::onKeyDown(wxKeyEvent &event) {
  int key = event.GetKeyCode();
  if (key == WXK_DELETE) {
    deleteSelectedItem();
  } else if (key == WXK_F5) {
    doRefresh();
  } else {
    event.Skip();
  }
}

void MainFrame::onNavigateBack(wxCommandEvent &event) {
  if (m_navPos > 0) {
    m_isHistoryNavigation = true;
    --m_navPos;
    navigateTo(m_navHistory[m_navPos]);
    m_isHistoryNavigation = false;
  }
}

void MainFrame::onNavigateForward(wxCommandEvent &event) {
  if (m_navPos + 1 < m_navHistory.size()) {
    m_isHistoryNavigation = true;
    ++m_navPos;
    navigateTo(m_navHistory[m_navPos]);
    m_isHistoryNavigation = false;
  }
}

void MainFrame::onNavigateUp(wxCommandEvent &event) {
  navigateTo(fs::path(m_currentPath).parent_path().string());
}

void MainFrame::onNewFolder(wxCommandEvent &event) {
  wxTextEntryDialog dlg(this, "Enter folder name:", "New Folder");
  if (dlg.ShowModal() == wxID_OK) {
    fs::path safe = fs::path(m_currentPath) / fs::path(dlg.GetValue().ToStdString()).filename();
    try {
      fs::create_directory(safe);
      doRefresh();
    } catch (const fs::filesystem_error &e) {
      wxMessageBox(e.what(), "Error", wxOK | wxICON_ERROR);
    }
  }
}