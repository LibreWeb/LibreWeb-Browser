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
    add(m_col_id);
    add(m_col_heading);
  }

  Gtk::TreeModelColumn<int> m_col_id;
  Gtk::TreeModelColumn<Glib::ustring> m_col_heading;
};

#endif
