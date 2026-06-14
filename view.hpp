#ifndef VILEFILE_VIEW_H
#define VILEFILE_VIEW_H

#include "model.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/wx.h>

class TreeItemPathData : public wxTreeItemData {
  std::string m_fullPath;

public:
  explicit TreeItemPathData(const std::string &path) : m_fullPath(path) {}
  const std::string &GetFullPath() const { return m_fullPath; }
};

constexpr int ID_PERMANENT_DELETE = wxID_HIGHEST + 1;

class MainFrame : public wxFrame {
public:
  MainFrame(const wxString &title, FileSystemModel *model);
  ~MainFrame() override;

  void setupColumns();
  void initializeTree(const std::string &rootPath);
  void updateListView(const std::vector<FileData> &files,
                      const std::string &currentPath);

  static bool compareNamesCaseInsensitive(const std::string &a,
                                          const std::string &b);

private:
  void onTreeExpanding(wxTreeEvent &event);
  void onTreeSelChanged(wxTreeEvent &event);
  void onListActivated(wxListEvent &event);
  void onRefresh(wxCommandEvent &event);
  void onDelete(wxCommandEvent &event);
  void deleteSelectedItem();
  void onColumnClick(wxListEvent &event);
  void onKeyDown(wxKeyEvent &event);
  void doRefresh();
  wxString formatSize(uintmax_t bytes, bool isDir) const;
  void addTreeChild(const wxTreeItemId &parent, const std::string &path);
  bool compareFileData(const FileData &a, const FileData &b) const;

  wxSplitterWindow *m_splitter = nullptr;
  wxTreeCtrl *m_tree = nullptr;
  wxListCtrl *m_list = nullptr;
  wxStatusBar *m_status = nullptr;
  wxToolBar *m_toolbar = nullptr;
  wxImageList *m_imageList = nullptr;

  FileSystemModel *m_model = nullptr;

  std::vector<FileData> m_currentFiles;
  std::string m_currentPath;
  std::unordered_map<std::string, wxTreeItemId> m_pathToTreeItem;
  int m_sortColumn = 0;
  bool m_sortAscending = true;
};

#endif