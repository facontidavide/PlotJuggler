/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <QFontMetrics>
#include <QLabel>
#include <QResizeEvent>
#include <QString>

// QLabel variant that elides its text with "…" when it doesn't fit the
// available width, instead of demanding its full natural width and forcing
// the parent layout wider. The full text is also set as the tool-tip so
// the user can still hover to read it.
//
// setText is *shadowed*, not virtual — hold as ElidedLabel*, not QLabel*.
class ElidedLabel : public QLabel
{
public:
  explicit ElidedLabel(QWidget* parent = nullptr) : QLabel(parent)
  {
    // Horizontal Ignored: layout may give any width, independent of sizeHint.
    // Combined with the elision below this stops long text from widening the
    // window.
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  }

  void setText(const QString& text)
  {
    full_text_ = text;
    setToolTip(text);
    updateElided();
  }

  QString text() const
  {
    return full_text_;
  }

protected:
  void resizeEvent(QResizeEvent* event) override
  {
    QLabel::resizeEvent(event);
    updateElided();
  }

private:
  void updateElided()
  {
    const int avail = contentsRect().width();
    if (avail <= 0)
    {
      // Not yet laid out — defer to the first resizeEvent.
      QLabel::setText(full_text_);
      return;
    }
    QFontMetrics fm(font());
    QLabel::setText(fm.elidedText(full_text_, Qt::ElideRight, avail));
  }

  QString full_text_;
};
