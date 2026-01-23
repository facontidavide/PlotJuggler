#ifndef PJ_DIALOG_UTILS_H
#define PJ_DIALOG_UTILS_H

#include <QDialog>
#include <QLayout>
#include <QWidget>

namespace PJ
{

/**
 * @brief Adjusts dialog size to fit contents after widget visibility changes
 *
 * This function resets dialog size constraints and forces the layout system
 * to recalculate the optimal size. Useful when showing/hiding option widgets
 * in plugin dialogs.
 *
 * @param dialog The dialog to resize
 */
inline void adjustDialogToContent(QDialog* dialog)
{
  if (!dialog)
  {
    return;
  }
  dialog->setMinimumHeight(0);
  dialog->layout()->invalidate();
  dialog->layout()->activate();
  dialog->adjustSize();
}

/**
 * @brief Shows/hides parser options widget and its container, then resizes dialog
 *
 * Hides the options container if the widget is null.
 *
 * @param dialog The dialog to resize
 * @param options_box The container widget for parser options
 * @param options_widget The parser's options widget (may be null)
 */
inline void showOptionsWidget(QDialog* dialog, QWidget* options_box, QWidget* options_widget)
{
  if (options_widget)
  {
    options_widget->setVisible(true);
    options_box->setVisible(true);
  }
  else
  {
    options_box->setVisible(false);
  }
  adjustDialogToContent(dialog);
}

}  // namespace PJ

#endif  // PJ_DIALOG_UTILS_H
