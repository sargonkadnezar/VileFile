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
constexpr int ID_NAV_BACK = wxID_HIGHEST + 2;
constexpr int ID_NAV_FORWARD = wxID_HIGHEST + 3;
constexpr int ID_NAV_UP = wxID_HIGHEST + 4;
constexpr int ID_NEW_FOLDER = wxID_HIGHEST + 5;

class MainFrame : public wxFrame {
public:
  MainFrame(const wxString &title, FileSystemModel *model);
  ~MainFrame() override;

  void setupColumns();
  void initializeTree(const std::string &rootPath);
  void updateListView(const std::vector<FileData> &files,
                      const std::string &currentPath);

  static wxString formatSize(uintmax_t bytes, bool isDir);
  static wxString detectFileType(const std::string &name, bool isDir);
  static bool compareNamesCaseInsensitive(const std::string &a,
                                          const std::string &b);
  static bool compareFileData(const FileData &a, const FileData &b,
                              int sortColumn, bool sortAscending);
  static void pushHistoryEntry(std::vector<std::string> &history, size_t &pos,
                               bool isNav, const std::string &path);

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
  void addTreeChild(const wxTreeItemId &parent, const std::string &path);
  void navigateTo(const std::string &path);
  void updateToolbarState();
  void onNavigateBack(wxCommandEvent &event);
  void onNavigateForward(wxCommandEvent &event);
  void onNavigateUp(wxCommandEvent &event);
  void onNewFolder(wxCommandEvent &event);
  void createMenuBar();
  void onAbout(wxCommandEvent &event);
  void onExit(wxCommandEvent &event);
  void onSortBy(wxCommandEvent &event);

  wxSplitterWindow *m_splitter = nullptr;
  wxTreeCtrl *m_tree = nullptr;
  wxListCtrl *m_list = nullptr;
  wxStatusBar *m_status = nullptr;
  wxToolBar *m_toolbar = nullptr;
  wxToolBarToolBase *m_backBtn = nullptr;
  wxToolBarToolBase *m_forwardBtn = nullptr;
  wxImageList *m_imageList = nullptr;

  std::vector<std::string> m_navHistory;
  size_t m_navPos = 0;
  bool m_isHistoryNavigation = false;

  FileSystemModel *m_model = nullptr;

  std::vector<FileData> m_currentFiles;
  std::string m_currentPath;
  std::unordered_map<std::string, wxTreeItemId> m_pathToTreeItem;
  int m_sortColumn = 0;
  bool m_sortAscending = true;
};

#endif