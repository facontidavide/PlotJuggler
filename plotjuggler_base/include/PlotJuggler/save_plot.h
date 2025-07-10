/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_SAVE_PLOT_H
#define PJ_SAVE_PLOT_H

#include <qwt_plot.h>

#include <QSize>
#include <QWidget>

const static QSize default_document_dimentions{ 1200, 900 };

void savePlotToFile(QSize dims, QwtPlot* plot, QWidget* parent);

#endif  // PJ_SAVE_PLOT_H
