/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef LINE_EDIT_H
#define LINE_EDIT_H

#include <QLineEdit>
#include <QPalette>
#include <QEvent>

/**
 * @brief A QLineEdit subclass that fixes placeholder text color when using stylesheets.
 *
 * Qt bug QTBUG-92199: Setting 'color' via stylesheet breaks placeholder text graying.
 * This class works around the bug by manually setting the placeholder color via QPalette
 * after stylesheet changes, ensuring placeholder text is always visually distinct.
 */
class LineEdit : public QLineEdit
{
public:
  explicit LineEdit(QWidget* parent = nullptr) : QLineEdit(parent)
  {
    updatePlaceholderColor();
  }

  explicit LineEdit(const QString& text, QWidget* parent = nullptr) : QLineEdit(text, parent)
  {
    updatePlaceholderColor();
  }

protected:
  void changeEvent(QEvent* event) override
  {
    QLineEdit::changeEvent(event);
    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange)
    {
      updatePlaceholderColor();
    }
  }

private:
  void updatePlaceholderColor()
  {
    QPalette pal = palette();
    QColor textColor = pal.color(QPalette::Text);
    QColor baseColor = pal.color(QPalette::Base);

    // Create a placeholder color that's between text and background
    // This makes it visible but clearly distinct from actual text
    QColor placeholderColor;
    placeholderColor.setRed((textColor.red() + baseColor.red()) / 2);
    placeholderColor.setGreen((textColor.green() + baseColor.green()) / 2);
    placeholderColor.setBlue((textColor.blue() + baseColor.blue()) / 2);

    pal.setColor(QPalette::PlaceholderText, placeholderColor);
    setPalette(pal);
  }
};

#endif  // LINE_EDIT_H
