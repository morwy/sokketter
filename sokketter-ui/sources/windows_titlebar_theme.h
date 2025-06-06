#ifndef WINDOWS_TITLEBAR_THEME_H
#define WINDOWS_TITLEBAR_THEME_H

#pragma once

#include <QWindow>
#include <QtGlobal>

#ifdef Q_OS_WIN
void toggle_dark_titlebar(WId window_id, const bool enabled);
#endif

#endif // WINDOWS_TITLEBAR_THEME_H
