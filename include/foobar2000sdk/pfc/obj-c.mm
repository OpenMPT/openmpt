//
//  PFC-ObjC.m
//  pfc-test
//
//  Created by PEPE on 28/07/14.
//  Copyright (c) 2014 PEPE. All rights reserved.
//
#ifdef __APPLE__
#import <Foundation/Foundation.h>


#include <TargetConditionals.h>

#if TARGET_OS_MAC && !TARGET_OS_IPHONE
#import <Cocoa/Cocoa.h>
#endif

#include "pfc.h"


namespace pfc {
    void * thread::g_entry(void * arg) {
        @autoreleasepool {
            reinterpret_cast<thread*>(arg)->entry();
        }
        return NULL;
    }
    void thread::appleStartThreadPrologue() {
        if (![NSThread isMultiThreaded]) [[[NSThread alloc] init] start];
    }
    
    bool isShiftKeyPressed() {
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
        return ( [NSEvent modifierFlags] & NSShiftKeyMask ) != 0;
#else
        return false;
#endif
    }
    bool isCtrlKeyPressed() {
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
        return ( [NSEvent modifierFlags] & NSControlKeyMask ) != 0;
#else
        return false;
#endif
    }
    bool isAltKeyPressed() {
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
        return ( [NSEvent modifierFlags] & NSAlternateKeyMask ) != 0;
#else
        return false;
#endif
    }

	void inAutoReleasePool(std::function<void()> f) {
		@autoreleasepool {		
			f();
		}
	}
}

#endif
