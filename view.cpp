#include "view.hpp"
#include <algorithm>
#include <filesystem>
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
}

bool MainFrame::compareNamesCaseInsensitive(const std::string &a,
                                            const std::string &b) {
  return wxString(a).CmpNoCase(wxString(b)) < 0;
}

bool MainFrame::compareFileData(const FileData &a, const FileData &b) const {
  bool result = false;

  switch (m_sortColumn) {
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
  }

  return m_sortAscending ? result : !result;
}

MainFrame::MainFrame(const wxString &title, FileSystemModel *model)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(900, 600)),
      m_model(model) {
  SetMinSize(wxSize(600, 400));

  m_toolbar = CreateToolBar(wxTB_HORIZONTAL | wxTB_TEXT);
  m_toolbar->AddTool(wxID_REFRESH, "Refresh",
                     wxArtProvider::GetBitmap(wxART_REDO));
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
  m_status = CreateStatusBar(1);

  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(m_splitter, 1, wxEXPAND);
  SetSizer(sizer);

  // Events
  m_tree->Bind(wxEVT_TREE_ITEM_EXPANDING, &MainFrame::onTreeExpanding, this);
  m_tree->Bind(wxEVT_TREE_SEL_CHANGED, &MainFrame::onTreeSelChanged, this);
  m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &MainFrame::onListActivated, this);
  m_list->Bind(wxEVT_LIST_COL_CLICK, &MainFrame::onColumnClick, this);
  m_list->Bind(wxEVT_CHAR_HOOK, &MainFrame::onKeyDown, this);
  this->Bind(wxEVT_CHAR_HOOK, &MainFrame::onKeyDown, this);
  Bind(wxEVT_MENU, &MainFrame::onRefresh, this, wxID_REFRESH);
  Bind(wxEVT_MENU, &MainFrame::onDelete, this, ID_PERMANENT_DELETE);
}
// destructor handles by default, no need for anything
MainFrame::~MainFrame() {}

void MainFrame::setupColumns() {
  m_list->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 300);
  m_list->InsertColumn(1, "Size", wxLIST_FORMAT_RIGHT, 100);
  m_list->InsertColumn(2, "Type", wxLIST_FORMAT_LEFT, 100);
  m_list->InsertColumn(3, "Date Modified", wxLIST_FORMAT_LEFT, 150);
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
}

wxString MainFrame::formatSize(uintmax_t bytes, bool isDir) const {
  if (isDir)
    return "--";
  if (bytes < 1024)
    return wxString::Format("%llu B", bytes);
  if (bytes < 1024 * 1024)
    return wxString::Format("%.1f KB", bytes / 1024.0);
  if (bytes < 1024 * 1024 * 1024)
    return wxString::Format("%.1f MB", bytes / (1024.0 * 1024.0));
  return wxString::Format("%.1f GB", bytes / (1024.0 * 1024.0 * 1024.0));
}

void MainFrame::updateListView(const std::vector<FileData> &files,
                               const std::string &currentPath) {
  m_currentFiles = files;
  m_currentPath = currentPath;

  if (currentPath != "/" && !currentPath.empty()) {
    m_currentFiles.insert(
        m_currentFiles.begin(),
        {"..", fs::path(currentPath).parent_path().string(), true, 0, ""});
  }

  size_t startIdx =
      (!m_currentFiles.empty() && m_currentFiles[0].name == "..") ? 1 : 0;
  std::stable_sort(m_currentFiles.begin() + startIdx, m_currentFiles.end(),
                   [this](const FileData &a, const FileData &b) {
                     return compareFileData(a, b);
                   });

  m_list->Freeze();
  m_list->DeleteAllItems();
  for (size_t i = 0; i < m_currentFiles.size(); ++i) {
    const auto &fd = m_currentFiles[i];
    long idx = m_list->InsertItem(i, fd.name);
    m_list->SetItem(idx, 1, formatSize(fd.size, fd.isDirectory));
    m_list->SetItem(idx, 2, fd.isDirectory ? "Directory" : "File");
    m_list->SetItem(idx, 3, fd.dateModified);
    m_list->SetItemPtrData(idx, static_cast<wxUIntPtr>(i));
  }
  m_list->Thaw();
  m_status->SetStatusText(wxString::Format("%zu items", m_currentFiles.size()));
}

// ── event handlers ──────────────────────────────────────────────────

void MainFrame::onTreeExpanding(wxTreeEvent &event) {
  wxTreeItemId itemId = event.GetItem();
  auto *data = dynamic_cast<TreeItemPathData *>(m_tree->GetItemData(itemId));
  if (!data)
    return;

  wxTreeItemIdValue cookie;
  wxTreeItemId firstChild = m_tree->GetFirstChild(itemId, cookie);

  // Wenn es der Dummy ist, laden wir die echten Unterordner
  if (firstChild.IsOk() && m_tree->GetItemText(firstChild) == "dummy") {
    m_tree->Delete(firstChild); // Löscht den Dummy sauber
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
    updateListView(m_model->getDirectoryContents(data->GetFullPath()),
                   data->GetFullPath());
  }
}

void MainFrame::onListActivated(wxListEvent & /*event*/) {
  long idx = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (idx < 0)
    return;

  size_t fileIndex = static_cast<size_t>(m_list->GetItemData(idx));
  if (fileIndex >= m_currentFiles.size())
    return;

  const auto &fd = m_currentFiles[fileIndex];

  if (fd.name == ".." || fd.isDirectory) {
    if (auto it = m_pathToTreeItem.find(fd.fullPath);
        it != m_pathToTreeItem.end()) {
      m_tree->SelectItem(it->second);
    }
  } else {
    wxLaunchDefaultApplication(wxString(fd.fullPath));
  }
}

void MainFrame::onColumnClick(wxListEvent &event) {
  int col = event.GetColumn();
  m_sortAscending = (col == m_sortColumn) ? !m_sortAscending : true;
  m_sortColumn = col;
  updateListView(m_currentFiles, m_currentPath);
}

void MainFrame::onRefresh(wxCommandEvent & /*event*/) { doRefresh(); }

void MainFrame::doRefresh() {
  wxTreeItemId selId = m_tree->GetSelection();
  auto *data =
      selId.IsOk()
          ? dynamic_cast<TreeItemPathData *>(m_tree->GetItemData(selId))
          : nullptr;
  if (!data)
    return;

  // Nur aus der Map entfernen, wxTreeCtrl übernimmt das "delete" des Speichers!
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

void MainFrame::onDelete(wxCommandEvent & /*event*/) { deleteSelectedItem(); }

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