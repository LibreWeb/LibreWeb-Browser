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
    add(col_iter);
    add(col_level);
    add(col_heading);
    add(col_valid);
  }

  Gtk::TreeModelColumn<Gtk::TextIter> col_iter;
  Gtk::TreeModelColumn<int> col_level;
  Gtk::TreeModelColumn<Glib::ustring> col_heading;
  Gtk::TreeModelColumn<bool> col_valid;
};

#endif
