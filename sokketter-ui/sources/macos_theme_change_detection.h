#ifndef MACOS_THEME_CHANGE_DETECTION_H
#define MACOS_THEME_CHANGE_DETECTION_H

#pragma once

#include <QtGlobal>

#ifdef Q_OS_MACOS
void registerForMacThemeChanges();
#endif

#endif // MACOS_THEME_CHANGE_DETECTION_H
