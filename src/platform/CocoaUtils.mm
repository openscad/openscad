#include "CocoaUtils.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <objc/runtime.h>

#include <QMetaObject>
#include <QString>
#include <QStringList>

#include "gui/OpenSCADApp.h"

namespace {

using SetDelegateIMP = void (*)(id, SEL, id<NSApplicationDelegate>);

SetDelegateIMP originalSetDelegate = nullptr;
QStringList pendingOpenFiles;
bool openFileDispatchReady = false;

void dispatchOpenFile(NSString *filename)
{
  if (!filename) {
    return;
  }

  const QString path = QString::fromUtf8([filename fileSystemRepresentation]);

  if (!openFileDispatchReady || !scadApp) {
    if (!pendingOpenFiles.contains(path)) {
      pendingOpenFiles.append(path);
    }
    return;
  }

  QMetaObject::invokeMethod(scadApp, "handleOpenFileEvent", Qt::QueuedConnection,
                            Q_ARG(QString, path));
}

void openscad_application_openFiles(id, SEL, NSApplication *, NSArray<NSString *> *filenames)
{
  for (NSString *filename in filenames) {
    dispatchOpenFile(filename);
  }
}

void installSelectorHandler(id delegate, SEL selector, IMP implementation, const char *types)
{
  if (!delegate) return;

  Class delegateClass = [delegate class];
  Method method = class_getInstanceMethod(delegateClass, selector);
  if (method) {
    method_setImplementation(method, implementation);
  } else {
    class_addMethod(delegateClass, selector, implementation, types);
  }
}

void installOpenFilesHandler(id<NSApplicationDelegate> delegate)
{
  installSelectorHandler(delegate, @selector(application:openFiles:),
                         reinterpret_cast<IMP>(openscad_application_openFiles), "v@:@@");
}

void openscad_setDelegate(NSApplication *application, SEL command, id<NSApplicationDelegate> delegate)
{
  installOpenFilesHandler(delegate);
  originalSetDelegate(application, command, delegate);
  installOpenFilesHandler(delegate);
}

void drainPendingOpenFiles()
{
  if (!openFileDispatchReady || !scadApp || pendingOpenFiles.isEmpty()) {
    return;
  }

  const QStringList files = pendingOpenFiles;
  pendingOpenFiles.clear();
  for (const auto& file : files) {
    scadApp->handleOpenFileEvent(file);
  }
}

}  // namespace

void CocoaUtils::endApplication()
{
  [[NSNotificationCenter defaultCenter]
    postNotificationName:@"NSApplicationWillTerminateNotification"
                  object:nil];
}

void CocoaUtils::prepareOpenFileHandler()
{
  if (!originalSetDelegate) {
    Method setDelegateMethod = class_getInstanceMethod([NSApplication class], @selector(setDelegate:));
    if (setDelegateMethod) {
      originalSetDelegate = reinterpret_cast<SetDelegateIMP>(method_getImplementation(setDelegateMethod));
      method_setImplementation(setDelegateMethod, reinterpret_cast<IMP>(openscad_setDelegate));
    }
  }

  installOpenFilesHandler([NSApp delegate]);
}

void CocoaUtils::installOpenFileHandler()
{
  CocoaUtils::prepareOpenFileHandler();
  openFileDispatchReady = true;
  installOpenFilesHandler([NSApp delegate]);
  drainPendingOpenFiles();
}

void CocoaUtils::nslog(const std::string &str, void * /* userdata */)
{
  NSLog(@"%s", str.c_str());
}
