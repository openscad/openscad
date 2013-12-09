#include "CocoaUtils.h"
#import <Foundation/Foundation.h>

void CocoaUtils::endApplication()
{
  [[NSNotificationCenter defaultCenter]
    postNotificationName:@"NSApplicationWillTerminateNotification"
                  object:nil];
}

void CocoaUtils::nslog(const std::string &str, void * /* userdata */)
{       
  NSLog(@"%s", str.c_str());
}
