#include "CocoaUtils.h"
#import <Foundation/Foundation.h>

void CocoaUtils::endApplication()
{
  [[NSNotificationCenter defaultCenter]
    postNotificationName:@"NSApplicationWillTerminateNotification"
                  object:nil];
}

