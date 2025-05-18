#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <QtWidgets/QApplication>
#import <QtWidgets/QWidget>
#import <QEvent>
#import <QCoreApplication>
#import <QDebug>

void sendThemeChangeRecursive(QWidget* widget) {
    QEvent event(QEvent::ThemeChange);
    QCoreApplication::sendEvent(widget, &event);

    const QList<QObject*> children = widget->children();
    for (QObject* obj : children) {
        QWidget* childWidget = qobject_cast<QWidget*>(obj);
        if (childWidget) {
            sendThemeChangeRecursive(childWidget);
        }
    }
}

static void themeChanged(CFNotificationCenterRef center, void *observer,
                         CFStringRef name, const void *object,
                         CFDictionaryRef userInfo)
{
    QMetaObject::invokeMethod(qApp, []() {
        qDebug() << "Detected macOS theme change!";
        const auto topLevelWidgets = QApplication::topLevelWidgets();
        for (QWidget* widget : topLevelWidgets)
        {
            sendThemeChangeRecursive(widget);
        }
    }, Qt::QueuedConnection);
}

void registerForMacThemeChanges()
{
    CFNotificationCenterAddObserver(
        CFNotificationCenterGetDistributedCenter(),
        NULL,
        themeChanged,
        CFSTR("AppleInterfaceThemeChangedNotification"),
        NULL,
        CFNotificationSuspensionBehaviorDeliverImmediately);
}
