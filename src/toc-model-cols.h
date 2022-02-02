#ifndef TOC_MODEL_COLS_H
#define TOC_MODEL_COLS_H

#include <gtkmm/treemodel.h>

/**
 * \class TocModelCols
 * \brief Table of Contents Model Columns
 */
class TocModelCols : public Gtk::TreeModel::ColumnRecord
{
public:
  TocModelCols()
  {
    add(m_col_iter);
    add(m_col_level);
    add(m_col_heading);
    add(m_col_valid);
  }

  Gtk::TreeModelColumn<Gtk::TextIter> m_col_iter;
  Gtk::TreeModelColumn<int> m_col_level;
  Gtk::TreeModelColumn<Glib::ustring> m_col_heading;
  Gtk::TreeModelColumn<bool> m_col_valid;
};

#endif
