//
//  UIIMageResizing.hh
//  DCApp_iOS
//
//  Created by Shores, David L. (JSC-ER711) on 7/5/17.
//  Copyright © 2017 Shores, David L. (JSC-ER711). All rights reserved.
//

#ifndef UIIMageResizing_h
#define UIIMageResizing_h

#include "UIKit/UIKit.h"

@interface UIImage (Resize)
- (UIImage*)scaleToSize:(CGSize)size;
@end


#endif /* UIIMageResizing_h */
