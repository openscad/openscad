#include "CocoaUtils.h"
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

void CocoaUtils::endApplication()
{
  [[NSNotificationCenter defaultCenter] postNotificationName:NSApplicationWillTerminateNotification
                                                      object:nil];
}

void CocoaUtils::nslog(const std::string& str, void * /* userdata */)
{
  NSLog(@"%s", str.c_str());
}

void CocoaUtils::setAppearance(bool dark)
{
  NSAppearance *appearance =
    [NSAppearance appearanceNamed:dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
  [NSApp setAppearance:appearance];
}

void CocoaUtils::resetAppearance()
{
  // Passing nil lets macOS manage the app appearance based on the system
  // setting, which is required for colorSchemeChanged to fire at runtime.
  [NSApp setAppearance:nil];
}
