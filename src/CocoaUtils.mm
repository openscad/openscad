#include "CocoaUtils.h"
#import <Foundation/Foundation.h>
#include <stdio.h>

void CocoaUtils::endApplication()
{
  [[NSNotificationCenter defaultCenter]
    postNotificationName:@"NSApplicationWillTerminateNotification"
                  object:nil];
}

std::string CocoaUtils::documentsPath()
{
  return std::string([[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] UTF8String]);
}
